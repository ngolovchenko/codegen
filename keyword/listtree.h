#ifndef listtree_h
#define listtree_h

#define MAXLISTNODES 512

class listtree
{
public:
    listtree(bool insens, bool whole);
    ~listtree();
    int totalstates()
    {
        return nextstate - 1;
    }
    char* addkeyword(char* t); /* add a keyword to the tree */
    size_t jumptable(char* code);
    size_t statetable(char* code);
    listnode* searchstate(int state);
    listnode* searchnextstate(listnode* to);
    listnode* searchnextstate2(int tostate);

    int keywords;
    int Error;
protected:
    bool insensitive;
    bool wholeword;
    int searchindex;
    int searchnextindex;
    listnode* Start;	//root node
    listnode* List[MAXLISTNODES]; //linear list of all nodes
    int ListIndex;
    int nextstate;
};

#endif
