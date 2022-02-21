#define TextSize 512	//a practical limit for input text size
#define VERSIONDATE "April 21, 2000"
#define MAXCODESIZE 65536

void
ReportError(char Err);
int
generate(char* Text, bool Insense, bool Whole);
void
TimeStamp();
void
logRequest();
void
logPrint(FILE* log, char* envName);
void
helpScreen();
