//***********************************************
// Delay Code Generator
// for PIC microcontrollers
//
// 7-July-2000 by Nikolai Golovchenko
//
// 10-July
//     Banner removed
// 11-July
//     Fixed bug when called without query string
//     from MS IIS 4.0 server
// 14-July
//     1) Embedded the minimal form in code to display it if the external page doesn't exist
//     2) Implemented shared access to the log file
// 27-December-2001
//     1) Added another delay generation method, which is more efficient and
//     simpler to calculate. For example
/*
            ;499994 cycles
    movlw   0x03
    movwf   d1
    movlw   0x18
    movwf   d2
    movlw   0x02
    movwf   d3
Delay_0
    decfsz  d1, f
    goto    $+2
    decfsz  d2, f
    goto    $+2
    decfsz  d3, f
    goto    Delay_0
*/
//      2) Small improvements in large numbers printing
// 1-January-2002
//      Adding SX version
// 7-December-2005
//      Fixed the SX delay formula when using loops.
//      Removed the older PIC version of the delay.
//      A couple adjustments to make it compile in Visual C++ 6.0.

//***********************************************

//Standard includes
#include <math.h>
#include <time.h>
#include <stdio.h>
#include <string.h>

#ifdef WIN32
  #include <io.h>
  #include <share.h>
  char* ScriptName = "delay.exe";
  #include <windows.h>
  #pragma warning(disable:4786)
#else
  #include <unistd.h>
  #include <fcntl.h>
  #include <errno.h>
  char* ScriptName = "delay";
#endif

#include <string>
#include <list>
using namespace std;
typedef list<string> LISTSTR;

//Custom includes
extern "C"
{
  #include "../cgihtml/cgi-lib.h"
  #include "../cgihtml/html-lib.h"
#ifdef WIN32
  char** CommandLineArguments;
#else
  extern char** CommandLineArguments;
#endif
}

#include "delay.h"

bool isCGI;

double Delay = -1;
string RegName = "d"; //prefix. If temporary registers are specified, prefix is the first name
LISTSTR Regs;   //list of specified or used register names
//(if not enough list is expanded using RegName prefix)
double Clock = 4; //Clock frequency in MHz
bool Routine = false; //generate as a routine
int RoutineAddedDelay = 4; //call & return time
bool inSeconds = true; //seconds or cycles
string Name = "Delay"; //name of routine
bool picMode = true; //true for PIC and false for SX


int main(int argc, char *argv[], char **envp)
{
    llist entries = {NULL}; //clear list!!!
    char Error = 0;
    string buf;

    //make pointer to command line global
    CommandLineArguments = argv;

    /********* DEBUG *****************
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
    //_putenv("QUERY_STRING=Delay=0.5&Type=seconds&Regs=d1+d2+d3+d4&clock=4&name=Delay&CPU=SX");
    //_putenv("QUERY_STRING=Delay=1000&Type=cycles&Regs=d1+d2+d3+d4&clock=4&name=Delay&CPU=SX");
    _putenv("QUERY_STRING=Delay=458500&Type=cycles&Regs=d1+d2+d3+d4&clock=4&name=Delay&CPU=SX");
    /***********************************/

    //what kind of request
    isCGI = false;
    if(getenv("REQUEST_METHOD") != NULL)
    {
        isCGI = true;
    }

    /* Read form values */ 
    read_cgi_input(&entries);

    //Check for very bad input
    if(isCGI)
    {
        //log request
        logRequest();
        //check if form is empty
        if( !is_field_exists(entries, "Delay") 
        || !is_field_exists(entries, "Type") 
        || !is_field_exists(entries, "Clock") )
        {
            //CGI form should have query string and these fields
            helpScreen();
            list_clear(&entries);
            exit(0);
        }
        // Generate header and title
        html_header();
        html_begin("Code Generator Results");
    }
    else if(argc == 1 || !is_field_exists(entries, "Delay"))
    {
        //command line mode and no required parameters
        helpScreen();
        list_clear(&entries);
        exit(0);
    }

    //read the form fields / options
    if(is_field_exists(entries, "Delay"))
    {
        Delay = atof(cgi_val(entries, "Delay"));
    }
    if(Delay <= 0)
    {
        ReportError(Error = 2);
    }
    if(is_field_exists(entries, "Type"))
    {
        buf = cgi_val(entries, "Type");
        lowercaseString(buf);
        if(buf == "cycles")
        {
            inSeconds = false;
        }
        else if(buf != "seconds")
        {
            ReportError(Error = 1);
        }
    }

    buf = "";
    if(is_field_exists(entries, "Regs"))
    {
        buf = cgi_val(entries, "Regs");
    }
    //extract names to list
    extractRegs(buf, Regs, RegName);

    if(is_field_exists(entries, "Clock"))
    {
        Clock = atof(cgi_val(entries, "Clock"));
        if(Clock <= 0)
        {
            ReportError(Error = 3);
        }
    }
    if(is_field_exists(entries, "Routine"))
    {
        buf = cgi_val(entries, "Routine");
        lowercaseString(buf);
        if(buf == "yes")
        {
            Routine = true;
        }
    }
    if(is_field_exists(entries, "Name"))
    {
        Name = cgi_val(entries, "Name");
    }
    
    if(is_field_exists(entries, "CPU"))
    {
        if(strcmp(cgi_val(entries, "CPU"), "SX") == 0)
        {
            picMode = false;
        }
    }

    //convert delay to cycles
    if(Clock > 0 && Delay > 0)
    {
        double x = delayInCycles();
        RoutineAddedDelay = picMode ? 4 : 6;
        if(x < 1)
        {
            ReportError(Error = 4);
        }
        else if(x + 1 == x)
        {
            //double can hold this number with the resolution of 1
            ReportError(Error = 5);
        }
        else if(Routine && x < RoutineAddedDelay)
        {
            //Delay too small
            ReportError(Error = 6);
        }
    }

    if(Error == 0)
    {
        /* Open preformatted block */ 
        if(isCGI)
        {
            printf("<PRE>\n");
        }
        /* Call code generation */
        if(picMode)
        {
            generateDelay2();
            /*printf("\n\n---------------------------------\n\n");
            printf("Previous version (July 8, 2000)\n\n");
            printf("---------------------------------\n\n");
            generateDelay();*/
        }
        else
        {
            generateDelaySX();
        }
        /* Close preformatted block */
        if(isCGI)
        {
            printf("</PRE>\n");
            TimeStamp();
            Banner();
        }
    }
    else
    {
        printLinkBack();
    }

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
ReportError(char Err)
{
    //char *var; 
    if(isCGI)
    {
        printf("<H2>");
    }
/*  else
    {
        //Print form values
        printForm();
    }*/
    printf("Error %d: ", Err);
    switch(Err)
    {
    case 1:
        printf("Wrong type of delay. Enter 'seconds' or 'cycles'.");
        break;
    case 2:
        printf("Delay is zero or negative.");
        break;
    case 3:
        printf("Clock frequency is zero or negative.");
        break;
    case 4:
        printf("Delay is less than one instruction cycle.");
        break;
    case 5:
        printf("Number of cycles is too big.");
        break;
    case 6:
        printf("Delay is too small to generate it as a routine.");
        break;
    case 8:
        break;
    default:
        break;
    }
    if(isCGI)
    {
        printf("</H2>");
    }
    printf("\n");
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
#ifdef WIN32
      log = _fsopen( "delay_log.csv", "a", _SH_DENYWR);   //create or open file in append mode
      //writing is disabled for other processes

      if(log == NULL)
      {
        // try again in a bit
        Sleep(987);
      }
            
#else
      log = fopen("./delay_log.csv", "a");
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

        //Address,Host,Client,Method
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
            delete(buf);
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
printForm()
{
    if(inSeconds)
    {
        printf("; Delay = %g seconds\n", Delay);
    }
    else
    {
        printf("; Delay = %.0f instruction cycles\n", Delay);
    }

    printf("; Clock frequency = %g MHz\n", Clock);
    printf("\n");
}
 
void
TimeStamp()
{
    time_t ltime;
    struct tm *gmt;
    
    /* Display UTC. */
    time(&ltime);
    gmt = gmtime(&ltime);
    printf("; Generated by <A HREF=\"http://%s%s\">http://%s%s</A> (%s version)<BR>\n", 
        getenv("SERVER_NAME"), getenv("SCRIPT_NAME"),
        getenv("SERVER_NAME"), getenv("SCRIPT_NAME"),
        VERSIONDATE);
    printf("; %s GMT<BR>\n", asctime(gmt));

    /* See also */
    if(picMode)
    {
        printf("<p>; See also various delay routines at\n"
        "<A HREF=\"http://www.piclist.com/techref/microchip/delays.htm\">\n"
        "http://www.piclist.com/techref/microchip/delays.htm</A>\n");
    }
    else
    {
        printf("<p>; See also various delay routines at\n"
        "<A HREF=\"http://www.sxlist.com/techref/ubicom/lib/flow/delays_sx.htm\">\n"
        "http://www.sxlist.com/techref/ubicom/lib/flow/delays_sx.htm</A>\n");
    }
}

void Banner()
{
#ifdef SHOW_BANNER
    printf("<TABLE ALIGN=\"CENTER\"  BORDER=\"1\"  CELLPADDING=\"8\">\n");
    printf("    <TR>\n");
    printf("        <TD WIDTH=90%s BGCOLOR=\"#FFFF00\" >\n", "%");
    printf("        <FONT COLOR=\"#000080\"><CENTER>\n");
    printf("I hope my little code generator helps you. Would you like to help \n");
    printf("me find a job in the U.S.?<BR>\n");
    printf("<A HREF=\"http://techref.massmind.org/member/NG--944\">Nikolai Golovchenko</A>\n");
    printf("        </CENTER></FONT></TD>\n");
    printf("    </TR>\n");
    printf("</TABLE>\n");
#endif
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
        form = fopen("delay.htm", "r");
        if(form == NULL)
        {
            html_begin("Delay Code Generator");
            printf("<!-- Can't open \"delay.htm\" -->\n");
            //print built-in form
            h1("Delay Code Generator");
            printf("<form action=\"%s\" method=\"get\">", getenv("SCRIPT_NAME"));
            printf("<table border=\"2\" cellpadding=\"10\" bgcolor=\"#e0e0e0\">");
         printf("<tr><td><p>Delay</p><p><input name=\"Delay\" value=\"0.5\"></p>");
            printf("<p><input name=\"Type\" type=\"radio\" value=\"cycles\">");
            printf("Instruction cycles</p>");
            printf("<p><input name=\"Type\" type=\"radio\" value=\"seconds\" checked>Seconds</p></td>");
            printf("<td><p>Temporary registers names</p>");
            printf("<p><input name='Regs' value=\"d1 d2 d3 d4\"></p>");
            printf("<p>Clock frequency</p>");
            printf("<p><input name=\"clock\" value=\"4\">MHz</p></td></tr>");
            printf("<tr><td colspan=\"2\">");
            printf("<input type=\"checkbox\" name=\"routine\" value=\"yes\">Generate ");
            printf("routine&nbsp;&nbsp;&nbsp; <input name=\"name\" value=\"Delay\"></td></tr>");
            // cpu
            printf("<tr><td colspan=\"2\">");
            printf("Select CPU:&nbsp;&nbsp;&nbsp; <input name=\"CPU\" type=\"radio\" value=\"PIC\" checked>PIC&nbsp;&nbsp;&nbsp;\n");
            printf("<input name=\"CPU\" type=\"radio\" value=\"SX\">SX</td></tr>\n");
            // submit
            printf("<tr><td colspan=\"2\"><div align=\"center\">\n");
            printf("<input type=\"submit\" value=\"Generate code!\">&nbsp;</div></td></tr></table>");
            printf("</form>");
            html_end();
        }
        else
        {
            //print external form
            while(!feof(form))
            {
                size_t n = fread(&ch, 1, 1, form);
                (void) n;
                putchar(ch);
            }
            fclose(form);
        }
    }
    else
    {
        printf("--------------------------------------------------------------------\n");
        printf("DELAY CODE GENERATION IN PIC/SX ASSEMBLY\n");
        printf("http://www.piclist.com/codegen/\n");
        printf("--------------------------------------------------------------------\n");
        printf("Command line: %s {option=value} {> OutputFile}\n", ScriptName);
        printf("Options:\n\tDelay - value of delay in cycles or seconds (no default)\n");
        printf("\tType - type of delay - cycles/seconds (default: seconds )\n");
        printf("\tRegs - temporary registers, separated by comma (default: d1, d2, ...)\n");
        printf("\tClock - clock frequency in MHz (default: 4)\n");
        printf("\tRoutine - Generate as a routine - yes/no  (default: no)\n");
        printf("\tName - Routine name (default: Delay)\n");
        printf("\tCPU - Select PIC or SX (default: PIC)\n");
        printf("\nNote:\n");
        printf("  If a value contains spaces, insert quotes around the value,\n  e.g. regs=\"r1 r2 r3\"\n");
        printf("\nExample:\n");
        printf("  %s delay=3600 type=seconds clock=20 > result.asm\n", ScriptName);
    }

}

void
extractRegs(string& buf, LISTSTR& Regs, string& newPrefix)
{
    char x;
    LISTSTR::iterator i;
    bool empty=true;

    //collect all names in the list Regs
    while((x = buf.find_first_of(" ,")) != -1) //find first space or comma
    {
        Regs.push_back(buf.substr(0, x));
        stripBlanks(Regs.front());
        if(Regs.back().empty())
        {
            Regs.pop_back();
        }
        buf.erase(0, x + 1);
    }
    Regs.push_back(buf);
    stripBlanks(Regs.front());
    if(Regs.back().empty())
    {
        Regs.pop_back();
    }
    //change prefix
    if(!Regs.empty() && !Regs.front().empty())
    {
        newPrefix = Regs.front() + "_";
    }
    //fill all other regs names
    for(x = Regs.size() + '0'; x < MAXNUMBEROFLOOPS + '0'; x++)
    {
        Regs.push_back(newPrefix + x);
    }

    /*i = Regs.begin();
    while(i != Regs.end())
    {
        printf("%s\n", (*i).c_str());
        i++;
    }   */
}

void 
lowercaseString(string& m)
{
    int x = 0;
    while(x < m.size())
    {
        m.at(x) = tolower(m.at(x));
        x++;
    }
}

void
stripBlanks(string& buf)
{
    while(!buf.empty() && isspace(buf.at(0)))
    {
        buf.erase(0, 1);
    }
    while(!buf.empty() && isspace(buf.at(buf.size() - 1)))
    {
        buf.erase(buf.size() - 1, 1);
    }
}

double
delayInCycles()
{
    if(inSeconds)
    {
        if(picMode)
        {
            return(floor(Delay * Clock * 0.25e6 + 0.5));    //Clock * 0.25e6 = instruction cycle frequency, Hz
        }
        else
        {
            return(floor(Delay * Clock * 1e6 + 0.5));   //Clock * 1e6 = instruction cycle frequency, Hz
        }
    }
    else
    {
        return(floor(Delay + 0.5));
    }
}

double
delayInCyclesOld()
{
    if(inSeconds)
    {
        if(picMode)
        {
            return(Delay * Clock * 0.25e6); //Clock * 0.25e6 = instruction cycle frequency, Hz
        }
        else
        {
            return(Delay * Clock * 1e6);    //Clock * 1e6 = instruction cycle frequency, Hz
        }
    }
    else
    {
        return(Delay);
    }
}

int
numberOfLoops(double cycles)
{
    int x = 1;

    if(cycles <= BASICDELAY)
    {
        return(0);
    }
    while((maximumDelay(x) + BASICDELAY) < cycles && x < MAXNUMBEROFLOOPS)
    {
        //printf("loops=%d max=%f\n", x, maximumDelay(x));
        x++;
    }
    return(x);
}

double
maximumDelay(int loops)
{
    int counters[MAXNUMBEROFLOOPS];
    int x;
    for(x = 0; x < loops; x++)
    {
        counters[x] = 256;
    }
    return(calculateDelay(counters, loops));
}

void
adjustCounters(int* counters, int loops, double cycles)
{
    int tries = 0;
    double delay;
    int x = 0;
    int offset = 0;
    double gradient[MAXNUMBEROFLOOPS];
    bool large = true;

    while(tries++ <  (256 * loops)) //256 tries for each counter
    {
        delay = calculateDelay(counters, loops);
        findGradient(gradient, counters, loops);    //find gradient
        if(large && (cycles * 0.8) < delay && cycles > delay)
        {
            //switch to small steps
            large = false;
            counters[x]--;
            offset = 0;
            delay = calculateDelay(counters, loops);
            findGradient(gradient, counters, loops);    //find gradient
        }
        if(cycles >= delay)
        {
            if(large)
            {
                //large steps
                x = findMax(gradient, loops, offset);
            }
            else
            {
                //small steps
                x = findMin(gradient, loops, offset);
            }
            counters[x]++;
            if(counters[x] > 256)
            {
                counters[x] = 256;
                if(++offset >= loops)
                {
                    offset = 0;
                }
            }
            else
            {
                offset = 0;
            }
        }
        else
        {
            counters[x]--;
            break;
        }
    }
}

double
calculateDelay(int* counters, int loops)
{
    double x;
    double y;
    int i;
    int j;  
    //calculate:
    //N1 = 3*d1 + 1
    //N2 = 3*d1*d2 + 4*d1 + 1
    //N3 = 3*d1*d2*d3 + 4*d1*d2 + 4*d1 + 1
    //...
    x = 0;
    for(i = 0; i < loops; i++)
    {
        y = 1;
        for(j = i - 1; j >= 0; j--)
        {
            y *= counters[j];
        }
        x += y * (3 * counters[i] + 1);
    }
    return(x);
}

void
findGradient(double* gradient, int* counters, int loops)
{
    double x;
    double y;
    int i;
    int j;  
    int k;
    bool zero;

    for(k = 0; k < loops; k++)
    {
        //function:
        //N1 = 3*d1 + 1
        //N2 = 3*d1*d2 + 4*d1 + 1
        //N3 = 3*d1*d2*d3 + 4*d1*d2 + 4*d1 + 1
        //...
        x = 0;
        for(i = 0; i < loops; i++)
        {
            y = 1;
            zero = true;
            for(j = i - 1; j >= 0; j--)
            {
                if(j != k)
                {
                    y *= counters[j];
                }
                else
                {
                    zero = false;
                }
            }
            if(zero && i != k)
            {
                y = 0;
            }
            if(i != k)
            {
                x += y * (3 * counters[i] + 1);
            }
            else
            {
                x += 4 * y;
            }
        }
        gradient[k] = x - 1;
    }
}

int
findMin(double* gradient, int loops, int offset)
{
    int ordered[MAXNUMBEROFLOOPS];
    bool passed[MAXNUMBEROFLOOPS];
    int x;
    int y;
    double min;
    int m;

    for(x = 0; x < loops; x++)
    {
        passed[x] = false;
    }

    for(x = 0; x < loops; x++)
    {
        min = maximumDelay(MAXNUMBEROFLOOPS) + 1;
        m = 0;
        for(y = 0; y < loops; y++)
        {
            if(!passed[y] && gradient[y] <= min)
            {
                min = gradient[y];
                m = y;
            }
        }
        ordered[x] = m;
        passed[m] = true;
    }
    return(ordered[offset]);
}

int
findMax(double* gradient, int loops, int offset)
{
    int ordered[MAXNUMBEROFLOOPS];
    bool passed[MAXNUMBEROFLOOPS];
    int x;
    int y;
    double max;
    int m;

    for(x = 0; x < loops; x++)
    {
        passed[x] = false;
    }

    for(x = 0; x < loops; x++)
    {
        max = 0;
        m = 0;
        for(y = 0; y < loops; y++)
        {
            if(!passed[y] && gradient[y] >= max)
            {
                max = gradient[y];
                m = y;
            }
        }
        ordered[x] = m;
        passed[m] = true;
    }
    return(ordered[offset]);
}

void
generateDelay()
{
    double cycles;
    int loops;
    int x;
    char j = '0';
    LISTSTR::iterator i;
    
    printForm();
    //convert to cycles
    cycles = delayInCycles();
    printf("; Actual delay = %.12g seconds = %.0f cycles\n; Error = %.12g %%\n",
        cycles / Clock / 0.25e6,
        cycles,
        (delayInCyclesOld() - cycles) / delayInCyclesOld() * 100);
    //substract cycles for call+return
    if(Routine)
    {
        cycles -= RoutineAddedDelay;
    }
    //print cblock
    i = Regs.begin();
    loops = numberOfLoops(cycles);
    if(loops > 0)
    {
        printf("\n\tcblock\n");
        for(x = 0; x < loops; x++)
        {
            printf("\t%s\n", (*i).c_str());
            i++;
        }
        printf("\tendc\n");
    }

    // print routine name
    if(Routine)
    {
        printf("\n%s", Name.c_str());
    }

    while(cycles != 0)
    {
        cycles -= generateSubDelay(cycles, Name + "_" + j);
        j++;
    }
    if(Routine)
    {
        printf("\n\t\t\t;%d cycles (including call)\n\treturn\n", RoutineAddedDelay);
    }
}

double
generateSubDelay(double cycles, string label)
{
    int loops;
    int counters[MAXNUMBEROFLOOPS];
    int x;
    double d;
    LISTSTR::iterator i;

    //find the number of nested loops
    loops = numberOfLoops(cycles);
    if(loops != 0)
    {
        //Initialize counters
        for(x = 0; x < loops; x++)
        {
            counters[x] = 1;
        }
        //adjust coefficients
        adjustCounters(counters, loops, cycles);
        d = calculateDelay(counters, loops);

        printf("\n\t\t\t;%.0f cycles\n", d);
        i = Regs.begin();
        for(x = 0; x < loops; x++)
        {
            printf("\tmovlw\t%#.2x\n", counters[x] & 0xFF);
            printf("\tmovwf\t%s\n", (*i).c_str());
            printf("%s%d\n", label.c_str(), x);
            i++;
        }
        for(x = loops - 1; x >= 0; x--)
        {
            i--;
            printf("\tdecfsz\t%s, f\n", (*i).c_str());
            printf("\tgoto\t%s%d\n", label.c_str(), x);
        }
        return(d);
    }
    else
    {
        printf("\n\t\t\t;%.0f cycle%s\n", cycles, cycles > 1 ? "s" : "");
        x = cycles;
        while(x != 0)
        {
            if(x >= 2)
            {
                printf("\tgoto\t$+1\n");
                x -= 2;
            }
            else
            {
                printf("\tnop\n");
                x -= 1;
            }
        }
        return(cycles);
    }
}




void
generateDelay2()
{
    double cycles;
    int loops;
    int x;
    char j = '0';
    LISTSTR::iterator i;
    
    printForm();
    //convert to cycles
    cycles = delayInCycles();
    printf("; Actual delay = %.12g seconds = %.0f cycles\n; Error = %.12g %%\n",
        cycles / Clock / 0.25e6,
        cycles,
        (delayInCyclesOld() - cycles) / delayInCyclesOld() * 100);
    //substract cycles for call+return
    if(Routine)
    {
        cycles -= RoutineAddedDelay;
    }
    //print cblock
    i = Regs.begin();
    loops = numberOfLoops2(cycles);
    if(loops > 0)
    {
        printf("\n\tcblock\n");
        for(x = 0; x < loops; x++)
        {
            printf("\t%s\n", (*i).c_str());
            i++;
        }
        printf("\tendc\n");
    }

    // print routine name
    if(Routine)
    {
        printf("\n%s", Name.c_str());
    }

    while(cycles != 0)
    {
        cycles -= generateSubDelay2(cycles, Name + "_" + j);
        j++;
    }
    if(Routine)
    {
        printf("\n\t\t\t;%d cycles (including call)\n\treturn\n", RoutineAddedDelay);
    }
}

int
numberOfLoops2(double cycles)
{
    int x = 1;

    if(cycles <= BASICDELAY)
    {
        return(0);
    }
    while((maximumDelay2(x) + BASICDELAY) < cycles && x < MAXNUMBEROFLOOPS)
    {
        //printf("loops=%d max=%f\n", x, maximumDelay2(x));
        x++;
    }
    return(x);
}

double
maximumDelay2(int loops)
{
    int counters[MAXNUMBEROFLOOPS];
    int x;
    for(x = 0; x < loops; x++)
    {
        counters[x] = 256;
    }
    return(calculateDelay2(counters, loops));
}

double
calculateDelay2(int* counters, int loops)
{
    double x;
    int i;
    //calculate:
    //N1 = 1+3*d1
    //N2 = 3+5*(d1+256*(d2-1))
    //N3 = 5+7*(d1+256*(d2-1+256*(d3-1)))
    //...
    //or,
    //N1 = 4+3*(d1-1)
    //N2 = 8+5*(d1-1+256*(d2-1))
    //N3 = 12+7*(d1-1+256*(d2-1+256*(d3-1)))
    //...
    //since dx = 1..256, then we can represent the delay like this:
    //Nn = 4*n+(1+2*n)*D, where D=d1-1+256*(d2-1+256*(d3-1))+... (multibyte counter)

    //find D
    x = 0;
    for(i = loops-1; i >= 0; i--)
    {
        x *= 256;
        x += counters[i] - 1;
    }

    //find Nn
    x = 4*loops+(1+2*loops)*x;

    return(x);
}

double
generateSubDelay2(double cycles, string label)
{
    int loops;
    int counters[MAXNUMBEROFLOOPS];
    int x;
    double d;
    LISTSTR::iterator i;

    //find the number of nested loops
    loops = numberOfLoops2(cycles);
    if(loops != 0)
    {
        //adjust coefficients
        adjustCounters2(counters, loops, cycles);
        d = calculateDelay2(counters, loops);

        printf("\n\t\t\t;%.0f cycles\n", d);
        i = Regs.begin();
        for(x = 0; x < loops; x++)
        {
            printf("\tmovlw\t0x%.2X\n", counters[x] & 0xFF);
            printf("\tmovwf\t%s\n", (*i).c_str());
            if(x == (loops - 1))
            {
                printf("%s\n", label.c_str());
            }
            i++;
        }
        i = Regs.begin();
        for(x = 0; x < loops; x++)
        {
            printf("\tdecfsz\t%s, f\n", (*i).c_str());
            if(x < (loops-1))
            {
                printf("\tgoto\t$+2\n");
            }
            else
            {
                printf("\tgoto\t%s\n", label.c_str());
            }
            i++;
        }
        return(d);
    }
    else
    {
        printf("\n\t\t\t;%.0f cycle%s\n", cycles, cycles > 1 ? "s" : "");
        x = cycles;
        while(x != 0)
        {
            if(x >= 2)
            {
                printf("\tgoto\t$+1\n");
                x -= 2;
            }
            else
            {
                printf("\tnop\n");
                x -= 1;
            }
        }
        return(cycles);
    }
}


void
adjustCounters2(int* counters, int loops, double cycles)
{
    double big_counter;
    double new_counter;
    int x = 0;

    //find values for counters

    //Nn = 4*n+(1+2*n)*D, where D=d1-1+256*(d2-1+256*(d3-1))+... (multibyte counter)
    //D=floor((Nn-4*n)/(1+2*n))

    big_counter = floor((cycles - 4 * loops) / (1 + 2 * loops));

    //if we don't have enough counters, which can happen, if the total
    //delay consists of looped and non-looped delays, write 
    //maximum value to counters

    if(big_counter >= pow(256, loops))
    {
        big_counter = pow(256, loops) - 1;
    }

    //load counters
    for(x = 0; x < loops; x++)
    {
        new_counter = floor(big_counter / 256);
        counters[x] = (big_counter - 256 * new_counter + 1);
        //counters[x] &= 0xFF;
        big_counter = new_counter;
    }


}


//**************************************************************************
//
//              SX VERSION
//
//**************************************************************************

void
generateDelaySX()
{
    double cycles;
    int loops;
    int x;
    char j = '0';
    LISTSTR::iterator i;
    
    printForm();
    //convert to cycles
    cycles = delayInCycles();
    printf("; Actual delay = %.12g seconds = %.0f cycles\n; Error = %.12g %%\n",
        cycles / Clock / 1e6,
        cycles,
        (delayInCyclesOld() - cycles) / delayInCyclesOld() * 100);
    //substract cycles for call+return
    if(Routine)
    {
        cycles -= RoutineAddedDelay;
    }
    //print used registers
    i = Regs.begin();
    loops = numberOfLoopsSX(cycles);
    if(loops > 0)
    {
        printf("\n");
        for(x = 0; x < loops; x++)
        {
            printf("%s\tDS\t1\n", (*i).c_str());
            i++;
        }
    }
    // print routine name
    if(Routine)
    {
        printf("\n%s:", Name.c_str());
    }

    while(cycles != 0)
    {
        cycles -= generateSubDelaySX(cycles, Name + "_" + j);
        j++;
    }
    if(Routine)
    {
        printf("\n\t\t\t;%d cycles (including call)\n\tret\n", RoutineAddedDelay);
    }
}


int
numberOfLoopsSX(double cycles)
{
    int x = 1;

    if(cycles <= BASICDELAY)
    {
        return(0);
    }
    while((maximumDelaySX(x) + BASICDELAY) < cycles && x < MAXNUMBEROFLOOPS)
    {
        //printf("loops=%d max=%f\n", x, maximumDelaySX(x));
        x++;
    }
    return(x);
}

double
maximumDelaySX(int loops)
{
    int counters[MAXNUMBEROFLOOPS];
    int x;
    for(x = 0; x < loops; x++)
    {
        counters[x] = 256;
    }
    return(calculateDelaySX(counters, loops));
}

double
calculateDelaySX(int* counters, int loops)
{
    double x;
    double d;
    int i;
    //calculate:
    //
    //  Nn = step*D-floor(D/256)-floor(D/256^2)...-floor(D/256^loops)+min,
    //
    //where 
    // n - number of loops
    // D=d1-1+256*(d2-1+256*(d3-1))+... (multibyte counter)
    // step = 3*n+1
    // min = 4*n
    //

    //find D
    x = 0;
    for(i = loops-1; i >= 0; i--)
    {
        x *= 256;
        x += counters[i] - 1;
    }

    //find Nn
    d = (3*loops+1)*x + 4*loops;
    while(x = floor(x / 256))
        d -= x;

    //printf("%.0f  ", x);

    return(d);
}


double
generateSubDelaySX(double cycles, string label)
{
    int loops;
    int counters[MAXNUMBEROFLOOPS];
    int x;
    double d;
    LISTSTR::iterator i;

    //find the number of nested loops
    loops = numberOfLoopsSX(cycles);
    if(loops != 0)
    {
        //adjust coefficients
        adjustCountersSX(counters, loops, cycles);
        d = calculateDelaySX(counters, loops);

        printf("\n\t\t\t;%.0f cycles\n", d);
        i = Regs.begin();
        for(x = 0; x < loops; x++)
        {
            printf("\tmov\tw, #$%.2X\n", counters[x] & 0xFF);
            printf("\tmov\t%s, w\n", (*i).c_str());
            if(x == (loops - 1))
            {
                printf("%s:\n", label.c_str());
            }
            i++;
        }
        i = Regs.begin();
        for(x = 0; x < loops; x++)
        {
            printf("\tdecsz\t%s\n", (*i).c_str());
            if(x < (loops-1))
            {
                printf("\tjmp\t$+2\n");
            }
            else
            {
                printf("\tjmp\t%s\n", label.c_str());
            }
            i++;
        }
        return(d);
    }
    else
    {
        printf("\n\t\t\t;%.0f cycle%s\n", cycles, cycles > 1 ? "s" : "");
        x = cycles;
        while(x != 0)
        {
            if(x >= 3)
            {
                printf("\tjmp\t$+1\n");
                x -= 3;
            }
            else
            {
                printf("\tnop\n");
                x -= 1;
            }
        }
        return(cycles);
    }
}


void
adjustCountersSX(int* counters, int loops, double cycles)
{
    int x = 0;
    double weight;

    //find values for counters

    //  Nn = step*D-floor(D/256)-floor(D/256^2)...-floor(D/256^loops)+min,
    //
    //where 
    // n - number of loops
    // D=d1-1+256*(d2-1+256*(d3-1))+... (multibyte counter)
    // step = 3*n+1
    // min = 4*n

    // Another way to calculate the delay is this:
    //
    // Nn = (d1-1)*step
    //    + (d2-1)*(256*step-1)+
    //    + (d3-1)*(256^2*step-256-1)+
    //    + (d4-1)*(256^3*step-256^2-256-1)+ ...
    //    + min.
    //
    // We'll find each di value by successive approximation, starting
    // from the most significant value.

    double weightCorrection = 0;
    for(x = 0; x < loops; x++)
    {
        weightCorrection += pow(256, x);
    }

    // subtract the min
    cycles -= 4*loops;

    // successive approximation
    for(x = loops - 1; x >= 0; x--)
    {
        weightCorrection -= pow(256, x);
        weight = pow(256, x) * (3*loops + 1) - weightCorrection;
        
        counters[x] = floor(cycles / weight) + 1;
        
        // make sure the result fits in 8 bits
        if(counters[x] > 256)
            counters[x] = 256;

        // reduce the remaining delay
        cycles -= (counters[x] - 1) * weight;
    }
}
