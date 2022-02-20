#ifndef listnode_h
#define listnode_h

class listnode
{
public:
    listnode(char v);
    void setkeycode(int code); /* saves code of keyword */
    char val;		//character value
    int keyword;	//keyword code(nextchar should be 0)
    int state;		//state number
    int passes;	    //how many keywords passed this state
    int reserved;   //reserved state
    listnode* nextchar;	//next char in a string
    listnode* altchar;	//alternative char
protected:
};

#endif