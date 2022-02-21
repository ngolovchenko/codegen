/* Header file constdivmul.h */
#ifndef constdivmul_h
#define constdivmul_h


#define MAXINPBITS  32  /* Maximum number of bits in input */ 
#define MAXOUTBITS  64  /* Maximum number of bits in output */ 
#define MAXFRCBITS  64  /* Maximum number of bits in fractional part of constant */
#define MAXALGSIZE  (MAXOUTBITS - MAXINPBITS + MAXFRCBITS)
#define ALGINTSIZE  (MAXOUTBITS - MAXINPBITS) /* Size of integral part in algorithm */
#define MAXCODESIZE 65536
#define VERSIONDATE "1-May-2002"

void
constdivmul(Reg* Acc, Reg* Tmp);
void
ReportError(char Err, Reg* Acc, Reg* Temp);
void
TimeStamp();
void
logRequest();
void
logPrint(FILE* log, char* envName);
void
printForm(Reg* Acc, Reg* Temp);
void
helpScreen();
void
printLinkBack();
void
constToBinary(double Const, char* alg);
void
optimizeBinary(char* alg);
void
printCode(char* code);

#endif
