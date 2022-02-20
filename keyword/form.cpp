#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "form.h"

LinkedStrings::LinkedStrings()
{
    str[0] = '\0';
    next = NULL;
}

LinkedStrings* LinkedStrings::addString(
    LinkedStrings** List,
    char* newstr
)
{
    int i = 0;
    LinkedStrings* last = this;

    if (*List == NULL)
    {
        last = *List = new LinkedStrings();
    }
    else
    {
        //skip to the last parsed character
        while (last->next != NULL) last = last->next;
        last = last->next = new LinkedStrings();
    }

    while (newstr[i] != '\0' && i < 19)
    {
        last->str[i] = newstr[i];
        i++;
    }
    last->str[i] = '\0';
    return last;
}
//-------------------------------------------------------------------------
TextFormEntry::TextFormEntry()
{
    CharsList = NULL;
    text[0] = '\0';
}
TextFormEntry :: ~TextFormEntry()
{
    //clean-up
    Clear();
}

void TextFormEntry::Clear()
{
    LinkedStrings* ptr, * nextptr;
    for (ptr = CharsList; ptr != NULL; ptr = nextptr)
    {
        nextptr = ptr->next;
        delete ptr;
    }
    CharsList = NULL;
    text[0] = '\0';
}

void TextFormEntry::ParseCharsList()
{
    unsigned int i, subi;
    char substr[20], ch;
    bool openQuote = false, startOfChar = false;
    LinkedStrings* list = CharsList;


    //parse comma separated characters list
    for (i = 0, subi = 0; i < strlen(text) && subi < 19; i++)
    {
        ch = text[i];
        if (openQuote)
        {
            substr[subi++] = ch;
            if (ch == '\'')
            {
                openQuote = false;
                startOfChar = false;
                substr[subi] = '\0';
                subi = 0;
                list = list->addString(&CharsList, substr);
            }
        }
        else
        {
            if (startOfChar)
            {
                //char ended?
                if (isspace(ch) || ch == ',')
                {
                    //is whitespace
                    startOfChar = false;
                    openQuote = false;
                    substr[subi] = '\0';
                    subi = 0;
                    list = list->addString(&CharsList, substr);
                }
                else
                {
                    substr[subi++] = ch;
                }
            }
            else
            {
                //skip whitespaces
                if (!(isspace(ch) || ch == ','))
                {
                    //not whitespace
                    startOfChar = true;
                    if (ch == '\'')
                    {
                        openQuote = true;
                    }
                    substr[subi++] = ch;
                }
            }
        }
    }
    //finalize
    if (startOfChar)
    {
        substr[subi] = '\0';
        subi = 0;
        list = list->addString(&CharsList, substr);
    }
}
void TextFormEntry::printList()
{
    int i = 0;
    LinkedStrings* list = CharsList;

    textBuf[0] = '\0';
    if (list == NULL)
    {
        strcpy(textBuf, "no elements");
    }
    while (list != NULL)
    {
        i += sprintf(textBuf + i, "%s", list->str);
        list = list->next;
        if (list != NULL)
        {
            i += sprintf(textBuf + i, ", ");
        }
    }
}
void TextFormEntry::printfCode()
{
    LinkedStrings* list = CharsList, * last;

    if (list != NULL)
    {
        printf("\taddlw\t-%s\t\t;accumulator == %s ?\n", list->str, list->str);
        printf("\tskpnz\n\t retlw 0\t\t;return if equal\n");
    }
    last = list;
    while ((list = list->next) != NULL)
    {
        printf("\taddlw\t%s - %s\t;restore accumulator and check if equals %s\n", last->str, list->str, list->str);
        printf("\tskpnz\n\t retlw 0\t\t;return if equal\n");
        last = list;
    }
}
