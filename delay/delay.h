#ifndef delay_h
#define delay_h

#define VERSIONDATE "December 7, 2005"

#define MAXNUMBEROFLOOPS 8
#define BASICDELAY (picMode ? 10 : 15)  //maximum delay without loops

#undef SHOW_BANNER

void
ReportError(char Err);
void
logRequest();
void
logPrint(FILE* log, char* envName);
void
printForm();
void
TimeStamp();
void
Banner();
void
helpScreen();

void
extractRegs(string& buf);
void 
lowercaseString(string& m);
void
stripBlanks(string& buf);
void
generateDelay();
void
generateDelay2();
double
delayInCycles();
double
delayInCyclesOld();
void
adjustCounters(int* counters, int loops, double cycles);
void
adjustCounters2(int* counters, int loops, double cycles);
double
calculateDelay(int* counters, int loops);
double
calculateDelay2(int* counters, int loops);
double
maximumDelay(int loops);
double
maximumDelay2(int loops);
void
findGradient(double* gradient, int* counters, int loops);
int
findMin(double* gradient, int loops, int offset);
int
findMax(double* gradient, int loops, int offset);
double
generateSubDelay(double cycles, string label);
double
generateSubDelay2(double cycles, string label);
void
printLinkBack();
int
numberOfLoops(double cycles);
int
numberOfLoops2(double cycles);

//SX stuff
void
generateDelaySX();

int
numberOfLoopsSX(double cycles);
double
generateSubDelaySX(double cycles, string label);
double
maximumDelaySX(int loops);
double
calculateDelaySX(int* counters, int loops);
void
adjustCountersSX(int* counters, int loops, double cycles);

#endif

