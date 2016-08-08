/*Tri Nguyen
* masc0055
* CS570: Operating Systems
* Dr. John Carroll
* Simple POSIX SHELL. 
* Built-Ins: "cd", "ls", "ls-F", "setenv", "printenv"
* commands: pipe, redirection, double redirection, double redirection with pipe.
*/

#include "p2.h"
#define STDOUT_FILENO 1
#define STDIN_FILENO 0

char cmd[MAXITEM];		
char *ptrCmd = cmd;
char *newArgv[MAXITEM];
//arrays to parse for redirections
char *fileA[MAXITEM];
char *fileB[MAXITEM];
char *command1[MAXITEM];
char *command2[MAXITEM];
char inputChar;
int pos;			//position of the words in the newArgv array
int numWords;		//number of words for parse function
DIR *dirpoint;
struct dirent *dp;
struct stat sb;
//flags for metacharacters
int pipeFlag = 0;
int redirectionFlag = 0;
//flag for indicating order of redirections and redirections with pipe
int redirectionType = 0;
//k is for loop counter
int k;
int fileDescriptor;
//process ID for forking
int pid;
int dollarFlag = 0;

//start of program
int main()
{
	//infinite loop to take commands but breaks if there is EOF
	for(;;)
	{ 
		printf("Tri$: ");

		// //contains the incoming input characters from stdin
		inputChar = getchar();

		//line terminators and skipped
		if(inputChar == '\n' || inputChar == ';')
		{
			continue;
		}

		//if isn't line terminators then return the chars to stdin
		else
		{
			inputChar = ungetc(inputChar, stdin);
		}

		//return value of parse(); 
		int parseValue = parse(ptrCmd, newArgv);

		//breaks if parseValue is -1
		if(parseValue == EOF)
		{	
			break;
		}

		//if redirection or pipe is detected, first cal the parseredpip to parse and error check
		//then call redirection to actually do the pipe and redirection
		if(redirectionFlag == 1 || pipeFlag == 1)
		{
			parseredpip();
			settype();

			if(pipeFlag == 1 && redirectionFlag == 0)
			{
				pipeC();
			}
			if(redirectionFlag == 1 && pipeFlag == 0)
			{
				redirection();
			}
			if(redirectionFlag == 1 && pipeFlag == 1)
			{
				redirpipe();
			}

			//clear out all arrays for new sets of inputs
			for(k=0; k<=pos; k++)
			{	
				newArgv[k] = NULL;
				fileA[k] = NULL;
				fileB[k] = NULL;
				command1[k] = NULL;
				command2[k] = NULL;
			}

			redirectionType = 0;
			redirectionFlag = 0;
			pipeFlag = 0;
		}

		//if command is 'cd'. (BUILT-IN)
		//first check with strcmp for cd should return 1 if match. 
		//then change directory to whatever directory it is. 
		//if no arguments after 'cd' then change to 'HOME' directory.
		//only accepts one or less argument after cd.
		else if(strcmp(newArgv[0], "cd") == 0)
		{
			//flushing the standard out and standard error for clear buffer.
			fflush(stdout);
			fflush(stderr);

			//print error message if more than one argument
			if(newArgv[2] != NULL)
			{
				perror("More than one argument!");
			}

			//if argument is empty, then change to 'HOME' directory
			else if(newArgv[1] == NULL)
			{
				//if can not change to HOME directory, print error message
				if(chdir(getenv("HOME")) != 0)
				{
					perror("Could not change to HOME directory!");
				}
			}

			//if can not change to directory, then print error message
			else if(chdir(newArgv[1]) != 0)
			{
				perror("Can no change to that directory!");
			}
		}

		//if command is 'ls-F'. (BUILT-IN)
		//this function first checks the number of arguments. Only accepts 0 or 1 arguments.
		//if argument is NULL then list out the current contents in the working directory
		//This should work like the normal 'ls .' in a UNIX/LINUX shell.
		else if(strcmp(newArgv[0], "ls-F") == 0)
		{
			//flushing the standard out and standard error for clear buffer.
			fflush(stdout);
			fflush(stderr);

			//check for 2+ arguments
			if(newArgv[2] != NULL)
			{
				int i;
				//if 2+ arguments, print error to screen and restart main.
				perror("Error. To many arguments!");
				//clear out the newArgv array 
				for(i=0; i<=pos; i++)
				{
					newArgv[i] = NULL;
				}
				main();
			}

			//check for NULL argument. Behave like 'ls .' in UNIX/LINUX shell
			else if(newArgv[1] == NULL)
			{
				//open working directory. If it is NULL, print error to screen. 
				if((dirpoint = opendir(".")) == NULL)
				{
					int i;
					perror("Could not open current directory!");
					//clear our newArgv array and call main to restart array
					for(i=0; i<=pos; i++)
					{
						newArgv[i] = NULL;
					}
					main();
				}

				else 
				{
					//check if able to read dp is NULL
					while((dp = readdir(dirpoint)) != NULL)
					{
						lstat((dp->d_name), &sb);

						//check to see if its a dir, if it is add a / in front
						if((sb.st_mode & S_IFMT) == S_IFDIR)
						{		
							printf("%s/\n", dp->d_name);
						}	

						//check to see if is linked, then heck for broken softlinks.
						else if((sb.st_mode & S_IFMT) == S_IFLNK)
						{  
							//if broken softlink then print a & in front)
							if(stat(dp->d_name, &sb) == -1)
							{
								printf("%s&\n", dp->d_name);	
							}

							//if regular softlink, then print with @ in front.
							else{
								printf("%s@\n", dp->d_name);		
							}
						}	

						else if(!stat(dp->d_name, &sb))
						{
							//check to see if executable, if it is add * in front
							if(S_ISREG(sb.st_mode) && sb.st_mode & 0111)
						  	{	
						    	printf("%s*\n", dp->d_name);
							}
							//if regular file, print nothing in front
							else
							{
								printf("%s\n", dp->d_name);		
							}
						}
					}
				}
			}

			else if(newArgv[1] != NULL)
			{
				//check return value of . If NULL then return error message
				if((dirpoint = opendir(newArgv[1])) == NULL)
				{
					int i;
					perror("Can not open that directory!");

					//clear our newArgv array and call main to restart array
					for(i=0; i<=pos; i++)
					{
						newArgv[i] = NULL;
					}
					main();
				}
				else 
				{
					//check if able to read dp is NULL
					while((dp = readdir(dirpoint)) != NULL)
					{
						lstat((dp->d_name), &sb);

						//check to see if its a dir, if it is add a / in front
						if((sb.st_mode & S_IFMT) == S_IFDIR)
						{		
							printf("%s/\n", dp->d_name);
						}	

						//check to see if is linked, then heck for broken softlinks.
						else if((sb.st_mode & S_IFMT) == S_IFLNK)
						{  
							//if broken softlink then print a & in front)
							if(stat(dp->d_name, &sb) == -1)
							{
								printf("%s&\n", dp->d_name);	
							}

							//if regular softlink, then print with @ in front.
							else{
								printf("%s@\n", dp->d_name);		
							}
						}	

						else if(!stat(dp->d_name, &sb))
						{
							//check to see if executable, if it is add * in front
							if(S_ISREG(sb.st_mode) && sb.st_mode & 0111)
						  	{	
						    	printf("%s*\n", dp->d_name);
							}
							//if regular file, print nothing in front
							else
							{
								printf("%s\n", dp->d_name);		
							}
						}
					}
				}
			}
		}

		//printenv should only accept exactly one argument. (BUILT-IN)
		//Purpose is to print to screen the current value of the input's environment variable
		else if(strcmp(newArgv[0], "printenv") == 0)
		{
			//flushing the standard out and standard error for clear buffer.
			fflush(stdout);
			fflush(stderr);

			//if not exactly one argument, then print error message to screen
			if((newArgv[1] == NULL) || (newArgv[2] != NULL))
			{
				perror("printenv must have one argument!");
				
				//clear our newArgv array for new sets of inputs
				for(k=0; k<=pos; k++)
				{	
					newArgv[k] = NULL;
				}
				main();
			}

			//if environment variable does not have a value, print error to screen.
			else if(getenv(newArgv[1]) == NULL)
			{
				printf("%s: Undefined variable.\n", newArgv[1]);
				//clear our newArgv array for new sets of inputs
				for(k=0; k<=pos; k++)
				{	
					newArgv[k] = NULL;
				}
				main();
			}

			else
			{
				printf("%s\n", getenv(newArgv[1]));
			}
		}

		//setenv changes or add an environment variable if it doesnt exist. (BUILT-IN)
		//should accept only two arguments. 
		//First argument is the name, second is the value to replace that name
		else if(strcmp(newArgv[0], "setenv") == 0)
		{
			//flushing the standard out and standard error for clear buffer.
			fflush(stdout);
			fflush(stderr);

			//if there isn't two arguments, send error.
			if(newArgv[2] == NULL)
			{
				perror("Too few arguments for setenv. Must have two!");
				//clear our newArgv array for new sets of inputs
				for(k=0; k<=pos; k++)
				{	
					newArgv[k] = NULL;
				}
				main();
			}

			//if there are more than two arguments, send error.
			else if(newArgv[3] != NULL)
			{
				perror("Too many arguments for setenv. Must have two!");
				//clear our newArgv array for new sets of inputs
				for(k=0; k<=pos; k++)
				{	
					newArgv[k] = NULL;
				}
				main();
			}

			//execute the setenv 
			else
			{
				//setenv here is a system call.
				int setval = setenv(newArgv[1], newArgv[2], 1);

				//if setval returns -1 then there is an error and print to screen.
				if(setval < 0)
				{
					perror("setenv failed.");
					//clear our newArgv array for new sets of inputs
					for(k=0; k<=pos; k++)
					{	
						newArgv[k] = NULL;
					}
					main();					
				}
			}
		}

		//exit terminates the shell. (BUILT-IN)
		else if(strcmp(newArgv[0], "exit") == 0)
		{
			return 0;
		}

		//fork and execvp the commands from newargv.
		//check for return value of fork and execvp
		//wait for child and check wait's return value
		//do not execvpif dollarFlag is set to 1
		else if (dollarFlag != 1)
		{
			pid = fork();

			//pid must be zero to fork().
			if(pid == 0)
			{
				//print error to screen and exit if execvp return -1.
				if(execvp(newArgv[0], newArgv) == -1)
				{
					perror("Command not found.");
					exit(1);
				}
			}

			//pid of -1 sends an error to screen and exits
			if(pid < 0)
			{
				perror("fork failed!");
				exit(1);
			}

			//wait for child to complete process
			int w = wait(&pid);

			//check wait value
			if(w < 0)
			{
				perror("wait failed in main!");
			}
		}

		//reset flags
		dollarFlag = 0;
		redirectionFlag = 0;

		//clear out all arrays for new sets of inputs
		for(k=0; k<=pos; k++)
		{	
			newArgv[k] = NULL;
			fileA[k] = NULL;
			fileB[k] = NULL;
			command1[k] = NULL;
			command2[k] = NULL;
		}
	}

	//flushing the standard out and standard error for clear buffer.
	fflush(stderr);
	fflush(stdout);
	//send signal SIGTERM to the processID.
	killpg(getpid(), SIGTERM);	
	printf("Tri$ terminated.\n");
	exit(0);
	return 0;
}

//Parse takes in a char pointer and a char array of pointers. 
//Purpose of this function is to parse through user inputs and to 
//set flags for: <", ">", "|", "$".
int parse(char *ptr, char **newArg)
{
	//position contains the index of each word.
	pos = 0;
	//counter for loop
	int i;

	for(;;)
	{

		//numWords contains number of characters read from getword().c. 
		numWords = getword(ptr);

		//return -1 if there is EOF
		if(numWords == -1)
		{
			return -1;
		}

		//when there are no more words to collect. start the parsing of the commands.
		//set flags for metacharacters
		if(numWords == 0)
		{
			//loop through the newArgv array which contains the words from stdin to 
			//flag the metacharacters
			for(i=0; i<pos; i++)
			{
				//if ">" is detected then set to 1
				if(*newArgv[i] == '>' || *newArgv[i] == '<')
				{
					redirectionFlag = 1;
				}

				//if "|" is detected then set to 1
				else if(*newArgv[i] == '|')
				{
					pipeFlag = 1;
				}
			}

			return 0;
		}

		//character is $, skip it and replace the current value of the 
		//environment varaible that follows  '$'
		if(*ptr == '$')
		{
			//point to the next character 
			*ptr++;
			//set flag to 1 so it does not execvp the command preceeding the '$'
			dollarFlag = 1;
			//print error messages if the environment variable is null
			//else print the current value of current varaible
			if(getenv(ptr) == NULL)
			{
				printf("%s: Undefined variable.\n", ptr);
			}
			else
			{
				printf("%s\n", getenv(ptr));
			}
		}

		newArg[pos] = ptr;
		ptr = ptr+numWords+1;
		pos++;
	}
	return 0;
}

//parse out the words to execute the pipes and redirrections in its respective arrays.
//also throw errors if there is an ambiguous redirection.
//only one pipe is allowed
int parseredpip()
{
	//flag for when '<' is first character
	int firstin = 0;
	int finish = 0;
	//loop counters
	int i,j,l,m,t,p,r,y,f,n;
	int onlyPipe = 0; 	//flag to keep track if there is only pipe and no redirection
	int mc = 0;
	int onlypipe = 0;	//only if theres a pipe and no more meta character exit then set to 1
	int locationofPipe;

	/***** ERROR CHECKING for RED. and PIPE *****/

	//if the pipe character is the first character then send an error
	//clear the arrays, reset flags and call main
	if(*newArgv[0] == '|')
	{
		perror("Invalid null command.");

		//clear out all arrays for new sets of inputs
		for(k=0; k<=pos; k++)
		{	
			newArgv[k] = NULL;
			fileA[k] = NULL;
			fileB[k] = NULL;
			command1[k] = NULL;
			command2[k] = NULL;
		}

		redirectionFlag = 0;
		pipeFlag = 0;
		main();
	}

	if(*newArgv[0] == '>' || *newArgv[0] =='<')
	{
		perror("SHELL does not support that syntax.\nInvalid Command.");

		//clear out all arrays for new sets of inputs
		for(k=0; k<=pos; k++)
		{	
			newArgv[k] = NULL;
			fileA[k] = NULL;
			fileB[k] = NULL;
			command1[k] = NULL;
			command2[k] = NULL;
		}

		redirectionFlag = 0;
		pipeFlag = 0;
		main();
	}

	/***** ERROR CHECKS *****/
	for(i=0; i<pos; i++)
	{
		//case for '>' to parse to find ambiguous output redirects if > is  first
		if(*newArgv[i] == '>')
		{
			for(j=i+1; j<pos; j++)
			{
				//if > or | precedes '>', then its ambiguous
				//sending error message, then clear the arrays and reset flag and call main()
				if(*newArgv[j] == '>' || *newArgv[j] == '|')
				{
					perror("Ambiguous output redirect!");

					//clear out all arrays for new sets of inputs
					for(k=0; k<=pos; k++)
					{	
						newArgv[k] = NULL;
						fileA[k] = NULL;
						fileB[k] = NULL;
						command1[k] = NULL;
						command2[k] = NULL;
					}

					redirectionFlag = 0;
					pipeFlag = 0;
					main();
				}

				//check for ambiguos output after '<'
				else if(*newArgv[j] == '<')
				{
					for(l=j+1; l<pos; l++)
					{
						//if '|' is dected after > and < then its an error
						//sending error message, then clear the arrays and reset flag and call main()
						if(*newArgv[l] == '|')
						{
							perror("Ambiguous output redirect!");

							//clear out all arrays for new sets of inputs
							for(k=0; k<=pos; k++)
							{	
								newArgv[k] = NULL;
								fileA[k] = NULL;
								fileB[k] = NULL;
								command1[k] = NULL;
								command2[k] = NULL;
							}

							redirectionFlag = 0;
							pipeFlag = 0;
							main();						
						}
					}
					//break out of for loop
					break;
				}
			}
		}

		//case for '<' to parse to find ambiguous output redirects if < is first
		//if '<' is first character, then dont flag as error for sequence < > (| or > or <)
		else if(*newArgv[i] == '<')
		{
			//set flag when '<' is first
			if(*newArgv[0] == '<')
			{
				firstin = 1;
			}

			for(j=i+1; j<pos; j++)
			{
				//case for '<'
				//send error message, clear arrays, reset flags and call main
				if(*newArgv[j] == '<')
				{
					perror("Ambiguous output redirect!");

					//clear out all arrays for new sets of inputs
					for(k=0; k<=pos; k++)
					{	
						newArgv[k] = NULL;
						fileA[k] = NULL;
						fileB[k] = NULL;
						command1[k] = NULL;
						command2[k] = NULL;
					}

					redirectionFlag = 0;
					pipeFlag = 0;
					firstin = 0;
					main();		
				}

				//case for '|'
				//if '<' comes next then send error message, reset flags, clear arrays and call main
				if(*newArgv[j] =='|')
				{
					for(l=j+1; l<pos; l++)
					{
						if(*newArgv[l] == '<')
						{
							perror("Ambiguous output redirect!");

							//clear out all arrays for new sets of inputs
							for(k=0; k<=pos; k++)
							{	
								newArgv[k] = NULL;
								fileA[k] = NULL;
								fileB[k] = NULL;
								command1[k] = NULL;
								command2[k] = NULL;
							}

							redirectionFlag = 0;
							pipeFlag = 0;
							firstin = 0;
							main();					
						}
					}
				}

				//case for '>'
				//special case for when '<' is next. Check in its own loop when firstin is 1
				//else if '|' '>' or '<' comes next then throw error message
				//clear arrays, reset flags and call main
				if(*newArgv[j] == '>')
				{
					//special case for when '<' is first with '<' following. 
					//if '>' or '<' comes next then throw error, clear array, reset flag and call main
					if(firstin == 1)
					{
						for(t=j+1; t<pos; t++)
						{
							if(*newArgv[t] == '>' || *newArgv[t] == '<')
							{
								perror("Ambiguous output redirect!");

								//clear out all arrays for new sets of inputs
								for(k=0; k<=pos; k++)
								{	
									newArgv[k] = NULL;
									fileA[k] = NULL;
									fileB[k] = NULL;
									command1[k] = NULL;
									command2[k] = NULL;
								}

								redirectionFlag = 0;
								pipeFlag = 0;
								firstin = 0;
								main();									
							}
						}
						return 0;
					}

					for(m=j+1; m<pos; m++)
					{
						if(*newArgv[m] == '|' || *newArgv[m] == '>' || *newArgv[m] == '<')
						{
							perror("Ambiguous output redirect!");

							//clear out all arrays for new sets of inputs
							for(k=0; k<=pos; k++)
							{	
								newArgv[k] = NULL;
								fileA[k] = NULL;
								fileB[k] = NULL;
								command1[k] = NULL;
								command2[k] = NULL;
							}

							redirectionFlag = 0;
							pipeFlag = 0;
							firstin = 0;
							main();			
						}
					}
					//break out of for loop
					break;
				}
			}
		}

		//case for '|' to parse to find ambiguous output redirects
		else if(*newArgv[i] == '|')
		{
			for(j= i+1; j<pos; j++)
			{
				//case for two pipes. Send message saying it is not handled in this shell
				//clear array, reset flags, and call main
				if(*newArgv[j] == '|')
				{
					printf("Shell only handles one pipe. \n");

					//clear out all arrays for new sets of inputs
					for(k=0; k<=pos; k++)
					{	
						newArgv[k] = NULL;
						fileA[k] = NULL;
						fileB[k] = NULL;
						command1[k] = NULL;
						command2[k] = NULL;
					}

					redirectionFlag = 0;
					pipeFlag = 0;
					main();		
				}

				//case for '<'. ambiguous output. Clear array, reset flags and call main.
				else if(*newArgv[j] == '<')
				{
					perror("Ambiguous output redirect!");

					//clear out all arrays for new sets of inputs
					for(k=0; k<=pos; k++)
					{	
						newArgv[k] = NULL;
						fileA[k] = NULL;
						fileB[k] = NULL;
						command1[k] = NULL;
						command2[k] = NULL;
					}

					redirectionFlag = 0;
					pipeFlag = 0;
					main();		
				}

				//case for '>'
				//if any other redirections or pipe comes after then its an error and it clears array,
				//reset flags and calls main.
				else if(*newArgv[j] == '>')
				{
					for(l=j+1; l<pos; l++)
					{
						if(*newArgv[l] == '>' || *newArgv[l] == '<' || *newArgv[l] == 'l')
						{
							perror("Ambiguous output redirect!");

							//clear out all arrays for new sets of inputs
							for(k=0; k<=pos; k++)
							{	
								newArgv[k] = NULL;
								fileA[k] = NULL;
								fileB[k] = NULL;
								command1[k] = NULL;
								command2[k] = NULL;
							}

							redirectionFlag = 0;
							pipeFlag = 0;
							main();					
						}
					}
					//break out of for loop
					break;
				}
			}
		}
	}

	/***** PARSE THE COMMANDS *****/

	//when >, <, | is not the first character 
	//loop to find meta characters then store the index of those characters
	//use characters to iterate through newArgv to find the words and 
	//store it in its respective arrays.
	if(*newArgv[0] != '>' || *newArgv[0] != '<' || *newArgv[0] != '|')
	{
		for(i=0; i<pos; i++)
		{
			//case is detecting a |. set onlyPipe to 1 to indicate this might be
			//only a pipe command and nothing else. In next condition and loop if proven
			//false then set onlyPipe to 0
			if(*newArgv[i] == '|')
			{
				locationofPipe = i;
				onlyPipe = 1;
			}

			//case is detecting a < or >
			if(*newArgv[i] == '>' || *newArgv[i] == '<' || *newArgv[i] == '|')
			{
				//save index in newArgv of where the first (>, <) meta character is.
				//index is used to iterate through newArgv to get the first word to 
				//store in the command1 array
				j=i;
				for(l=0; l<j; l++)
				{
					command1[l] = newArgv[l];
				}

				//starting location for the next set of words is j+1 
				for(l=j+1; l<pos; l++)
				{
					//case for detecting < > | after first word has been collected 
					if(*newArgv[l] == '>' || *newArgv[l] == '<' || *newArgv[l] == '|')
					{
						//theres redirection so reset onlyPipe flag back to 0
						onlyPipe = 0;

						//when meta character is found. Store the word that preceds that 
						//meta character in fileA. this word should be the name of the output
						//or input file.
						fileA[0] = newArgv[--l];
						//save the index of the meta character found in newArgv
						m = l;

						//next starting location for the next set of words is t=m+1
						for(t=m+1; t<pos+1; t++)
						{
							if(newArgv[t] == NULL)
							{
								//if onlypipe is set then break;
								//no need to store a word for fileB[0] because it doesnt exist in input
								if(onlypipe == 1)
								{
									break;
								}
								else
								{
									//store the last words and the name for the file to be output/ or inputed to.
									fileB[0] = newArgv[--t];
									//change to return 0; later
									//leave for testing
									finish = 1;
								}
							}

							//case for a pipe |
							if(*newArgv[t] == '|')
							{
								//index of where pipe is in newArgv
								p = t;
								//next starting location for the next to fine the index of > < 
								for(p = t+1; p<pos; p++)
								{
									if(*newArgv[p] == '<' || *newArgv[p] == '>')
									{
										//r is the index of where > or < is in newArgv
										r = p;
										break;
									}
								}

								//if there was no < or > left in the input then set r to equal the last position
								if(r == 0)
								{
									r = pos;
									onlypipe = 1;
								}

								//location for the word after the pipe is at y=t+1
								//iterates through next meta character and stores the words
								//between those meta characters
								for(y=t+1; y<r; y++)
								{
									//mc starts at 0 and increase
									command2[mc] = newArgv[y];
									mc++;
								}
							}

							//if finish set to 1 then break out 
							else if(finish == 1)
							{
								break;
							}
						}
						break;
					}

					//condition that onlyPipe is 1 which means commmand is only a pipe.
					//store command in command2
					if(onlyPipe == 1)
					{	
						int marker = 0;
						for(n=locationofPipe+1; n<pos; n++)
						{
							command2[marker] = newArgv[n];
							marker++;
						}	
						break;
					}

					//if no more characters after first meta character then 
					//store the last words as the file name in fileA[] array
					if(newArgv[l+1] == NULL)	
					{
						fileA[0] = newArgv[l];
						break;
					}

					// //no other redirections or pipe detected
					// else{
					// 	fileA[0] = newArgv[l];
					// 	break;
					// }
				}
				break;
			}
		}
	}

	return 0;
}

//go through the commands from input and set the flags for the redirection type.
/* 
	 redirectionType = 1 ---> '>'
	 redirectionType = 2 ---> '>' '<'
	 redirectionType = 3 ---> '<'
	 redirectionType = 4 ---> '<' '>'
	 redirectionType = 5 ---> '<' '|'
	 redirectionType = 6 ---> '<' '|' '>'
*/
int settype()
{
	//loop counters
	int i,j,k,l;

	//main for loop to go through command to set the redirection type
	for(i=0; i<pos; i++)
	{
		//case for all combinantions starting with '>'
		if(*newArgv[i] == '>')
		{
			//loop to go through command after first case of '>'
			for(j=i+1;j<pos;j++)
			{
				//detecting a '>' then '<' other legal case for redirection
				//case for '>' and '<' is label for 2.
				if(*newArgv[j] == '<')
				{
					redirectionType = 2;
					break;
				}
				//just a '>' is label as 1
				else
				{
					redirectionType = 1;
				}
			}
			break;
		}

		//case for all combinations starting with '<'
		if(*newArgv[i] == '<')
		{
			//loop to go through command after case of '<'
			for(j=i+1;j<pos;j++)
			{
				//if '<' then '>' label as 4
				if(*newArgv[j] == '>')
				{
					redirectionType = 4;
					break;
				}

				//if '<' then '|' and nothing else then label as 5
				//if detecting '<' '|' '>' then label as 6
				if(*newArgv[j] == '|')
				{
					//loop to see if there is a '>'
					for(k=j+1;k<pos;k++)
					{
						if(*newArgv[k] == '>')
						{
							redirectionType = 6;
							break;
						}
						else
						{
							redirectionType = 5;
						}
					}
					break;
				}
				//case if only just '<' then label as 3
				else
				{
					redirectionType = 3;
				}
			}
			break;
		}
	}

	return 0;
}

//this method only handles pipe
//parent will create the pipe and the first child will run the first command to stdout of pipe
//the second child will then run the first command to the stin of pipe
int pipeC()
{
	//flush standard error and out
	fflush(stderr);
	fflush(stdout);

	int pfildes[2];
	int pidfirst, pidsecond;

	//parent creates pipe
	//check return value, if less than 0 then return an error
	int pipe1 = pipe(pfildes);
	if(pipe1 < 0)
	{
		perror("Error in creating pipe.");
		return 1;
	}

	//create first child and check the return value for error
	pidfirst = fork();
	if(pidfirst < 0)
	{
		perror("Forking of first child error.");
		return 1;
	}

	//check to see if child process by checking return value of 0
	//if 0, then execute dup2, close, and execvp
	if(pidfirst == 0)
	{
		//duplicate file descriptor
		int firstdup2 = dup2(pfildes[1], STDOUT_FILENO);	
		int close1 = close(pfildes[0]);
		int close2 = close(pfildes[1]);

		/* Error checking of dup2 and the two close() */
		if(firstdup2 < 0)
		{
			perror("dup2 error in pipeC.");
			return 1;
		}
		if(close1 < 0)
		{
			perror("close of pfildes[0] error");
			return 1;
		}
		if(close2 < 0)
		{
			perror("close of pfildes[1] error");
			return 1;
		}
		//execvp call on the first command of pipe which is in command1 array
		int execvp1 = execvp(command1[0], command1);

		if(execvp1 < 0)
		{
			perror("Exec of pipe on command1 failed.");
			return 1;
		}
	}

	//creat second child and check the return value for error
	pidsecond = fork();
	if(pidsecond < 0)
	{
		perror("Forking of second child error.");
		return 1;
	}

	//check to see if child process by checking return value of 0
	//if 0, then execute dup2, close, and execvp
	if(pidsecond == 0)
	{
		//duplicate file descriptor
		int seconddup2 = dup2(pfildes[0], STDIN_FILENO);
		int close1 = close(pfildes[0]);
		int close2 = close(pfildes[1]);

		/* Error checking of dup2 and the two close() */
		if(seconddup2 < 0)
		{
			perror("dup2 error in pipe.");
			return 1;
		}
		if(close1 < 0)
		{
			perror("close of pfildes[0] error");
			return 1;
		}
		if(close2 < 0)
		{
			perror("close of pfildes[1] error");
			return 1;
		}
		//execvp call on the fsecond command of pipe which is in command2 array
		int execvp2 = execvp(command2[0], command2);
		
		if(execvp2 < 0)
		{
			perror("Exec of pipe command2 failed.");
			return 1;
		}
	}

	int close1 = close(pfildes[0]);
	int close2 = close(pfildes[1]);

	if(close1 < 0)
	{
		perror("close of pfildes[0] error");
		return 1;
	}
	if(close2 < 0)
	{
		perror("close of pfildes[1] error");
		return 1;
	}

	//continue to reap children until the second child is found then break.
	for(;;)
	{
		int pid;
		pid = wait(NULL);

		if(pid < 0)
		{
			perror("wait error in pipeC.");
			return 1;
		}

		if(pid == pidsecond)
		{
			break;
		}
	}

	return 0;
}

//handles redirection without pipe.
//There are four cases describe by the flag redirectionType = 1,2,3,4.
/*	 redirectionType = 1 ---> '>'
	 redirectionType = 2 ---> '>' '<'
	 redirectionType = 3 ---> '<'
	 redirectionType = 4 ---> '<' '>'
*/
//redirection should never overwrite an exisiting file
//forks a child to do the redirection
int redirection()
{
	int openval, waitvalue;

	//output case. 
	//Can not overwrite a file that already exist.
	if(redirectionType == 1)
	{
		//clear the standard out and error.
		fflush(stderr);
		fflush(stdout);

		//check if file exit. If it does then throw error.
		if(access(fileA[0], F_OK) != -1) {
	    	perror("File already exist. Can not write to existing file.");
			return 1;
		}

		//reposition r/w file offset
		lseek(openval, 0, SEEK_SET);
		//open with create, append, read, write, and permissions flags
		openval = open(fileA[0], O_CREAT| O_APPEND | O_RDWR, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);

		//check return value of open()
		if(openval < 0)
		{
			perror("Open() failed in output redirection().");
			return 1;
		}

		//fork child to do redirection
		pid = fork();
		if(pid == 0)
		{
			//duplicate file descriptor
			int dupval = dup2(openval, STDOUT_FILENO);
			//close the open file
			int closeval = close(openval);
			//exec the commands thats stored in command1
			int execval = execvp(command1[0], command1);

			/* check return values for dup2, close, and execvp */
			if(execval == -1)
			{
				perror("Exec failed in output redirection().");
				waitvalue = wait(&pid);
				return 1;
			}
			if(dupval < 0)
			{
				perror("Dup2 failed in output redirection().");
				waitvalue = wait(&pid);
				return 1;
			}
			if(closeval == -1)
			{
				perror("Close failed in output redirection().");
				waitvalue = wait(&pid);
				return 1;
			}
		}

		else if(pid < 0){
			perror("Fork failed in output redirection().");
			return 1;
		}

		//wait for child process to finish
		wait(&pid);
		
		return 0;
	}

	//iput case.
	if(redirectionType == 3)
	{
		//clear the standard out and error.
		fflush(stderr);
		fflush(stdout);

		//open the file with rdonly and permision flags
		openval = open(fileA[0],O_RDONLY, S_IRUSR | S_IWUSR);

		if(openval < 0)
		{
			perror("File does not exist.");
			return 1;
		}

		//fork child to do redirection
		pid = fork();
		if(pid == 0){

			//duplicate file descriptor
			int dupval = dup2(openval, STDIN_FILENO);
			//close the open file
			int closeval = close(openval);
			//exec the commands thats stored in command1
			int execval = execvp(command1[0], command1);

			/* check return values for dup2, close, and execvp */
			if(execval == -1)
			{
				perror("Exec failed in output redirection().");
				waitvalue = wait(&pid);
				return 1;
			}
			if(dupval < 0)
			{
				perror("Dup2 failed in output redirection().");
				waitvalue = wait(&pid);
				return 1;
			}
			if(closeval == -1)
			{
				perror("Close failed in output redirection().");
				waitvalue = wait(&pid);
				return 1;
			}
		}

		else if(pid < 0){
			perror("Fork failed in output redirection().");
			return 1;
		}

		//wait for child process to finish
		wait(&pid);

		return 0;
	}

	//output then input case.
	//can not overwrite a file that already exist.
	if(redirectionType == 2)
	{
		//clear the standard out and error.
		fflush(stderr);
		fflush(stdout);

		//open the input file with rdonly and permission flags
		int openval1 = open(fileB[0],O_RDONLY, S_IRUSR | S_IWUSR);

		//check return value of the open for input file
		if(openval1 < 0){
			perror("File does not exist");
			return 1;
		}

		//check if output file exist
		if(access(fileA[0], F_OK) != -1) {
			perror("File already exist. Can not write to existing file.");
			return 1;
		}

		//open output file with create, append, read, write, and permissions flags
		int openval2 = open(fileA[0], O_WRONLY | O_APPEND | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);

		//check return value of open()
		if(openval2 < 0)
		{
			perror("Open() failed in output redirection().");
			return 1;
		}

		//fork child to do redirection
		pid = fork();
		if(pid == 0){
			//duplicate file descriptor for input file
			int dupval1 = dup2(openval1, STDIN_FILENO);
			//duplicate file descriptor for output file
			int dupval2 = dup2(openval2, STDOUT_FILENO);
			//close input file
			int closeval1 = close(openval1);
			//close output file
			int closeval2 = close(openval2);
			//exec command that is in array command1
			int execval = execvp(command1[0], command1);

			/* check return values for dup2, close, and execvp */
			if(execval == -1)
			{
				perror("Exec failed in output redirection().");
				waitvalue = wait(&pid);
				return 1;
			}
			if(dupval1 < 0)
			{
				perror("Dup2 failed in output redirection().");
				waitvalue = wait(&pid);
				return 1;
			}
			if(closeval1 == -1)
			{
				perror("Close failed in output redirection().");
				waitvalue = wait(&pid);
				return 1;
			}
			if(dupval2 < 0)
			{
				perror("Dup2 failed in output redirection().");
				waitvalue = wait(&pid);
				return 1;
			}
			if(closeval2 == -1)
			{
				perror("Close failed in output redirection().");
				waitvalue = wait(&pid);
				return 1;
			}
		}

		else if(pid < 0){
			perror("Fork failed in output redirection().");
			return 1;
		}

		//wait for child process to finish
		wait(&pid);
		return 0;
	}

	//input then output.
	//can not overwrite a file that already exist.
	if(redirectionType == 4)
	{
		//clear the standard out and error.
		fflush(stderr);
		fflush(stdout);

		//open the input file with rdonly and permission flags
		int openval1 = open(fileA[0],O_RDONLY, S_IRUSR | S_IWUSR);

		//check return value of the open for input file
		if(openval1 < 0){
			perror("File does not exist");
			return 1;
		}

		//check if output file exist
		if(access(fileB[0], F_OK) != -1) {
			perror("File already exist. Can not write to existing file.");
			return 1;
		}

		//open output file with create, append, read, write, and permissions flags
		int openval2 = open(fileB[0], O_WRONLY | O_APPEND | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);

		//check return value of open()
		if(openval2 < 0)
		{
			perror("Open() failed in output redirection().");
			return 1;
		}

		//fork child to do redirection
		pid = fork();
		if(pid == 0){
			//duplicate file descriptor for input file
			int dupval1 = dup2(openval1, STDIN_FILENO);
			//duplicate file descriptor for output file
			int dupval2 = dup2(openval2, STDOUT_FILENO);
			//close input file
			int closeval1 = close(openval1);
			//close output file
			int closeval2 = close(openval2);
			//exec command that is in array command1
			int execval = execvp(command1[0], command1);

			/* check return values for dup2, close, and execvp */
			if(execval == -1)
			{
				perror("Exec failed in output redirection().");
				waitvalue = wait(&pid);
				return 1;
			}
			if(dupval1 < 0)
			{
				perror("Dup2 failed in output redirection().");
				waitvalue = wait(&pid);
				return 1;
			}
			if(closeval1 == -1)
			{
				perror("Close failed in output redirection().");
				waitvalue = wait(&pid);
				return 1;
			}
			if(dupval2 < 0)
			{
				perror("Dup2 failed in output redirection().");
				waitvalue = wait(&pid);
				return 1;
			}
			if(closeval2 == -1)
			{
				perror("Close failed in output redirection().");
				waitvalue = wait(&pid);
				return 1;
			}
		}

		else if(pid < 0){
			perror("Fork failed in output redirection().");
			return 1;
		}

		//wait for child process to finish
		wait(&pid);
		return 0;
	}

	return 0;
}

//handles redirection and pipe. 
//only two cases decribe by the flag redirectionType 5,6
/*	
	 redirectionType = 5 ---> '<' '|'
	 redirectionType = 6 ---> '<' '|' '>'
*/
//redirection can never overwrite an existing file 
//forks a child to do the redirection
int redirpipe()
{
	int openval, waitvalue;

	//input then pipe.
	if(redirectionType == 5)
	{
		//clear the standard out and error.
		fflush(stderr);
		fflush(stdout);

		//open the file with rdonly and permision flags
		openval = open(fileA[0],O_RDONLY, S_IRUSR | S_IWUSR);

		if(openval < 0)
		{
			perror("File does not exist.");
			return 1;
		}

		int pfildes[2];
		int pidfirst, pidsecond;

		//parent creates pipe
		//check return value, if less than 0 then return an error
		int pipe1 = pipe(pfildes);
		if(pipe1 < 0)
		{
			perror("Error in creating pipe.");
			return 1;
		}

		//create first child and check the return value for error
		pidfirst = fork();
		if(pidfirst < 0)
		{
			perror("Forking of first child error.");
			return 1;
		}

		//check to see if child process by checking return value of 0
		//if 0, then execute dup2, close, and execvp
		if(pidfirst == 0)
		{
			//duplicate file descriptor
			int dupval = dup2(openval, STDIN_FILENO);
			//close the open file
			int closeval = close(openval);
			//duplicate file descriptor
			int firstdup2 = dup2(pfildes[1], STDOUT_FILENO);	
			int close1 = close(pfildes[0]);
			int close2 = close(pfildes[1]);

			/* Error checking of dup2 and the two close() */
			if(firstdup2 < 0)
			{
				perror("dup2 (firstdup2) error in redirpipe.");
				return 1;
			}
			if(firstdup2 < 0)
			{
				perror("dup2 (dupval) error in redirpipe.");
				return 1;
			}
			if(closeval < 0)
			{
				perror("close of closeval error");
				return 1;				
			}
			if(close1 < 0)
			{
				perror("close of pfildes[0] error");
				return 1;
			}
			if(close2 < 0)
			{
				perror("close of pfildes[1] error");
				return 1;
			}
			//execvp call on the first command of pipe which is in command1 array
			int execvp1 = execvp(command1[0], command1);

			if(execvp1 < 0)
			{
				perror("Exec of pipe on command1 failed.");
				return 1;
			}
		}

		//creat second child and check the return value for error
		pidsecond = fork();
		if(pidsecond < 0)
		{
			perror("Forking of second child error.");
			return 1;
		}

		//check to see if child process by checking return value of 0
		//if 0, then execute dup2, close, and execvp
		if(pidsecond == 0)
		{
			//duplicate file descriptor
			int seconddup2 = dup2(pfildes[0], STDIN_FILENO);
			int close1 = close(pfildes[0]);
			int close2 = close(pfildes[1]);

			/* Error checking of dup2 and the two close() */
			if(seconddup2 < 0)
			{
				perror("dup2 error in redirpipe.");
				return 1;
			}
			if(close1 < 0)
			{
				perror("close of pfildes[0] error");
				return 1;
			}
			if(close2 < 0)
			{
				perror("close of pfildes[1] error");
				return 1;
			}
			//execvp call on the fsecond command of pipe which is in command2 array
			int execvp2 = execvp(command2[0], command2);
			
			if(execvp2 < 0)
			{
				perror("Exec of pipe command2 failed.");
				return 1;
			}
		}

		int close1 = close(pfildes[0]);
		int close2 = close(pfildes[1]);

		if(close1 < 0)
		{
			perror("close of pfildes[0] error");
			return 1;
		}
		if(close2 < 0)
		{
			perror("close of pfildes[1] error");
			return 1;
		}

		//continue to reap children until the second child is found then break.
		for(;;)
		{
			int pid;
			pid = wait(NULL);

			if(pid < 0)
			{
				perror("wait error in redirpipe.");
				return 1;
			}

			if(pid == pidsecond)
			{
				break;
			}
		}

		return 0;
	}

	//input pipe then output.
	//can not overwrite an existing file.
	if(redirectionType == 6)
	{
		//clear the standard out and error.
		fflush(stderr);
		fflush(stdout);

		//open the input file with rdonly and permission flags
		int openval1 = open(fileA[0],O_RDONLY, S_IRUSR | S_IWUSR);

		//check return value of the open for input file
		if(openval1 < 0){
			perror("File does not exist");
			return 1;
		}

		//check if output file exist
		if(access(fileB[0], F_OK) != -1) {
			perror("File already exist. Can not write to existing file.");
			return 1;
		}

		//open output file with create, append, read, write, and permissions flags
		int openval2 = open(fileB[0], O_WRONLY | O_APPEND | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);

		//check return value of open()
		if(openval2 < 0)
		{
			perror("Open() failed in output redirection().");
			return 1;
		}

		int pfildes[2];
		int pidfirst, pidsecond;

		//parent creates pipe
		//check return value, if less than 0 then return an error
		int pipe1 = pipe(pfildes);
		if(pipe1 < 0)
		{
			perror("Error in creating pipe.");
			return 1;
		}

		//create first child and check the return value for error
		pidfirst = fork();
		if(pidfirst < 0)
		{
			perror("Forking of first child error.");
			return 1;
		}

		//check to see if child process by checking return value of 0
		//if 0, then execute dup2, close, and execvp
		if(pidfirst == 0)
		{
			//duplicate file descriptor
			int dupval = dup2(openval1, STDIN_FILENO);
			//close the open file
			int closeval = close(openval1);
			//duplicate file descriptor
			int firstdup2 = dup2(pfildes[1], STDOUT_FILENO);	
			int close1 = close(pfildes[0]);
			int close2 = close(pfildes[1]);

			/* Error checking of dup2 and the two close() */
			if(firstdup2 < 0)
			{
				perror("dup2 (firstdup2) error in redirpipe.");
				return 1;
			}
			if(dupval < 0)
			{
				perror("dup2 (dupval) error in redirpipe.");
				return 1;				
			}
			if(closeval < 0)
			{
				perror("close of closeval error");
				return 1;
			}
			if(close1 < 0)
			{
				perror("close of pfildes[0] error");
				return 1;
			}
			if(close2 < 0)
			{
				perror("close of pfildes[1] error");
				return 1;
			}

			//execvp call on the first command of pipe which is in command1 array
			int execvp1 = execvp(command1[0], command1);

			if(execvp1 < 0)
			{
				perror("Exec of pipe on command1 failed.");
				return 1;
			}
		}

		//creat second child and check the return value for error
		pidsecond = fork();

		if(pidsecond < 0)
		{
			perror("Forking of second child error.");
			return 1;
		}

		//check to see if child process by checking return value of 0
		//if 0, then execute dup2, close, and execvp
		if(pidsecond == 0)
		{
			//duplicate file descriptor
			int dupvalue2 = dup2(openval2, STDOUT_FILENO);
			int seconddup2 = dup2(pfildes[0], STDIN_FILENO);
			//close the open file
			int closeval2 = close(openval2);
			int close1 = close(pfildes[0]);
			int close2 = close(pfildes[1]);

			/* Error checking of dup2 and the two close() */
			if(seconddup2 < 0)
			{
				perror("dup2 (seconddup2) error in redirpipe.");
				return 1;
			}
			if(dupvalue2 < 0)
			{
				perror("dup2 (dupvalue) error in redirpipe.");
				return 1;
			}
			if(closeval2 < 0)
			{
				perror("close of closeval2 error");
				return 1;
			}
			if(close1 < 0)
			{
				perror("close of pfildes[0] error");
				return 1;
			}
			if(close2 < 0)
			{
				perror("close of pfildes[1] error");
				return 1;
			}
			//execvp call on the fsecond command of pipe which is in command2 array
			int execvp2 = execvp(command2[0], command2);
			
			if(execvp2 < 0)
			{
				perror("Exec of pipe command2 failed.");
				return 1;
			}
		}

		int close1 = close(pfildes[0]);
		int close2 = close(pfildes[1]);

		if(close1 < 0)
		{
			perror("close of pfildes[0] error");
			return 1;
		}
		if(close2 < 0)
		{
			perror("close of pfildes[1] error");
			return 1;
		}

		//continue to reap children until the second child is found then break.
		for(;;)
		{
			int pid;
			pid = wait(NULL);

			if(pid < 0)
			{
				perror("wait error in redirpipe.");
				return 1;
			}

			if(pid == pidsecond)
			{
				break;
			}
		}

		return 0;
	}

	return 0;
}
