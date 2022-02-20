#include "listnode.h"

listnode::listnode(char v)
{
    keyword = 0;
    nextchar = 0;
    altchar = 0;
    val = v;
    state = 0;
    passes = 0;
    reserved = 0;
}

void listnode::setkeycode(int code)
{

    this->keyword = code;
}
