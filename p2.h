#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <strings.h>
#include <signal.h>
#include <stddef.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/wait.h>
#include "getword.h"
#define MAXITEM 100 /*max number of words per line */
#define MAXFILENAME 50

//Main function of program. It takes calls parse() for user input. Does the fork() 
//and exec() functions of the inputs.
int main();

//This function is responsible for syntactic analysis. It repeatedly calls
//the getword() function. parse() set flags when getword() encounters a char that is a metacharacter. 
int parse(char *, char **); 

//function to parse and error check the redirections and pipes
int parseredpip();

//function to process redirections 
int redirection();

//function to handle pipes
int pipeC();

//function to handle redirection plus pipe
int redirpipe();

//function to set the type of redirections and pipes. 
int settype();

