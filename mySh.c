#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>

#define PATH_MAX 1024

void showPrompt() {
	char cwd[PATH_MAX];
	char hostname[PATH_MAX];
	char directory[PATH_MAX];

	getcwd(cwd, sizeof(cwd));
	gethostname(hostname, PATH_MAX);
	if (strcmp(cwd, getenv("HOME")) == 0) {  //se está no diretório home
		printf("[MySh] %s@%s:~$ ", getenv("LOGNAME"), hostname);
		fflush(stdout);
	} else {
		if (strncmp(cwd, getenv("HOME"), strlen(getenv("HOME"))) == 0) // se esta dentro do diretorio home
		{
			strcpy(directory, cwd + strlen(getenv("HOME")) + 1);
			printf("[MySh] %s@%s:~/%s$ ", getenv("LOGNAME"), hostname, directory);
		} else {
			printf("[MySh] %s@%s:%s$ ", getenv("LOGNAME"), hostname, cwd); // se nao esta dentro de home
		}
		fflush(stdout);
	}
}

void ctrlCHandler(int sig)
{
	signal(SIGINT, ctrlCHandler);
	fflush(stdout);
	return;
}

void ctrlZHandler(int sig)
{
	signal(SIGINT, ctrlZHandler);
	fflush(stdout);
	return;
}

/* Subrotina retirada dos slides da aula */
int spawn(char *program, char **arg_list)
{
	pid_t child_pid;
	/* Duplicar este processo. */
	child_pid = fork();
	if (child_pid != 0) {/* Este é o processo pai. */
		return child_pid;
	} else {
		/* Agora execute PROGRAM, buscando-o no path. */
		execvp(program, arg_list);
		/* A função execvp só retorna se um erro ocorrer. */
		fprintf(stderr, "Command \'%s\' not found.\n", program);
		abort();
	}
}

void onePipeFunction(char *programs[], int number_commands) {
    /* getting the first program to execvp() */
    char *token;
    token = strtok(programs[0], " ");
    char *program_1;
    program_1 = token;

    char *arg_list_1[10];
    int size = 0;
    while (token != NULL)
    {
        arg_list_1[size] = token;
        token = strtok(NULL, " ");
        size++;
    }   
    arg_list_1[size] = NULL;

    /* getting the first program to execvp() */
    token = strtok(programs[1], " ");
    char *program_2;
    program_2 = token;

    char *arg_list_2[10];
    size = 0;
    while (token != NULL)
    {
        arg_list_2[size] = token;
        token = strtok(NULL, " ");
        size++;
    }   
    arg_list_2[size] = NULL;

    /* ****************************************************************** */
    int fds[2];
    if (pipe(fds) == -1) {
        fprintf(stderr, "Pipe creation failed. Exiting with error...\n");
        exit(1);
    }

    /******************** first process ********************/
    int child1_status;
    /* using the pipe */
    pid_t child1_pid;
    child1_pid = fork();
    
    if (child1_pid == (pid_t)0)
    {
        dup2(fds[1],STDOUT_FILENO); // redirect stdout to pipe
        close(fds[0]); //closing the read
        /* Primeiro processo filho (quem escreve) */
        execvp(program_1, arg_list_1);
    }
    close(fds[1]); /* parent process no need writing */

    /******************** second process ********************/
    int child2_status;
    /* using the pipe */
    pid_t child2_pid;
    child2_pid = fork();
    if (child2_pid == (pid_t)0)
    {
        dup2(fds[0],STDIN_FILENO); // redirect stdout to pipe
        /* Primeiro processo filho (quem escreve) */
        execvp(program_2, arg_list_2);
    }
    close(fds[0]); /* parent process no need reading */

    wait(NULL);
    wait(NULL);

    return;
}

void executeProgram(char *programs[], int number_commands) {
    char* token;
    char* path;
    if (number_commands == 1) { //checking for "exit" or "cd" instruction
        token = strtok(programs[0], " ");
        char *program;
        program = token;

        char* path;
        if (strncmp(program, "exit", 4) == 0) { // if the program is "exit", QUIT
            // printf("Exiting...\n");
            exit(0);
        } else if (strcmp(program, "cd") == 0) { // if the program is "cd", change the directory
            path = strtok(NULL, "");
            if (path == NULL || strcmp(path, "~") == 0) {
                path = getenv("HOME");
            }
            if(chdir(path) != 0) {
                fprintf(stderr, "Error: No such file or directory.\n");
            }
            return;
        }

        char *arg_list[10];
        int size = 0;
        while (token != NULL)
        {
            arg_list[size] = token;
            token = strtok(NULL, " ");
            size++;
        }   
        arg_list[size] = NULL;

        int child_status;

        spawn(arg_list[0], arg_list);
        wait(&child_status);
    } else if (number_commands == 2) {
        onePipeFunction(programs, number_commands);
    }
}

int main(void)
{
	signal(SIGINT, ctrlCHandler);
	signal(SIGTSTP, ctrlZHandler);

	int i;
	int number_commands;
	char* arg;
    char *token;

	while (1)
	{
		showPrompt();
        number_commands = 1;

		char instruction[1024];
		if (scanf(" %[^\n]%*c", instruction) == -1)
		{
			printf("exit\n");
			exit(0);
		}

        for (i = 0; i < strlen(instruction); i++) {
            if (instruction[i] == '|') {
                number_commands += 1;
            }
        }

        char *programs[number_commands];
		token = strtok(instruction, "|");
        i = 0;
        while (token != NULL)
			{
				programs[i] = token;
				token = strtok(NULL, "|");
				i++;
            }

        executeProgram(programs, number_commands);


		// } else { // else, execute the program
		// 	char *arg_list[32];

		// 	int size = 0;
		// 	while (token != NULL)
		// 	{
		// 		arg_list[size] = token;
		// 		token = strtok(NULL, " ");
		// 		size++;
		// 	}
		// 	arg_list[size] = NULL;
		// 	/* Neste ponto, todos os argumentos estão na lista de argumentos */
		// 	i = 0;
		// 	number_commands = 1;

		// 	while(arg_list[i] != NULL) {
		// 		if(strcmp(arg_list[i], "|") == 0){
		// 			number_commands++;
		// 		}
		// 		i++;
		// 	}

		// 	executeProgram(number_commands, arg_list);
		// }
	}
	return 0;
}

/**
 * https://stackoverflow.com/questions/12981199/multiple-pipe-implementation-using-system-call-fork-execvp-wait-pipe-i
 * https://stackoverflow.com/questions/9070177/redirecting-output-of-execvp-into-a-file-in-c
 * https://stackoverflow.com/questions/13801175/classic-c-using-pipes-in-execvp-function-stdin-and-stdout-redirection
 * https://stackoverflow.com/questions/14288559/why-does-my-c-program-of-a-pipeline-with-two-pipes-hang?rq=3
 * function do(commands)
    if commands is of size 1
        exec commands[0] || die
    split commands into c1 (first command) c2 (the rest of them)
    open
    if fork 
        close input end of pipe
        dup output of pipe to stdin
        do (c2) || die
    close output end of pipe
    dup input of pipe to stdout
    exec c1 || die
 * 
*/
