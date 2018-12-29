
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <assert.h>

struct sigaction SIGINT_action = {0};
struct sigaction SIGTSTP_action = {0};

int modeFlag = 0;

pid_t pPid;


void myCd(char *args[512], int numberOfArgs);
void myExit();
void killChildren(int signo);
void calcStatus(int childExitMethod, int* status, int* statusFlag);
void printStatus(int status, int statusFlag);
char *strReplace(char *orig, char *replace, char *with);
int inpRed(char* to, int* fd);
int outRed(char* to, int* fd);
void catchSIGTSTP(int signo);


void myCd(char *args[512], int numberOfArgs)
{
	char newDir[2048];
	if (numberOfArgs == 1)
	{
		strcpy(newDir, getenv("HOME"));
		chdir(newDir);
	}
	else if (numberOfArgs == 2)
	{
		strcpy(newDir, args[1]);
		chdir(newDir);

	}
	else
	{
		perror("Error:");
	}

}


void calcStatus(int childExitMethod, int* status, int* statusFlag)
{
	if (WIFEXITED(childExitMethod) != 0)
	{
		*status = WEXITSTATUS(childExitMethod);
		*statusFlag = 0;
	}
	else if (WIFSIGNALED(childExitMethod) != 0)
	{
		*status = WTERMSIG(childExitMethod);
		*statusFlag = 1;
	}
}


void printStatus(int status, int statusFlag)
{
	if (statusFlag == 0)
	{
		printf("exit value %d\n", status);
		fflush(stdout);
	}
	else
	{
		printf("terminated by signal %d\n", status);
		fflush(stdout);
	}
}


char *strReplace(char *orig, char *replace, char *with)
{
    int outputsize = strlen(orig) + 1;
    int dollarsFound = 0;
    char *res = malloc(outputsize);
    int resoffset = 0;
    char *needle;
    while (needle = strstr(orig, replace))
    {
    	if (dollarsFound % 2 == 0)
    	{
	        memcpy(res + resoffset, orig, needle - orig);
	        resoffset += needle - orig;
	        orig = needle + strlen(replace);
	        outputsize = outputsize - strlen(replace) + strlen(with);
	        res = realloc(res, outputsize);
	        memcpy(res + resoffset, with, strlen(with));
	        resoffset += strlen(with);
    	}
    	dollarsFound++;
    }
    strcpy(res + resoffset, orig);
    return res;
}


int inpRed(char* to, int* fd)
{
	*fd = open(to, O_RDONLY);
	fcntl(*fd, F_SETFD, FD_CLOEXEC);
	if (*fd == -1)
	{
		return 1;
	}
	int result = dup2(*fd, 0);
	if (result == -1)
	{
		return 1;
	}
	return 0;
}


int outRed(char* to, int* fd)
{
	*fd = open(to, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	fcntl(*fd, F_SETFD, FD_CLOEXEC);
	if (*fd == -1)
	{
		return 1;
	}
	int result = dup2(*fd, 1);
	if (result == -1)
	{
		return 1;
	}
	return 0;
}


void catchSIGTSTP(int signo)
{
	if (modeFlag == 0)
	{
		char* msg1 = "\nEntering foreground-only mode (& is now ignored)\n:";
		write(STDOUT_FILENO, msg1, 51);
		modeFlag = 1;
	}
	else
	{
		char* msg2 = "\nExiting foreground-only mode\n:";
		write(STDOUT_FILENO, msg2, 31);
		modeFlag = 0;
	}
}


void myExit()
{
	signal(SIGUSR1, killChildren);
	kill(-pPid, SIGUSR1);
}


void killChildren(int signo)
{
    assert(signo == SIGUSR1);
    pid_t self = getpid();
    if (pPid != self)
    {
    	_exit(0);
    }
}


int main(int argc, char const *argv[])
{
	SIGINT_action.sa_handler = SIG_IGN;
	sigfillset(&SIGINT_action.sa_mask);
	SIGINT_action.sa_flags = 0;
	sigfillset(&SIGTSTP_action.sa_mask);
	SIGTSTP_action.sa_flags = SA_RESTART;
	sigaction(SIGINT, &SIGINT_action, NULL);

	pid_t spawnpid = -5;
	int childExitMethod = -5;
	int status = 0;
	int statusFlag = 0;
	int bgFlag = 0;
	int printErrorFlag = 0;
	int inFlag = 0;
	int outFlag = 0;
	int bgRet = 0;
	int pidFlag = 0;
	int fileFlag = 0;

	char *userInput;
	int nbytes = 2048;
	userInput = (char *) malloc(nbytes + 1);
	memset(userInput, '\0', sizeof(userInput));
	int charsRead = -5;

	char* pidStr;
	char tempPid[512];
	char* argArr[512];
	char temp[2048];
	char *token;
	char inFileName[512];
	char outFileName[512];
	pPid = getpid();

	do
	{
		SIGTSTP_action.sa_handler = catchSIGTSTP;
		sigaction(SIGTSTP, &SIGTSTP_action, NULL);
		pidFlag = 0;
		printErrorFlag = 0;
		bgFlag = 0;
		inFlag = 0;
		outFlag = 0;

		sprintf(tempPid, "%d", pPid);
		memset(argArr, '\0', sizeof(argArr));
		while (1)
		{
			printf(": ");
			fflush(stdout);
			charsRead = getline(&userInput, &nbytes, stdin);
			if (charsRead == -1)
			{
				clearerr(stdin);
			}
			else
			{
				break;
			}
		}
		if (userInput[charsRead - 1] == '\n')
		{
			userInput[charsRead - 1] = '\0';
			--charsRead;
		}
		if (userInput[charsRead - 1] == '&' && printErrorFlag == 0)
		{
			userInput[charsRead - 1] = '\0';
			if (modeFlag == 0)
			{
				bgFlag = 1;
			}
			--charsRead;
		}
		strcpy(temp, userInput);
		token = strtok(temp, " ");
		int argCnt = 0;
		if (token != '\0')
		{
			unsigned int i;
			for (i = 0; i < strlen(token); i++)
			{
				if (token[i] == '$')
				{
					if (token[i+1] == '$')
					{
						pidStr = strReplace(token, "$$", tempPid);
						strcpy(tempPid, pidStr);
						argArr[argCnt] = tempPid;
						pidFlag = 1;
						free(pidStr);
						pidStr = NULL;
						break;
					}
				}
			}
		}
		if (pidFlag == 0)
		{
			argArr[argCnt] = token;
		}
		while(token != NULL)
		{
			pidFlag = 0;
			fileFlag = 0;
		    argCnt++;
		    token = strtok(NULL, " ");
			if (token != NULL)
			{
				if (strcmp(token, "<") == 0)
				{
					inFlag = 1;
					fileFlag = 1;
					token = strtok(NULL, " ");
					strcpy(inFileName, token);

				}
				else if (strcmp(token, ">") == 0)
				{
					
					outFlag = 1;
					fileFlag = 1;
					token = strtok(NULL, " ");
					strcpy(outFileName, token);
				}
				else
				{
					unsigned int j;
					for (j = 0; j < strlen(token); j++)
					{
						if (token[j] == '$')
						{
							if (token[j+1] == '$')
							{
								pidStr = strReplace(token, "$$", tempPid);
								strcpy(tempPid, pidStr);
								argArr[argCnt] = tempPid;
								pidFlag = 1;
								free(pidStr);
								pidStr = NULL;
								break;
							}
						}
					}
				}

			}
			if (pidFlag == 0 && fileFlag == 0)
			{
		    	argArr[argCnt] = token;
			}

		}
		if (argArr[0] == '\0')
		{
			//ignore the command
		}
		//check if first char of first arg is a # symbol.
		//if so do nothing to the input
		else if (argArr[0][0] == '#')
		{
			//do nothing
		}
		else if (strcmp(argArr[0], "cd") == 0)
		{
			myCd(argArr, argCnt);
		}
		else if (strcmp(argArr[0], "status") == 0)
		{
			printStatus(status, statusFlag);
		}
		//use exit function to... you guessed it, exit.
		else if (strcmp(argArr[0], "exit") == 0)
		{
			myExit();
			free(userInput);
			userInput = NULL;
			exit(0);
		}
		//handle forking
		else
		{
			spawnpid = fork();
			switch(spawnpid)
			{
				case -1:
				{
					perror("Error");
					break;
				}
				case 0:
				{
					int sourceFD, targFD, result;
					SIGTSTP_action.sa_handler = SIG_IGN;
					sigaction(SIGTSTP, &SIGTSTP_action, NULL);
					if (bgFlag == 0)
					{
						SIGINT_action.sa_handler = SIG_DFL;
						sigaction(SIGINT, &SIGINT_action, NULL);
					}
					if (inFlag == 1 && outFlag == 1)
					{
						result = inpRed(inFileName, &sourceFD);
						if (result == 1)
						{
							fprintf(stderr, "cannot open %s for output", inFileName);
						}
						result = outRed(outFileName, &targFD);
						if (result == 1)
						{
							fprintf(stderr, "cannot open %s for output", outFileName);
						}

					}
					else if (inFlag == 1)
					{
						result = inpRed(inFileName, &sourceFD);
						if (result == 1)
						{
							fprintf(stderr, "cannot open %s for output", inFileName);
						}
						if (bgFlag == 1)
						{
							result = outRed("/dev/null", &targFD);
							if (result == 1)
							{
								fprintf(stderr, "cannot open /dev/null for output");
								exit(1);
							}

						}
					}
					else if (outFlag == 1)
					{
						result = outRed(outFileName, &targFD);
						if (result == 1)
						{
							fprintf(stderr, "cannot open %s for output", outFileName);
						}
						if (bgFlag == 1)
						{
							result = inpRed("/dev/null", &sourceFD);
							if (result == 1)
							{
								fprintf(stderr, "cannot open /dev/null for input");
								exit(1);
							}
						}
					}
					else if (bgFlag == 1)
					{

						result = inpRed("/dev/null", &sourceFD);
						if (result == 1)
						{
							fprintf(stderr, "cannot open /dev/null for input");
							exit(1);
						}

						result = outRed("/dev/null", &targFD);
						if (result == 1)
						{
							fprintf(stderr, "cannot open /dev/null for output");
							exit(1);
						}
					}
					execvp(argArr[0], argArr);
					perror(argArr[0]);
					exit(1);
					break;
				}
				default:
				{
					if (bgFlag == 1)
					{
						printf("background pid is %d\n", spawnpid);
						fflush(stdout);
					}
					else
					{
						pid_t actualPid = waitpid(spawnpid, &childExitMethod, 0);
						SIGTSTP_action.sa_handler = catchSIGTSTP;
						sigaction(SIGTSTP, &SIGTSTP_action, NULL);
						calcStatus(childExitMethod, &status, &statusFlag);
						if (WIFSIGNALED(childExitMethod) != 0)
						{
							printStatus(status, statusFlag);
						}
					}

					break;
				}
			}
		}
		do
		{
			bgRet = waitpid(-1, &childExitMethod, WNOHANG);
			if (bgRet > 0)
			{
				calcStatus(childExitMethod, &status, &statusFlag);
				printf("background pid %d is done: ", bgRet);
				fflush(stdout);
				printStatus(status, statusFlag);
			}
		} while (bgRet > 0);
	} while (1);
}