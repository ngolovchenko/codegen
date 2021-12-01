#ifndef reg_h
#define reg_h

#define MAXSNIPSIZE 2048
#define MAXREGSNUMBER 10

enum endianType{big, little};


class Reg
{
public:
    Reg();
    Reg(char *N, char b, endianType endi);
    Reg(const char *N, char b, endianType endi);
    Reg(Reg& twin);
    ~Reg();
    char GetSize()                  //size of reg in bits (leads excluded)
    {
        return Bits;
    }
    void SetSize(char b, char l);   //set size of reg in bits and leads
    char GetBytes()                 //how many bytes from GetSize
    {
        return (Bits + 7) / 8;
    }
    char GetBytesTotal()            //including Leads
    {
        return (Bits + Leads + 7) / 8;
    }
    char *Round();  //shift right and increment if carry set
    char *ShiftRight(char Shift);       // shift right, truncates fraction part
    char *ShiftLeft(char Shift);        // shift left
    char *ShowUsedRegs();           //generate code for declaration
    char *Add(Reg *R2);     //add other register (left aligned)
    char *Sub(Reg *R2);     //substract other register(left aligned)
    char *Neg();        //change sign
    char *Copy(Reg *R2);
    char *CopyToUsed(Reg *R2); //copy only to ever used regs

    char *Name;
    bool Zero;  // Shift will set the flag if result is zero
    bool Sign;  //do sign extension for right shift
    endianType endian;  //big or little
    void setWasUsed(bool x, char Index);
    bool getWasUsed();
    bool getWasUsed(char Index);
    void clearWasUsed();

protected:
    //bool wasUsed; //indicates that the register was accessed for read
    void CheckBytesUsed();      //increase used bytes counter if needed
    void Normalize(Reg *R2);    //normalize register to add/substract  R2
    char Bits,  // significant bits number
        Leads,  // number of bits stuffed before bits field
        BytesUsed;  // number of registers for storage
    char str[MAXSNIPSIZE]; //snippet for code generation functions
//big endians:
    char *RoundBE();    
    char *ShiftRightBE(char Shift);
    char *ShiftLeftBE(char Shift);
    char *AddBE(Reg *R2);
    char *SubBE(Reg *R2);
    char *NegBE();
//little endians:
    char *RoundLE();    
    char *ShiftRightLE(char Shift);
    char *ShiftLeftLE(char Shift);
    char *AddLE(Reg *R2);
    char *SubLE(Reg *R2);
    char *NegLE();
    void NormalizeBE(Reg *R2);
    void NormalizeLE(Reg *R2);
/*
    Signed register is represented in twos complement form (-A = ~A + 1)
    Bits don't include the sign shown by higher bits (Leads), but only the
    number of significant bits.
    If Leads = 0 for signed register, the sign bit then is in carry
    Sign in carry is inverted for PICs, since substraction clears carry
    flag on borrow. Therefore signed shift left (and right) doesn't put
    sign in carry, but makes it explicit.
*/
    bool wasUsed[MAXREGSNUMBER];
};


#endif
