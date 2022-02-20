// Keyword Interpreter Code Generator
//
// by Nikolai Golovchenko March 31, 2000 - April 2, 2000;
// April 21, 2000 - added to form list of whitespaces and delimiters
//					and more comments
// http://techref.massmind.org/member/NG--944
// golovchenko@mail.ru
//
// 10-July
//     Banner removed
//     Added command line support
//     Broke source in several files by classes for easier maintainability
// 11-July
//     Fixed bug when called without query string
//     from MS IIS 4.0 server
// 14-July
//   1) Embedded the minimal form in code to display it if the external page doesn't exist
//	  2) Implemented shared access to the log file

#include <stdio.h>
#ifdef _WIN32
#include <windows.h>
#include <share.h>
#define strcasecmp _stricmp
#else
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#endif 
#include <string.h>
#include <time.h>
#include <ctype.h>

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

#include "listnode.h"
#include "listtree.h"
#include "form.h"

#include "keyword.h"
#include "keyword.inc"

#ifdef __EMSCRIPTEN__
const bool isCGI = false;
#else
bool isCGI;
#endif

bool Insense;
bool Whole;
TextFormEntry Whitespaces; //lists of characters from form
TextFormEntry Delimiters; //lists of characters from form

int
main(int argc, char* argv[], char** envp)
{
    llist entries = { NULL }; //clear list
    char Text[TextSize];		//list of keywords
    int Error = 0;

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
    ***********************************/

#ifndef __EMSCRIPTEN__
    //what kind of request
    isCGI = getenv("REQUEST_METHOD") != NULL;
#endif

    /* Read form values */
    read_cgi_input(&entries);

    if (isCGI)
    {
        //log request if this routine is called from server
        logRequest();
        //check if form is empty
        if (!is_field_exists(entries, "Keywords")
            || !is_field_exists(entries, "Whitespaces")
            || !is_field_exists(entries, "Delimiters"))
        {
            //CGI form should have query string and these fields
            helpScreen();
            list_clear(&entries);
            exit(0);
        }
        //Generate header and title
        html_header();
        html_begin("Code Generator Results");
    }
    else if (argc == 1 || !is_field_exists(entries, "Keywords"))
    {
        //command line mode and no required parameters
        helpScreen();
        list_clear(&entries);
        exit(0);
    }

    if (is_field_exists(entries, "Keywords"))
    {
        char* x;
        size_t len;
        x = cgi_val(entries, "Keywords");
        len = strlen(x);

        if (len == 0)
        {
            Error = 1;
        }
        else
        {
            if (len <= TextSize)
            {
                strcpy(Text, x);
            }
            else
            {
                Error = 2;
            }
        }
    }
    Insense = false;
    if (is_field_exists(entries, "CaseInsensitive"))
    {
        if (strcasecmp(cgi_val(entries, "CaseInsensitive"), "yes") == 0)
        {
            Insense = true;
        }
    }
    Whole = false;
    if (is_field_exists(entries, "Whole"))
    {
        if (strcasecmp(cgi_val(entries, "Whole"), "yes") == 0)
        {
            Whole = true;
        }
    }
    if (is_field_exists(entries, "Whitespaces"))
    {
        strcpy(Whitespaces.text, cgi_val(entries, "Whitespaces"));
        Whitespaces.ParseCharsList();
    }
    if (is_field_exists(entries, "Delimiters"))
    {
        strcpy(Delimiters.text, cgi_val(entries, "Delimiters"));
        Delimiters.ParseCharsList();
    }
    if (Error == 0)
    {
        /* Open preformatted block */
        if (isCGI)
        {
            printf("<PRE>\n");
        }
        /* Call code generation */
        Error = generate(Text, Insense, Whole);
        /* Close preformatted block */
        if (!Error)
        {
            if (isCGI)
            {
                printf("</PRE>\n");
                TimeStamp();
                Banner();
            }
        }
        else
        {
            ReportError(Error);
        }
    }
    else
    {
        /* Report error */
        ReportError(Error);
    }

    if (isCGI)
    {
        html_end();
    }
    list_clear(&entries);
    Whitespaces.Clear();
    Delimiters.Clear();
    return 0;
}

void
ReportError(char Err)
{
    if (isCGI)
    {
        printf("<H2>");
    }
    printf("Error %d: ", Err);
    switch (Err)
    {
    case 1:
        printf("Text area empty\n");
        break;
    case 2:
        printf("Text area size exceeded %d characters\n", TextSize);
        break;
    case 3:
        printf("No keywords\n");
        break;
    case 4:
        printf("Too much states to fit in an 8bit variable\n");
        break;
    case 5:
        printf("Two keywords with the same name detected\n");
        break;
    default:
        break;
    }
    if (isCGI)
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
    char* query;

    time(&nowTime);
    final = nowTime + 3; //3 seconds for retries

    while (log == NULL && nowTime < final)
    {
#ifdef _WIN32
      log = _fsopen( "keyword_log.csv", "a", _SH_DENYWR);   //create or open file in append mode
      //writing is disabled for other processes

      if(log == NULL)
      {
        // try again in a bit
        Sleep(987);
      }
            
#else
      log = fopen("./keyword_log.csv", "a");
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


    if (log != NULL)
    {
        //file opened ok

        //if file is empty, create header
        if (fseek(log, 0, SEEK_END))
            return;

        long size = ftell(log);

        if (size < 0)
            return;

        if (size == 0)
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
        if (query != NULL)
        {
            fprintf(log, "%s", query);
        }
        fprintf(log, "\n");
        fclose(log);
    }
}

//print env variable if not empty
void
logPrint(FILE* log, char* envName)
{
    char* var;

    var = getenv(envName);
    if (var != NULL)
    {
        fprintf(log, "%s", var);
    }
    fprintf(log, ","); //comma separation
}

void
TimeStamp()
{
    time_t ltime;
    struct tm* gmt;

    /* Display UTC. */
    time(&ltime);
    gmt = gmtime(&ltime);
    printf("; Generated by <A HREF=\"/cgi-bin/keyword.exe\">www.piclist.com/cgi-bin/keyword.exe</A> (%s version)<BR>\n", VERSIONDATE);
    printf("; %s GMT<BR>\n", asctime(gmt));
}

void Banner()
{
#ifdef SHOW_BANNER
    printf("<TABLE ALIGN=\"CENTER\"  BORDER=\"1\"  CELLPADDING=\"8\">\n");
    printf("	<TR>\n");
    printf("		<TD WIDTH=90%s BGCOLOR=\"#FFFF00\" >\n", "%");
    printf("		<FONT COLOR=\"#000080\"><CENTER>\n");
    printf("I hope my little code generator helps you. Would you like to help \n");
    printf("me find a job in the U.S.?<BR>\n");
    printf("<A HREF=\"http://techref.massmind.org/member/NG--944\">Nikolai Golovchenko</A>\n");
    printf("		</CENTER></FONT></TD>\n");
    printf("	</TR>\n");
    printf("</TABLE>\n");
#endif
}

void
helpScreen()
{
    FILE* form;
    char ch;

    if (isCGI)
    {
        html_header();
        //try to open form
        form = fopen("keyword.htm", "r");
        if (form == NULL)
        {
            html_begin("Keyword Interpreter Code Generator");
            h1("Keyword Interpreter Code Generator");
            printf("<!-- Can't open \"keyword.htm\" -->\n");
            //print built-in form
            printf("<FORM action=\"/cgi-bin/keyword.exe\" method=\"GET\">\n");
            printf("<TABLE  border=\"2\" cellpadding=\"5\" bgcolor=\"#E0E0E0\">\n");
            printf("<TR><TD valign=\"TOP\"><H2>Keywords List</H2>\n");
            printf("<TEXTAREA name=\"Keywords\" rows=\"20\" cols=\"32\">send\n");
            printf("write\n");
            printf("read\n");
            printf("on\n");
            printf("off\n");
            printf("0\n");
            printf("1\n");
            printf("help\n");
            printf("test\n");
            printf("getEE\n");
            printf("putEE</TEXTAREA><BR>\n");
            printf("Whitespaces:<BR><INPUT type=\"text\" name=\"Whitespaces\" size=\"36\" maxlength=\"255\" value=\"' ', '\\t', 0x0D, 0x0A\"><BR>\n");
            printf("Delimeters:<BR><INPUT type=\"text\" name=\"Delimiters\" size=\"36\" maxlength=\"255\" value=\"' ', '\\t', 0x0D, 0x0A\"><BR>\n");
            printf("<input type=\"checkbox\" name=\"CaseInsensitive\" value=\"yes\">Case	insensitive<BR>\n");
            printf("<input type=\"checkbox\" name=\"Whole\" value=\"yes\">Whole words match<BR><BR><BR>\n");
            printf("<CENTER><INPUT type=\"submit\" value=\"Generate Now!\"></CENTER></TD></TR>\n");
            printf("</TABLE>	</FORM>\n");
            html_end();
        }
        else
        {
            //print external form
            while (!feof(form))
            {
                size_t n = fread(&ch, 1, 1, form);
                if(n)
                    putchar(ch);
            }
            fclose(form);
        }
    }
    else
    {
        printf("--------------------------------------------------------------------\n");
        printf("KEYWORD INTERPRETER CODE GENERATION IN PIC ASSEMBLY\n");
        printf("http://www.piclist.com/codegen/\n");
        printf("--------------------------------------------------------------------\n");
        printf("Command line: keyword.exe {option=value} {> OutputFile}\n");
        printf("Options:\n\tKeywords - list of space separated keywords\n");
        printf("\tCaseInsensitive - yes/no (default: no)\n");
        printf("\tWhole - whole words match, yes/no (default: no)\n");
        printf("\tWhitespaces - characters that are ignored before a word (no default)\n");
        printf("\tDelimiters - characters indicating end of word  (no default)\n");
        printf("\nNote:\n");
        printf("  If a value contains spaces, insert quotes around the value,\n  e.g. Keywords=\"send write read\"\n");
        printf("\nExample:\n");
        printf("  keyword.exe keywords=\"send write read\" whitespaces=\"' ', '\\t'\" delimiters=\"' ','\\t',0x0D,0x0A\" > result.asm\n");
    }
}

int
generate(char* Text, bool Insense, bool Whole)
{
    int Err, keycode;
    static char code[MAXCODESIZE];
    listtree* parsetree;
    char* p;

    code[0] = 0;
    parsetree = new listtree(Insense, Whole);

    //convert to lowercase if caseinsensitive
    if (Insense)
    {
        p = Text;
        while (*p != '\0')
        {
            *p = tolower(*p);
            p++;
        }
    }
    //build the parser tree
    p = Text;
    while (*p != '\0')
    {
        //skip whitespace
        if (isspace(*p))
        {
            p++;
        }
        else
        {
            if (*p != '\0')
            {
                //start of a keyword
                p = parsetree->addkeyword(p);
                if (parsetree->Error)
                {
                    return parsetree->Error;
                }
            }
        }
    }

    if (parsetree->keywords == 0)
    {
        Err = 3;	//error - no keywords
    }
    else
    {
        if ((parsetree->keywords + parsetree->totalstates()) > 254)
        {
            Err = 4;	//error - too much states to fit in an 8bit variable
        }
        else
        {
            //print #define and cblock
            printf("#define parse_state_offset %#.2x\n\n", parsetree->keywords + 1);
            printf("\tcblock\n");
            printf("\tparse_state, parse_char\n");
            printf("\tendc\n");
            //generates keyword symbol table
            printf(";-------------- keywords list ---------------------\n");
            if (Insense)
            {
                printf("; Case Insensitive\n");
            }
            else
            {
                printf("; Case Sensitive\n");
            }
            if (Whole)
            {
                printf("; Whole words match\n");
            }
            else
            {
                printf("; Incomplete words match\n");
            }
            Whitespaces.printList();
            printf("; Whitespaces: %s\n", Whitespaces.textBuf);
            Delimiters.printList();
            printf("; Delimiters: %s\n", Delimiters.textBuf);

            printf(";\n; Keywords codes:\n");
            p = Text;
            keycode = 1;
            while (*p != '\0')
            {
                if (!isspace(*p))
                {
                    printf("; %#.2x: ", keycode++);
                    while ((*p != '\0') && (!isspace(*p)))
                    {
                        printf("%.1s", p++);
                    }
                    printf("\n");
                }
                else
                {
                    p++;
                }
            }
            printf("\n");
            //print file keywordasm1
            printf("%s", keywordasm1);
            if (Insense)
            {
                //if case insensitive, add conversion to lower case
                printf(";convert to lower case\n");
                printf("\taddlw\t-'A'\n");
                printf("\tsublw\t'Z' - 'A' + 1\n");
                printf("\tskpnc\n");
                printf("\t bsf parse_char, 5\n");
                //print the rest of keyword.asm
            }
            //print whitespace skiping code
            if (Whitespaces.CharsList != NULL)
            {
                printf("\tmovfw parse_state       ;check if current state is zero\n");
                printf("\tskpz\n");
                printf("\t goto parse2\n");
                printf("\tmovfw parse_char        ;then skip whitespaces\n");
                Whitespaces.printfCode();
                printf("parse2\n");
            }
            //print file keywordasm2
            printf("%s", keywordasm2);
            //print delimiters skiping code
            printf("parse_state_delimiter\n");
            printf(";called from state table to check for end of name\n");
            printf("\tmovwf parse_state\n");
            if (Delimiters.CharsList != NULL)
            {
                printf("\tmovfw parse_char\n");
                Delimiters.printfCode();
                printf("\tretlw 0xFF\t\t;Not a delimiter - error\n");
            }
            else
            {
                printf("\tretlw 0\n");
            }

            //generate jumptable
            printf("\nparse_table_start\n");
            parsetree->jumptable(code);
            printf(";---------------- jump table start ----------------\n");
            printf("%s", code);
            printf(";---------------- jump table end ------------------\n\n");
            //generate statetable
            code[0] = '\0';
            parsetree->statetable(code);
            printf(";--------------- state table start ----------------\n");
            printf("%s", code);
            printf(";--------------- state table end ------------------\n\n");
            Err = 0;
        }
    }

    delete parsetree;
    return Err;
}

