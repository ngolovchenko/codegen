#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "listnode.h"
#include "listtree.h"

listtree::listtree(bool insens, bool whole)
{
    ListIndex = 0;	//points to the next element
    Start = 0;
    nextstate = 1;
    keywords = 0;
    insensitive = insens;
    wholeword = whole;
    searchindex = 0;
    searchnextindex = 0;
    Error = 0;
}

listtree :: ~listtree()
{
    //free memory
    while (--ListIndex >= 0)
    {
        delete List[ListIndex];
    }
}

char* listtree::addkeyword(char* t)
{
    listnode* ptr, * cpy;
    cpy = ptr = Start;
    int state = 0;	//initial state (for the first character)

    keywords++;	//keywords codes start from 1
    do
    {
        //add one character to the tree
        if (state != 0)
        {
            //next character
            if (ptr->nextchar != 0)
            {
                ptr = ptr->nextchar;
            }
            else
            {
                ptr = ptr->nextchar = new listnode('\0');
                List[ListIndex++] = ptr;	//put it in linear list
            }
        }

        //scan if the character already there
        while (ptr != NULL)
        {
            cpy = ptr;
            //check if there is an empty place
            if (ptr->val == '\0')
            {
                //fill the empty space
                ptr->passes++;
                ptr->val = *t;
                ptr->state = state;
                nextstate++;
                if (isspace(*(t + 1)) || (*(t + 1) == '\0'))
                {
                    //this was the last character
                    ptr->setkeycode(keywords);
                    //reserve one more state
                    ptr->reserved = nextstate++;
                }
                break;
            }
            else
            {
                if (ptr->val == *t)
                {
                    //found equal in alternatives list
                    ptr->passes++;
                    if (ptr->reserved != 0)
                    {
                        if (isspace(*(t + 1)) || (*(t + 1) == '\0'))
                        {
                            Error = 5;	//two same keywords
                        }
                        else
                        {
                            //matched, but this is the end of some keyword
                            //compensate for reserved state
                            state = ptr->reserved;
                            ptr->reserved = 0;
                            ptr = ptr->nextchar = new listnode('\0');
                            List[ListIndex++] = ptr;	//put it in linear list
                            t++;
                            cpy = ptr;
                            nextstate--;
                            continue;
                        }
                    }
                    if (isspace(*(t + 1)) || (*(t + 1) == '\0'))
                    {
                        if (ptr->keyword != 0)
                        {
                            Error = 5;
                        }
                        //this was the last character - set key and preserve state
                        ptr->setkeycode(keywords);
                        /*/add one state for the last character
                        //protect from state overwriting by a smaller word
                        if(ptr->state == 0)
                        {
                            ptr->state = state;
                            nextstate++;
                        }*/
                    }
                    ptr->val = *t;
                    break;
                }
            }
            //not equal - take alternative
            ptr = ptr->altchar;
        }

        if (ptr == NULL)
        {
            //not found in alternatives
            ptr = cpy;

            if (Start == NULL)
            {
                //no start node yet
                ptr = Start = new listnode(*t);
                List[ListIndex++] = ptr;	//put it in linear list
            }
            else
            {
                state = ptr->state;
                ptr = ptr->altchar = new listnode(*t);
                List[ListIndex++] = ptr;	//put it in linear list
            }

            //set state number
            ptr->state = state;
            ptr->passes++;

            if (isspace(*(t + 1)) || (*(t + 1) == '\0'))
            {
                //this was the last character
                ptr->setkeycode(keywords);
                //reserve one state
                ptr->reserved = nextstate++;
            }
        }
        else
        {
            ptr = cpy;
        }
        state = nextstate;

    } while ((!isspace(*++t)) && (*t != '\0'));

    return t;
}

size_t listtree::jumptable(char* code)
{
    int i, k;

    size_t s = strlen(code);
    k = totalstates();
    for (i = 1; i <= k; i++)
    {
        s += sprintf(code + s, "\tgoto\tparse_state%d\n", i);
    }
    return s;
}

size_t listtree::statetable(char* code)
{
    int i, k, x;
    listnode* ptr, * ptr2, * lastptr = 0, * firstptr = 0;
    char last;

    size_t s = strlen(code);
    k = totalstates();
    for (i = 0; i <= k; i++)	//totalstates don't include zero state
    {
        last = '\0';

        //scan through all states
        while ((ptr = searchstate(i)) != NULL)
        {
            //state i found
            if (last == '\0')
            {
                //first two lines
                s += sprintf(code + s, "parse_state%d\n\
\tmovf\tparse_char, w\n", i);
                s += sprintf(code + s, "\taddlw\t-'%.1s'\n", &ptr->val);
                firstptr = ptr;
            }
            else
            {
                s += sprintf(code + s, "\taddlw\t'%.1s'-'%.1s'\n", &last, &ptr->val);
            }
            s += sprintf(code + s, "\tskpnz\n");
            if ((x = ptr->reserved) == 0)
            {
                x = ptr->nextchar->state;
            }
            s += sprintf(code + s, "\t retlw\t%#.2x\n", keywords + 1 + x);
            last = ptr->val;
            lastptr = ptr;
        }
        if (last == '\0')
        {
            //state not found - lookahead check?
            ptr2 = searchnextstate2(i);
            s += sprintf(code + s, "parse_state%d\n\tmovlw\t%#.2x\n", i, ptr2->keyword);
            s += sprintf(code + s, "\tgoto\tparse_state_delimiter\n");
        }
        else
        {
            //state found at least once - 
            //print final part of state
            //4 options:
            //1) movlw keyword, movwf parse_state, goto parse_state_delimiter
            //   (previous in next-chain state was last character in a keyword)
            //2) retlw 255
            //   (not (1), whole words match)
            //3) goto parse_state_(reserved)
            //   (not (1), not whole words match, further chain is resolved)
            //4) empty
            //   (not (1), not whole words match, -"-, reserved state is a next state)
            ptr2 = searchnextstate(firstptr);
            if ((ptr2 != NULL) && (ptr2->keyword != 0))
            {
                //option 1
                s += sprintf(code + s, "\tmovlw\t%#.2x\n", ptr2->keyword);
                //s += sprintf(code + s, "\tmovwf\tparse_state\n");
                s += sprintf(code + s, "\tgoto\tparse_state_delimiter\n");
            }
            else
            {
                if (ptr2 == NULL)
                {
                    //not preceder in next-chain
                    s += sprintf(code + s, "\tretlw\t0xFF\n");
                }
                else
                {
                    if (wholeword)
                    {
                        //option 2
                        s += sprintf(code + s, "\tretlw\t0xFF\n");
                    }
                    else
                    {
                        if (lastptr->passes != 1)
                        {
                            s += sprintf(code + s, "\tretlw\t0xFF\n");
                        }
                        else
                        {
                            if ((lastptr->reserved - i) != 1)
                            {
                                //option 3
                                ptr2 = lastptr;
                                while (ptr2->nextchar != 0)
                                {
                                    ptr2 = ptr2->nextchar;
                                }
                                s += sprintf(code + s, "\tgoto\tparse_state%d\n", ptr2->reserved);
                            }
                        }
                    }
                }
            }

        }
    }
    return s;
}

listnode* listtree::searchstate(int state)
{
    while (searchindex < ListIndex)
    {
        if (List[searchindex++]->state == state)
        {
            return List[searchindex - 1];
        }
    }
    searchindex = 0;
    return NULL;
}
listnode* listtree::searchnextstate(listnode* to)
{
    listnode* ptr;
    searchnextindex = 0;
    while (searchnextindex < ListIndex)
    {
        ptr = List[searchnextindex++];
        if (ptr->nextchar == to)
        {
            return List[searchnextindex - 1];
        }
    }
    return NULL;
}
listnode* listtree::searchnextstate2(int tostate)
{
    listnode* ptr;
    searchnextindex = 0;
    while (searchnextindex < ListIndex)
    {
        ptr = List[searchnextindex++];
        if (ptr->reserved == tostate)
        {
            return List[searchnextindex - 1];
        }
    }
    return NULL;
}
