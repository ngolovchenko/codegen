/*
 CONSTDIVMUL.CPP

CODE GENERATION FOR CONSTANT DIVISION/MULTIPLICATION IN PIC ASSEMBLY

Idea and project guidance by James Newton <jamesnewton@geocities.com>, <http://techref.massmind.org/codegen>
Used software tricks shared on PICLIST <www.piclist.com> by
  Scott Dattalo <http://www.dattalo.com>, Robert A. LaBudde <http://lcfltd.com>
  James Newton <http://techref.massmind.org>, Dwayne Reid <dwayner@PLANET.EON.NET>
Implementation by Nikolai Golovchenko <http://techref.massmind.org/member/NG--944>

-------------------    2000   -------------------------------------------
Date: Feb 19, 2000 - Mar 16, 2000; Mar 21, 2000;
Mar 30, 2000 (banner added);
July 3:
        1) broke source in several files for maintainability:
            constdivmul.h/.cpp
            reg.h/.cpp
            pic16.h/.cpp
        2) Enhanced 'back to form link'. Now it uses HTTP_REFERER env variable
        3) Added logging feature
July 4:
        1) Removed bug with name length
        2) Command line interface:
        Command line:
            constdivmul.exe {options} {> OutputFile}
        Options:
            Acc=InputRegName (default "ACC")
            Bits=SizeOfInput    (default "8")
            Const=1.2345 (no default)
            ConstErr=0.01 (default 0.5%)
            Temp=TempRegName (default "TEMP")
            endian=big (default "little")
        Example:
            constdivmul.exe const=0.35 bits=11 consterr=1 > result.txt
July 5
        1) Optimized big endian shifts 4 times left
        2) Optimized big endian shifts 4 times right
        3) Optimized little endian shifts 4 times left
        4) Optimized little endian shifts 4 times right
July 10
     Banner removed
July 12
     Fixed bug when called without query string
     from MS IIS 4.0 server
July 14
        1) Embedded the minimal form in code to display it if the external page doesn't exist
        2) Implemented shared access to the log file
September 13
        1) Separated a couple binary functions from constdivmul()
                void constToBinary(double Const, char* alg);
                void optimizeBinary(char* alg);
        2) Replaced literals display to hex ->printf("0x%.2X", literal)
        3) Corrected the registers copying so that the least significant
        byte was copied last. That can improve temps usage...
        Idea by Tony Kubek.
        4) Fixed bug in little endian substraction for case 
        when from a bigger size register a smaller one is substracted.
        5) Fixed bug in little endian right shifts for signed values.
        Sign was being lost.
        6) Fixed bug in conversion to binary! He-he %-)
        7) Added rounding.
September 20
        1)Complement carry implemented with 'incf STATUS,f'. One
        instruction saving, and working reg is not spoiled!
        Original idea by Charles Ader: lib\io\osi2\crc16ca.htm
        2)Restored carry complement because of possible problems with
        overflowing page/bank bits in STATUS.
        However carry complementdoesn't work this way in SX !
        a)  inc STATUS      ;doesn't complement STATUS.C
        b) mov W, #1        ;doesn't complement STATUS.C
            xor STATUS, W   ;
        
        c) clrb Z           ;works!
            incsz STATUS    ;skip never executed
            
November 16
        Added SX support:
            1) new object SX28 - descendant from PIC16, it redefines
            only instructions printing, and changes carry complement
            implementation
            2) new option cpu=pic16 or sx28
            3) changed used regs printing (cblock in PIC16, DS in SX)
            4) changed form in web page file and copied changes
            to the embedded web page
        Other:
            1) code is printed in HTML or text mode, depending on where
            the program is executed from - command line or server. It had
            to be made to print correctly SX instructions containing
            characters such as '<', '>', etc
November 17
        1) Fixed a small bug with copying Acc to Temp. The problem copied
        the number of bytes Acc had after multiplication (copying is made
        afterwards to only used temps). Fixed with caching the initial
        value of Acc object in AccCopy
        2) Fixed a big bug when Acc was shifted before copying before main
        generation loop (Const > 1). Shift operation was printed after copying!
        3) Added code size calculation
        4) Tested
-------------------    2001   -------------------------------------------
January 11, Thu
      Recompiled using G++ compiler in DevC++4.0 IDE for Windows.
      Had to change the requests logging routine logRequest(), the part, which
      opens a file in write denied share mode. _fsopen function
      apparently doesn't exist in G++ includes, so a lower level
      sopen is used and then the descriptor converted to FILE object
      by fdopen.
January 13, Sat
    It works also in VC.
    Fixed logging month number: from 0-11 to 1-12.

*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <time.h>

#ifdef _WIN32
  #include <windows.h>
  #include <io.h>
  #include <share.h>
  #define strcasecmp _stricmp

  char* ScriptName="constdivmul.exe";
#else
  #include <unistd.h>
  #include <fcntl.h>
  #include <errno.h>

  char* ScriptName="constdivmul";
#endif 


#include <string>
using namespace std;

extern "C"
{
  #include "../cgihtml/cgi-lib.h"
  #include "../cgihtml/html-lib.h"
#ifdef _WIN32
  char** CommandLineArguments;
#else
  extern char** CommandLineArguments;
#endif
}

#include "reg.h"
#include "pic16.h"
#include "sx28.h"
#include "constdivmul.h"

double Const;       //Constant value
double ConstErr;  //Constant error percent
char OutBits;   //Bits number in output
char AddBits;   //Bits added to input size in result
char InpBytes;  //Bytes number in input
char OutBytes;  //Bytes number in output
PIC16   *ALU;       //Status register and accumulator object
bool roundResult; //round mode
CPU_Code CPU_Option = CPU_PIC16; //selected processor

#ifdef __EMSCRIPTEN__
const bool isCGI = false;
#else
bool isCGI;
#endif

int main(int argc, char *argv[], char **envp)
{
    llist entries = {NULL}; //clear list
    char Error = 0;
    string AccName="ACC";
    string TmpName="TEMP";
    char AccVal=8;
    endianType endianVal=little;
    Reg* Acc;
    Reg* Temp;

    //make pointer to command line global
    CommandLineArguments = argv;

    /********* DEBUG *****************/
#if 0
    _putenv("SERVER_SOFTWARE=Microsoft-IIS/4.0");
    _putenv("SERVER_NAME=www.piclist.com");
    _putenv("GATEWAY_INTERFACE=CGI/1.1");
    _putenv("SERVER_PROTOCOL=HTTP/1.1");
    _putenv("SERVER_PORT=80");
    _putenv("REQUEST_METHOD=GET");
    _putenv("PATH_TRANSLATED=C:\\Inetpub\\wwwroot");
    _putenv("SCRIPT_NAME=/cgi-bin/test.exe");
    _putenv("REMOTE_HOST=204.210.50.240");
    _putenv("REMOTE_ADDR=204.210.50.240");
    _putenv("CONTENT_LENGTH=0");
    //_putenv("QUERY_STRING=Acc=AD_DataValue&Bits=24&endian=big&Const=0.0390625&ConstErr=0.05&Temp=Sample");
    //_putenv("QUERY_STRING=Acc=AD_DataValue&Bits=8&endian=big&Const=0.3&ConstErr=0.5&Temp=Sample");
    //_putenv("QUERY_STRING=Acc=AD_DataValue&Bits=16&endian=little&Const=0.60725293510314&ConstErr=0.1&Temp=Sample");
    //_putenv("QUERY_STRING=Acc=ACC&Bits=8&endian=little&Const=1.234&ConstErr=0.5&round=yes&Temp=TEMP");
    //_putenv("QUERY_STRING=Acc=ACC&Bits=8&endian=little&Const=1.234&ConstErr=0.5&Temp=TEMP&cpu=pic16");
    //_putenv("QUERY_STRING=Acc=ACC&Bits=2&Const=2.35&ConstErr=1&Temp=TEMP&cpu=pic16");
    _putenv("QUERY_STRING=Acc=ACC&Bits=16&endian=little&Const=0.889&ConstErr=0.5&Temp=TEMP&cpu=sx28");
#endif
    
#ifndef __EMSCRIPTEN__
    //what kind of request
    isCGI = getenv("REQUEST_METHOD") != NULL;
#endif

    // Read form values
    read_cgi_input(&entries);

    if(isCGI)
    {
        //log request
        logRequest();

        //check if form is empty
        if( !is_field_exists(entries, "Const") 
        || !is_field_exists(entries, "ConstErr") 
        || !is_field_exists(entries, "Bits") )
        {
            //CGI form should have query string and these fields
            helpScreen();
            list_clear(&entries);
            return 0;
        }
        // Generate header and title
        html_header();
        html_begin("Code Generator Results");
    }
    else if(argc == 1 || !is_field_exists(entries, "Const"))
    {
        //command line mode and no required parameters
        helpScreen();
        list_clear(&entries);
        return 0;
    }

    if(is_field_exists(entries, "Const"))
    {
        Const = atof(cgi_val(entries, "Const"));
    }
    if(is_field_exists(entries, "Acc"))
    {
        AccName = cgi_val(entries, "Acc");
    }
    if(is_field_exists(entries, "Bits"))
    {
        AccVal = atoi(cgi_val(entries, "Bits"));
    }
    ConstErr = 0.5;
    if(is_field_exists(entries, "ConstErr"))
    {
        ConstErr = atof(cgi_val(entries, "ConstErr"));
    }
    if(is_field_exists(entries, "Temp"))
    {
        TmpName = cgi_val(entries, "Temp");
    }
    if(is_field_exists(entries, "endian"))
    {
        if(cgi_val(entries, "endian")[0] == 'b')
        {
            endianVal = big;
        }
    }
    roundResult = false;
    if(is_field_exists(entries, "round"))
    {
        if(strcasecmp(cgi_val(entries, "round"), "yes") == 0)
        {
            //if constant has required fractional part
            //set round mode and multiply constant by 2

            if(Const != 0
                && (fabs(floor(Const) - Const)/Const*100 > ConstErr))
            {
                roundResult = true;
                //multiply constant by two
                Const = Const * 2;
            }
        }
    }
    if(is_field_exists(entries, "CPU"))
    {
        if(strcasecmp(cgi_val(entries, "CPU"), "PIC16") == 0)
        {
            CPU_Option = CPU_PIC16;
        }
        else
        {
            CPU_Option = CPU_SX28;
        }
    }
    /* Check for range */ 
    
    if((TmpName.size() == 0) || !isalpha(TmpName[0]))
    {
        Error = 4;  //Enter temporary register(s) name
    }
    if((ConstErr < 0) || (ConstErr > 50))
    {
        Error = 6;  //Constant error must be between 0 and 50%
    }
    if(Const <= 0)
    {
        Error = 2;  //Constant value is negative, zero, or error in input 
    }
    else
    {
        AddBits = (char) ceil(log10(Const - 0.5*Const*roundResult) / log10(2));
        OutBits =  AddBits + AccVal;
        
        if((AddBits + 1) > ALGINTSIZE)  /* 1 added because constant
            will be decomposed in series of powers of 2 and the highest
            power may exceed constant value, e.g. 7 = 8 - 1    */
        {
            Error = 5;  //Constant out of range (1..ALGINTSIZE)
        }
        if(OutBits <= 0)
        {
            Error = 7;  //Zero result
        }
    }
    if((AccVal < 1) || (AccVal > MAXINPBITS))
    {
        Error = 1;  //Accumulator size out of range (1..32) 
    }
    if((AccName.size() == 0) || !isalpha(AccName[0]))
    {
        Error = 3;  //Enter accumulator name
    }
    
    /* construct register objects */
    Acc = new Reg(AccName.c_str(), (char)AccVal, endianVal);
    Temp = new Reg(TmpName.c_str(), 0, endianVal);  //0 bits in temporary
    if(Error == 0)
    {
        if(isCGI)
        {
            /* Open preformatted block */ 
            printf("<PRE>\n");
        }
        /* Call code generation */
        constdivmul(Acc, Temp);
        /* Close preformatted block */

        if(isCGI)
        {
            printf("</PRE>\n");
            TimeStamp();
        }
    }
    else
    {
        /* Report error */
        ReportError(Error, Acc, Temp);
        printLinkBack();
    }
    delete(Temp);
    delete(Acc);

    if(isCGI)
    {
        html_end();
    }
    list_clear(&entries);
    return 0;
}

void
printLinkBack()
{
    char* var;
    var = getenv("HTTP_REFERER");
    if(isCGI && var != NULL)
    {
        printf("<HR width=\"100%%\">\n <A HREF=\"%s\">Back to form</A>\n", var);
    }
}

void
ReportError(char Err, Reg* Acc, Reg* Temp)
{
    if(isCGI)
    {
        printf("<H1>");
    }
    else
    {
        //Print form values
        printForm(Acc, Temp);
    }
    printf("#%d Error in input", Err);
    if(isCGI)
    {
        printf("</H1>");
    }
    printf("\n");
    switch(Err)
    {
    case 1:
        printf("Input size out of range (1..%d bits).\n", MAXINPBITS);
        break;
    case 2:
        printf("Constant value is negative, zero, or not a number.\n");
        break;
    case 3:
        printf("Enter input register(s) name.\n");
        break;
    case 4:
        printf("Enter temporary register(s) name.\n");
        break;
    case 5:
        printf("Too big constant, size over %d bits.\n", ALGINTSIZE);
        break;
    case 6:
        printf("Constant error must be between 0 and 50%%.\n");
        break;
    case 7:
        printf("Result is zero for this combination of input size and constant.");
        break;
    default:
        break;
    }
}

void
logRequest()
{
    FILE* log = NULL;
    time_t final;
    time_t nowTime;
    struct tm* now;
    char* buf;
    char* query;

    time(&nowTime);
    final = nowTime + 3; //3 seconds for retries

    while(log == NULL && nowTime < final)
    {
#ifdef _WIN32
      log = _fsopen( "constdivmul_log.csv", "a", _SH_DENYWR);   //create or open file in append mode
      //writing is disabled for other processes

      if(log == NULL)
      {
        // try again in a bit
        Sleep(987);
      }
            
#else
      log = fopen("./constdivmul_log.csv", "a");
      if(log == NULL)
      {
        // check the reason of failure
        if(errno == EACCES)
        {
          // file busy, try again in a bit
          sleep(1);
           
        }
        else
        {
            break;
        }
        
      }
      else
      {
        // lock the file
        flock lock;
        lock.l_type = F_WRLCK;
        lock.l_whence = SEEK_SET;
        lock.l_start = 0;
        lock.l_len = 0; // whole file
        int res = fcntl(fileno(log), F_SETLK, &lock);
        if(res == 0)
          break; // success
        else
          return; // failed to set a lock
      }

    
#endif
      time(&nowTime);
    }


    if(log != NULL)
    {
        //file opened ok

        //if file is empty, create header
        if(fseek(log, 0, SEEK_END))
         return;
  
        long size = ftell(log);

        if(size < 0)
         return;

        if(size == 0)
        {
          fprintf(log, "Date,Time,Address,Host,Client,Method,Query\n");
        }

        //Date and time
        time(&nowTime);
        now = localtime(&nowTime);
        fprintf(log,
            "%d/%d/%d,",
            1900 + now->tm_year,
            now->tm_mon + 1,    //months are counted from 0 here
            now->tm_mday);
        fprintf(log,
            "%d:%d:%d,",
            now->tm_hour,
            now->tm_min,
            now->tm_sec);

        //Address,Client,Method
        logPrint(log, "REMOTE_ADDR");
        logPrint(log, "REMOTE_HOST");
        logPrint(log, "HTTP_USER_AGENT");
        logPrint(log, "REQUEST_METHOD");

        //Query
        query = getenv("QUERY_STRING");
        if(query != NULL)
        {
            buf = new char[strlen(query) + 1];
            strcpy(buf, query);
            unescape_url(buf);  //remove escape sequencies
            fprintf(log, "%s", buf); //without comma at the end of line
            delete[] buf;
        }
        fprintf(log, "\n");
        fclose(log);
    }
}

//print env variable if not empty
void
logPrint(FILE* log, char* envName)
{
    char *var; 
    
    var = getenv(envName);
    if(var != NULL)
    {
        fprintf(log, "%s", var); 
    }
    fprintf(log, ","); //comma separation
}


void
printForm(Reg* Acc, Reg* Temp)
{
    printf("; %s = %s * %g\n", Acc->Name, Acc->Name, Const - 0.5*Const*roundResult);
    printf("; Temp = %s\n", Temp->Name);
    printf("; %s size = %d bits\n", Acc->Name, Acc->GetSize());
    printf("; Error = %g %%\n", ConstErr);
    printf("; Bytes order = %s endian\n", (Acc->endian==big)?"big":"little");
    printf("; Round = %s\n", roundResult? "yes" : "no");
    printf("\n");
}
 
void
constdivmul(Reg *Acc, Reg *Tmp)
{
    double i;
    double accum;
    char j;     //counters
    char k;
    char last;  
    char terms; 
    char shifts;
    char alg[MAXALGSIZE]; //algorithm, contains 0, +1, -1 for each shift
                                 //Point is fixed before index ALGINTSIZE
    char AlgSize;   //real size of algorithm
    int copytotempindex = 0;
    int copytotempend = 0;
    char code[MAXCODESIZE];
    char code2[MAXCODESIZE];
    Reg* AccCopy;
    int codeSize = 0; //code size

    printForm(Acc, Tmp);
    code[0] = '\0';
    code2[0] = '\0';

    //Determine shifts and adds/subs
    constToBinary(Const, alg);

    //Optimize fraction by converting sum of powers of 2 to sum/difference
    optimizeBinary(alg);

    //Show algorithm and choose the size of algorithm
    i = pow(2, ALGINTSIZE - 1);
    printf("; ALGORITHM:\n; Clear accumulator\n");
    terms = 0;
    last = 0;
    accum = Const;
    for(j = 0; j < MAXALGSIZE; j++)
    {

        //check space between shifts
        if(last < (ALGINTSIZE - 1))
        {
            k = ALGINTSIZE - 1;
        }
        else
        {
            k = last;
        }

        if((j - k) >= Acc->GetSize())
        {
            //too much right shifts - algorithm generation can be stopped
            printf("; WARNING:\n");
            printf("; Needed precision can't be reached because the input register size is too small\n");
            j = last;
            break;
        }

        if(alg[j] == 1)
        {
            accum -= i;
            terms++;
            if(i >= 1)
            {
                printf ("; Add input * %.0f to accumulator\n", i);
            }
            else
            {
                printf ("; Add input / %.0f to accumulator\n", 1 / i);
            }

        }
        if(alg[j] == -1)
        {
            accum += i;
            terms++;
            if(i >= 1)
            {
                printf ("; Substract input * %.0f from accumulator\n", i);
            }
            else
            {
                printf ("; Substract input / %.0f from accumulator\n", 1 / i);
            }
        }
        if( fabs(accum) <= (ConstErr / 100 * Const) )
        {
            break;
        }
        i = i / 2;
        if(alg[j] != 0)
        {
            last = j;
        }
    }
    AlgSize = j + 1;
    if(roundResult)
    {
        printf("; Shift accumulator right (LSb to carry)\n");
        printf("; If carry set, increment accumulator\n");
    }
    printf("; Move accumulator to result\n");
    printf(";\n; Approximated constant: %g, Error: %g %%\n",
        (Const-accum) * (roundResult?0.5:1), fabs(accum) / Const * 100);


    /*
    
      Now code can be generated
    
      alg[] contains algorithm


    */

    InpBytes = Acc->GetBytes();
    OutBytes = (OutBits + 7) / 8;
    printf("\n;     Input: %s0", Acc->Name);
    if(InpBytes > 1)
    {
        printf(" .. %s%d", Acc->Name, InpBytes - 1);
    }
    printf(", %d bits", Acc->GetSize());
    printf("\n;    Output: %s0", Acc->Name);
    if(OutBytes > 1)
    {
        printf(" .. %s%d", Acc->Name, OutBytes - 1);
    }
    printf(", %d bits", OutBits);

    //create processor object
    if(CPU_Option == CPU_PIC16)
    {
        ALU = new(PIC16);
    }
    else
    {
        ALU = new(SX28);
    }
        
    
    ///////// registers preparation ////////////
    ALU->clearCounter();
    if(AlgSize < ALGINTSIZE)
    {
        //shift left accumulator if its wegiht is higher then 1
        sprintf(code2 + strlen(code2), "\n;shift accumulator left %d times\n"
            "%s", ALGINTSIZE - AlgSize, Acc->ShiftLeft(ALGINTSIZE - AlgSize));
    }
    codeSize += ALU->getCounter();
    //remember state of Acc before copying
    AccCopy = new Reg(*Acc);
    //if more than 1 term, copy input to temporary
    if(terms != 1)
    {
        Tmp->Copy(Acc); //this operation is made
        //without printing, just to initialize registers
    }
    
    ///// main generation cycle ////////
    ALU->clearCounter();
    for(k = AlgSize - 2; k >= 0; k--)
    {
        j = k + 1;  //j indexes last value in alg[]
        shifts = 1;
        while((alg[k] == 0) && (k >= 0) && (k != (ALGINTSIZE - 1)))
        {
            shifts++;
            k--;
        }
        if(k >= 0)
        {
            if(j >= ALGINTSIZE)
            {
                /* fraction - shift right */
                //Acc->RegT = Static;
                sprintf(code + strlen(code), "\n;shift accumulator right %d times\n"
                    "%s", shifts, Acc->ShiftRight(shifts));
            }
            if(alg[AlgSize - 1] == - 1)
            {
                //don't forget to make initial input negative if needed
                //for integer make negative before shift
                //for fraction '    '       after   "
                alg[AlgSize - 1] = 0;
                sprintf(code + strlen(code), "\n;negate accumulator\n"
                    "%s", Acc->Neg());
                Acc->Sign = true;
            }
            if(j < ALGINTSIZE)
            {
                /* integer - shift left */
                //Acc->RegT = Dynamic;
                sprintf(code + strlen(code), "\n;shift temporary left %d times\n"
                    "%s", shifts, Tmp->ShiftLeft(shifts));
            }
        }
        else
        {
            /* end reached - break away */
            break;
        }
        if(alg[j - shifts] == 1)
        {
            // if 1, add
            sprintf(code + strlen(code), "\n;add temporary to accumulator\n"
                "%s", Acc->Add(Tmp));
            if(Acc->Sign)
            {
                //before addition accumulator was negative
                //after became positive
                //so carry can't be used, because it's set
                ALU->SetValidCarry(false, Acc);
                ALU->SetNoteCarry(false, Acc);
            }
            Acc->Sign = false;
        }
        else
        {
            if(alg[j - shifts] == -1)
            {
                //if -1, substract (on zero do nothing)
                sprintf(code + strlen(code), "\n;substract temporary from accumulator\n"
                    "%s", Acc->Sub(Tmp));
                Acc->Sign = true;   //always substracted bigger number
            }
        }
    }
    //round if needed
    if(roundResult)
    {
        ALU->ClearSnippet();
        sprintf(code + strlen(code), "\n;shift accumulator right once and\n"
            ";increment if carry set\n%s", Acc->Round());
    }
    //take care of carry if needed
    if(OutBits > Acc->GetSize())
    {
        ALU->ClearSnippet();
        sprintf(code + strlen(code), "%s", ALU->TakeCarry());
    }
    codeSize += ALU->getCounter();
    ALU->clearCounter();
    //prepare copying to temps code
    if(terms != 1)
    {
        sprintf(code2 + strlen(code2), "\n;copy accumulator to temporary\n"
            "%s", Tmp->CopyToUsed(AccCopy));
    }
    codeSize += ALU->getCounter();

    /////////////////////////////////////////////////
    /// All code is ready to print at this point ////
    /////////////////////////////////////////////////

    //print the number of used instructions
    printf("\n; Code size: %d instructions\n\n", codeSize);

    //print cblock
    if(ALU->whatCPU() == CPU_PIC16)
    {
        printf("\tcblock\n");
        printf("%s", Acc->ShowUsedRegs());
        printf("%s", Tmp->ShowUsedRegs());
        printf("\tendc\n");
    }
    else
    {
        printf("%s", Acc->ShowUsedRegs());
        printf("%s", Tmp->ShowUsedRegs());
    }

    //print code in text or HTML format
    printCode(code2);
    printCode(code);

    delete(AccCopy);
    delete(ALU);
}


void
TimeStamp()
{
    time_t ltime;
    struct tm *gmt;
    
    /* Display UTC. */
    time(&ltime);
    gmt = gmtime(&ltime);
    printf("; Generated by <A HREF=\"%s\">%s%s</A> (%s version)<BR>\n", 
    	getenv("SCRIPT_NAME"), 
    	getenv("SERVER_NAME"), 
    	getenv("SCRIPT_NAME"), 
    	VERSIONDATE);
    printf("; %s GMT<BR>\n", asctime(gmt));
}

void
helpScreen()
{
    FILE* form;
    char ch;

    if(isCGI)
    {
        html_header();
        //try to open form
        form = fopen("constdivmul.htm", "r");
        if(form == NULL)
        {
            html_begin("Constant Division/Multiplication Code Generator");
            h1("Constant Division/Multiplication Code Generator");
            printf("<!-- Can't open \"constdivmul.htm\" -->\n");
            //print built-in form
            printf("<form action=\"%s\" method=\"get\">\n", getenv("SCRIPT_NAME"));
            printf("<table border=\"2\" cellpadding=\"5\" bgcolor=\"#E0E0E0\">\n");
         printf("<tr><td>   <p>Input/Output register name:<br><input name=\"Acc\" value=\"ACC\"></p></td>\n");
            printf("<td><p>Input register size, bits<br><input name=\"Bits\" value=\"8\" size=\"3\"></p></td>\n");
            printf("<td>Bytes order:<br>\n");
            printf("<input type=\"radio\" name=\"endian\" value=\"big\">big endian<br>\n");
            printf("<input type=\"radio\" name=\"endian\" value=\"little\" checked>\n");
            printf("little endian<br></td></tr>\n");
         printf("<tr><td colspan=\"2\">Multiply by constant:<br><input name=\"Const\" value=\"1.234\">\n");
            printf("<p>Tip: To divide by a number,<br>enter the value of 1/number into the field.</p></td>\n");
            printf("<td>Maximum error<br>in constant approximation, %%<br>\n");
         printf("<a name=\"const\"> <input name=\"ConstErr\" value=\"0.5\" size=\"16\" maxlength=\"15\"></a><p>\n");
        printf("<INPUT name=round type=checkbox value=yes>&nbsp;Round result</p></td></tr>\n");
            printf("<tr><td colspan=\"2\">Temporary register name:<br><input name=\"Temp\" value=\"TEMP\"></td>\n");
            printf("<td>\n");
            printf("Select processor:<br>");
         printf("<input type=\"radio\" name=\"cpu\" value=\"pic16\" checked>PIC");
         printf("<input type=\"radio\" name=\"cpu\" value=\"sx28\">SX");
           printf("<P><A href=\"http://piclist.com/techref/piclist/codegen/constdivmul_help.htm\">Help</a>");
         printf("&nbsp;&nbsp;&nbsp;<input type=\"submit\" value=\"Generate code!\"></p>");
            printf("\n</td></tr>\n");
            printf("</table></form>\n");
            html_end();
        }
        else
        {
            //print external form
            while(!feof(form))
            {
                size_t n = fread(&ch, 1, 1, form);
		(void)n;
                putchar(ch);
            }
            fclose(form);
        }
    }
    else
    {
        printf("--------------------------------------------------------------------\n");
        printf("CODE GENERATION FOR CONSTANT DIVISION/MULTIPLICATION IN PIC ASSEMBLY\n");
        printf("http://www.piclist.com/codegen/\n");
        printf("--------------------------------------------------------------------\n");
        printf("Command line: %s {option=value} {> OutputFile}\n", ScriptName);
        printf("Options:\n\tAcc - input/output register name (default value: ACC)\n");
        printf("\tBits - input register size in bits (default 8)\n");
        printf("\tConst - constant value (no default)\n");
        printf("\tConstErr - tolerated error in error approximation in %% (default 0.5)\n");
        printf("\tTemp - temporary register name (default TEMP)\n");
        printf("\tendian - bytes order, \"little\": zero is lsb, \"big\": zero is msb.\n\t\t (default little)\n");
        printf("\tround=yes/no - rounding (default no)\n");
        printf("\tCPU=PIC16/SX28 - processor selection (default PIC16)\n");
        printf("\nExample:\n");
        printf("  %s const=0.35 bits=11 consterr=1 endian=little > result.txt\n", ScriptName);
    }
}


void
constToBinary(double Const, char* alg)
{
    double i;
    double accum;
    int j;
    //convert to binary fraction

    accum = Const;
    i = pow(2, ALGINTSIZE - 1); //highest power
    for(j = 0; j < MAXALGSIZE; j++, i = i / 2)
    {
        alg[j] = 0;
        if(accum >= i)
        {
            alg[j] = 1;
            accum -= i;
        }
    }
}

void
optimizeBinary(char* alg)
{
    int j;

    //Optimize fraction by converting sum of powers of 2 to sum/difference
    for(j = MAXALGSIZE - 1; j > 1 ; j--)
    {
        if((alg[j] + alg[j - 1] + alg[j - 2]) == 3)
        {
            //at least three consecutive ones -
            //change them to difference (111 = 1000 - 1)
            alg[j] = -1;
            while(--j >= 0)
            {
                if(alg[j] != 0)
                {
                    alg[j] = 0;
                }
                else
                {
                    alg[j++] = 1;   //continue from this bit
                    break;
                }
            }
        }
    }
}

void
printCode(char* code)
{
    int i = 0;
    char ch;

    if(!isCGI)
    {
        printf("%s", code);
    }
    else
    {
        //scan all characters and replace
        //      " quot;
        //      < lt;
        //      > gt;
        //      & amp;
        while((ch = code[i++]) != '\0')
        {
            switch(ch)
            {
            case '<':
                printf("&lt;");
                break;
            case '>':
                printf("&gt;");
                break;
            case '&':
                printf("&amp;");
                break;
            case '\"':
                printf("&quot;");
                break;
            default:
                printf("%.1s", &ch);
                break;
            }
        }
    }
    printf("\n");
}
