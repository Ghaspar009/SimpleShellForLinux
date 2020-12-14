/***************************************************************************
	@author       Kacper Knuth
	@file         microshell.c
*******************************************************************************/

#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 777
#include<stdio.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h>
#include<limits.h>
#include<linux/limits.h>
#include<fcntl.h>
#include <pwd.h>

#define PERMISSIONS 0666
#define BUF_TO_ARG_SIZE 128
#define DELIM " \n"
#define BUFSIZE 518

const char *shell_f[] = {
	"cd",
	"cp",
	"touch",
	"help",
	"exit"
};
const int count_f = sizeof(shell_f) / sizeof(shell_f[0]);

void type_prompt();
char **read_line();
void cd_f(char* arguments[]);
void help_f(char* arguments[]);
void exit_f(char* arguments[]);
void cp_f(char *arguments[]);
void touch_f(char* arguments[]);



int main(int argc, char** argv)
{
	while (1)
	{
		type_prompt();
		printf("\033[1;33m");
		char **arguments = read_line();
		void (*microshell_f[])(char**) = {cd_f, cp_f, touch_f, help_f, exit_f};
		int i = 0;
		int execute = 1;
		if (arguments[0] == NULL)
		{
			continue;
		}
		while (i != count_f)
		{
			
			if (strcmp(arguments[0], shell_f[i]) == 0)
			{
				(*microshell_f[i])(arguments);
				execute = 0;
			}
			i++;
		}
		if (execute)
		{
			if(fork() != 0)
			{
				wait(NULL);
			}
			else
			{
				if(execvp(arguments[0], arguments) == -1)
				{
					perror(arguments[0]);
                    			exit(EXIT_FAILURE);
				}
			}
            
		}
		free(arguments);
	}
	return 0;
}

void type_prompt()
{
	char current_path[PATH_MAX];
	/*char username[_POSIX_LOGIN_NAME_MAX]; */
    uid_t id = 0;
    struct passwd *pwd;


    pwd = getpwuid(id);    

	if (getcwd(current_path, sizeof(current_path)) == NULL)
	{
		int err = errno;
		printf("\033[1;31m");
		printf("%s\n", strerror(err)); 
	}
	else if ((pwd = getpwuid( getuid() ) ) == NULL)
    {
        printf("\033[1;31m");
        printf("login: error "); 
	}
	else
	{
		printf("\033[1;32m");
		printf(" %-8.8s", pwd->pw_name);
		printf("\033[1;34m");
		printf(":[");
		printf("\033[1;31m");
		printf("%s", current_path);
		printf("\033[1;34m");
		printf("]$ ");
	}
}

char **read_line()
{
	char* line = NULL;
	size_t length_line = 0;
	ssize_t gline;
	if ((gline = getline(&line, &length_line, stdin)) == -1)
	{
		int err = errno;
		printf("\033[1;31m");
		printf("%s\n", strerror(err));
	}
	int buffor_max = BUF_TO_ARG_SIZE, count_arg = 0;
	char *argument;
	char **arguments = malloc(buffor_max * sizeof(char*));
	argument = strtok(line, DELIM);
	while (argument != NULL)
	{
		arguments[count_arg] = argument;
		count_arg++;
		argument = strtok(NULL, DELIM);
		if (count_arg >= buffor_max)
		{
			buffor_max += BUF_TO_ARG_SIZE;
			arguments = realloc(arguments, buffor_max * sizeof(char*));
			if (!arguments)
			{
				printf("\033[1;31m");
				printf("allocation error\n");
			}
		}
	}
	arguments[count_arg] = NULL;
	return arguments;
}

void cd_f(char* arguments[])
{
	struct passwd *pwd = getpwuid(getuid());
	const char *homedir = pwd->pw_dir;
	
	if(arguments[2] == NULL)
	{
		if((arguments[1] == NULL) || (strcmp(arguments[1],"~") == 0))
		{
			if(chdir(homedir) != 0)
			{
				perror("~ ");
				arguments[1] = NULL;
			}
		}
		else
		{
			if(chdir(arguments[1]) != 0)
			{
				perror(arguments[1]);
				arguments[1] = NULL;
			}
			arguments[1] = NULL;
		}
	}
	else
	{
		printf("\033[1;31m");
		printf("cd: Too many arguments\n");
		arguments[1] = NULL;
	}
}

void help_f(char* arguments[])
{
	if (arguments[1] == NULL)
	{
		printf("\033[0;36m");
		printf("Microshell by Kacper Knuth\n");
		printf("\033[1;35m");
		printf("Builtin commands are:\n");
		int i = 0;
		while (i != count_f)
		{
			printf("\033[1;36m");
			printf("%s\n", shell_f[i]);
			i++;
		}
	}
	else
	{
		printf("\033[1;31m");
		printf("help: Too many arguments\n");
	}
}

void exit_f(char* arguments[])
{
	unsigned long n = 0;
	if (arguments[2] == NULL)
	{
		
		if(arguments[1] != NULL)
		{
			n = atoi(arguments[1]);
		}
		printf("\033[0m");
		exit(n);
	}
	else
	{
		printf("\033[1;31m");
		printf("exit: Too many arguments\n");
	}
}

void cp_f(char* arguments[])
{
	int file1, file2, n;
	char buffer[BUFSIZE];

	if (arguments[1] == NULL)
	{
		printf("\033[1;31m");
		printf("cp: missing file operand\n");
	}
	else if (arguments[2] == NULL)
	{
		printf("\033[1;31m");
		printf("cp: missing destination file operand after '%s'\n", arguments[1]);
	}
	else
	{
		if ((file1 = open(arguments[1], O_RDONLY, 0)) == -1)
		{
			printf("\033[1;31m");
			printf("cp: cannot open the file: %s\n", arguments[1]);
		}
		if ((file2 = creat(arguments[2], PERMISSIONS)) == -1)
		{
			printf("\033[1;31m");
			printf("cp: cannot create file: %s with permissions %03o\n", arguments[2], PERMISSIONS);
		}
		while ((n = read(file1, buffer, BUFSIZE)) > 0)
		{
			if (write(file2, buffer, n) != n)
			{
				printf("\033[1;31m");
				printf("cp: error while copying %s to %s\n", arguments[1], arguments[2]);
			}
		}
	}
}
void touch_f(char* arguments[])
{
	int file1;
	if (arguments[1] == NULL)
	{
		printf("\033[1;31m");
		printf("touch: missing file operand\n");
	}
	else
	{	
		if ((file1 = creat(arguments[1], PERMISSIONS)) == -1)
		{
			printf("\033[1;31m");
			printf("cp: cannot create file: %s with permissions %03o\n", arguments[1], PERMISSIONS);
		}
	}

}
