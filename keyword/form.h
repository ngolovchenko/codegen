#ifndef form_h
#define form_h

class LinkedStrings
{
public:
    LinkedStrings();
    LinkedStrings* addString(
        LinkedStrings** List,
        char* newstr);
    char str[20];
    LinkedStrings* next;
};

class TextFormEntry
{
public:
    TextFormEntry();
    ~TextFormEntry();
    void Clear();
    void ParseCharsList();
    void printList();
    void printfCode();
    char textBuf[256];
    char text[256];
    LinkedStrings* CharsList;	//parsed list of chars ('x', 's', 0x0d, 13, 12..)

};

#endif