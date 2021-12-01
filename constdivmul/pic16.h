#ifndef pic16_h
#define pic16_h

enum CPU_Code {CPU_PIC16, CPU_SX28};

class PIC16
{
public:
    PIC16();
    virtual ~PIC16();
    void initialize();
    void SetNoteCarry(bool val, Reg *R);    //Set vNoteCarry flag
    void SetValidCarry(bool val, Reg *R);   //Set vValidCarry flag
    bool GetNoteCarry(Reg *R);              //Get vNoteCarry flag
    bool GetValidCarry(Reg *R);             //Get vValidCarry flag
//member functions to generate code
    char *TakeCarry();                      //Takes care of carry
    void ClearSnippet(void)
    {
        str[0] = 0;
    }
    char *GetSnippet(void)
    {
        return str;
    }
    virtual char *movwf(Reg *File, char Index);     //movwf File->Name+'index'
    virtual char *movfw(Reg *File, char Index);     //movf File->Name+'index', w
    virtual char *swapfw(Reg *File, char Index);
    virtual char *swapff(Reg *File, char Index);
    virtual char *movlw(char unsigned literal);
    virtual char *andlw(char unsigned literal);
    virtual char *clrf(Reg *File, char Index);      //
    virtual char *clrw();
    virtual char *btfsc(Reg *File, char Index, char Bit);
    virtual char *btfss(Reg *File, char Index, char Bit);
    virtual char *bsf(Reg *File, char Index, char Bit);
    virtual char *comff(Reg *File, char Index);     //comf Name, f
    virtual char *clrc();
    virtual char *com_msb(Reg *File, char Index);   //invert msb
    virtual char *skpc();
    virtual char *skpnc();
    virtual char *skpnz();
    virtual char *rlff(Reg *File, char Index);
    virtual char *rlfw(Reg *File, char Index);
    virtual char *rrff(Reg *File, char Index);
    virtual char *rrfw(Reg *File, char Index);
    virtual char *addwff(Reg *File, char Index);
    virtual char *subwff(Reg *File, char Index);
    virtual char *iorwff(Reg *File, char Index);
    virtual char *iorlw(char unsigned literal);
    virtual char *incfszw(Reg *File, char Index);
    virtual char *incff(Reg *File, char Index);
    virtual char *decff(Reg *File, char Index);
    virtual char *xorwff(Reg *File, char Index);
    virtual char *jmp(char* x);
    char *label(char* x);
    virtual CPU_Code whatCPU();
    void clearCounter();
    int getCounter();

    Reg *cHolder;       //What register holds rights for Carry

protected:
    bool vNoteCarry,    //signals if carry contains MSB after addtion
        vValidCarry;    //set if carry can be used as an extension of register
    char str[MAXSNIPSIZE]; //snippet for code generation functions
//w Holder pointer and index
    Reg *wHolder;
    char wIndex;
    bool wValid(Reg *File, char Index); //returns true if w already contains needed register
    int counter; //instructions counter
};

#endif
