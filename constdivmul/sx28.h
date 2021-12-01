#ifndef sx28_h
#define sx28_h

#include "pic16.h"

class SX28 : public PIC16
{
public:
    SX28();
    virtual ~SX28();
    char *movwf(Reg *File, char Index);
    char *movfw(Reg *File, char Index);
    char *swapfw(Reg *File, char Index);
    char *swapff(Reg *File, char Index);
    char *movlw(char unsigned literal);
    char *andlw(char unsigned literal);
    char *clrf(Reg *File, char Index);
    char *clrw();
    char *btfsc(Reg *File, char Index, char Bit);
    char *btfss(Reg *File, char Index, char Bit);
    char *bsf(Reg *File, char Index, char Bit);
    char *comff(Reg *File, char Index);
    char *clrc();
    char *com_msb(Reg *File, char Index);
    char *skpc();
    char *skpnc();
    char *skpnz();
    char *rlff(Reg *File, char Index);
    char *rlfw(Reg *File, char Index);
    char *rrff(Reg *File, char Index);
    char *rrfw(Reg *File, char Index);
    char *addwff(Reg *File, char Index);
    char *subwff(Reg *File, char Index);
    char *iorwff(Reg *File, char Index);
    char *iorlw(char unsigned literal);
    char *incfszw(Reg *File, char Index);
    char *incff(Reg *File, char Index);
    char *decff(Reg *File, char Index);
    char *xorwff(Reg *File, char Index);
    char *jmp(char* x);
    CPU_Code whatCPU();
protected:
private:

};

#endif

