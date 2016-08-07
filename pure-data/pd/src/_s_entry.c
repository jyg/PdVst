/* In MSW, this is all there is to pd; the rest sits in a "pdlib" dll so
that externs can link back to functions defined in pd. */

int sys_main(int argc, char **argv);

/*
 * gcc does not support the __try stuff, only MSVC.  Also, MinGW allows you to
 * use main() instead of WinMain(). <hans@at.or.at>
 */
#if defined(_MSC_VER) && !defined(COMMANDVERSION)
#include <windows.h>
#include <stdio.h>
/* jsarlo { */
#include <malloc.h>

#define MAXARGS 1024
#define MAXARGLEN 1024

int tokenizeCommandLineString(char *clString, char **tokens)
{
    int i, charCount = 0;
    int tokCount= 0;
    int quoteOpen = 0;

    for (i = 0; i < strlen(clString); i++)
    {
        if (clString[i] == '"')
        {
            quoteOpen = !quoteOpen;
        }
        else if (clString[i] == ' ' && !quoteOpen)
        {
            tokens[tokCount][charCount] = 0;
            tokCount++;
            charCount = 0;
        }
        else
        {
            tokens[tokCount][charCount] = clString[i];
            charCount++;
        }
    }
    tokens[tokCount][charCount] = 0;
    tokCount++;
    return tokCount;
}

int WINAPI WinMain(HINSTANCE hInstance,
                               HINSTANCE hPrevInstance,
                               LPSTR lpCmdLine,
                               int nCmdShow)
{
    int i, argc;
    char *argv[MAXARGS];
	post("Ducon msvc  was here\n");
    __try
    {
	for (i = 0; i < MAXARGS; i++)
        {
            argv[i] = (char *)malloc(MAXARGLEN * sizeof(char));
        }
        GetModuleFileName(NULL, argv[0], MAXARGLEN);
        argc = tokenizeCommandLineString(lpCmdLine, argv + 1) + 1;
        sys_main(__argc,__argv);
	 for (i = 0; i < MAXARGS; i++)
        {
            free(argv[i]);
        }
    }
    __finally
    {
        printf("caught an exception; stopping\n");
    }
    //return (0);
}
/* } jsarlo */

#else /* not _MSC_VER ... */
int main(int argc, char **argv)
{
    return (sys_main(argc, argv));
}
#endif /* _MSC_VER */


