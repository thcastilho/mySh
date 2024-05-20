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

void nPipeFunction(char *programs[], int number_commands) {
    int i, j;
    int fds[number_commands - 1][2];

    for (i = 0; i < number_commands; i++) {
        // cria um pipe para cada "comunicação", ou seja, n - 1
        if (i < number_commands - 1) {
            if (pipe(fds[i]) == -1) {
                fprintf(stderr, "Pipe creation failed. Exiting with error...\n");
                exit(1);
            }
        }
        
        char *token;
        token = strtok(programs[i], " ");
        char *program;
        program = token;

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
        pid_t child_pid;
        child_pid = fork();
        
        if (child_pid == (pid_t)0) { /* se é um processo filho*/
            if (i != 0) { // se não é o primeiro comando
                // redireciona a entrada padrão para o pipe de leitura do comando anterior
                dup2(fds[i - 1][0], STDIN_FILENO);
            }

            if (i < number_commands - 1) { // se não é o último comando
                // redireciona a saída padrão para o pipe de escrita do próximo comando
                dup2(fds[i][1], STDOUT_FILENO);
            }

            // fecha todos os pipes abertos
            for (j = 0; j < number_commands - 1; j++) {
                close(fds[j][0]);
                close(fds[j][1]);
            }

            /* Agora execute PROGRAM, buscando-o no path. */
            execvp(program, arg_list);
            /* A função execvp só retorna se um erro ocorrer. */
            fprintf(stderr, "Command \'%s\' not found.\n", program);
            abort();
        } else { // se é o processo pai
            if (i != 0) { // se não é o primeiro comando
                // fecha o pipe de leitura do comando anterior
                close(fds[i - 1][0]);
                // fecha o pipe de escrita do comando anterior
                close(fds[i - 1][1]);
            }
        }
    }
   
    // fecha os pipes no processo pai
    for (i = 0; i < number_commands - 1; i++) {
        close(fds[i][0]); /* pai não precisa ler */
        close(fds[i][1]); /* pai não precisa escrever */
    }

    for (i = 0; i < number_commands; i++) {
        wait(NULL); /* espera todos os processos filhos */
    }

    return;
}

void executeProgram(char *programs[], int number_commands) {
    char* token;
    char* path;
    if (number_commands == 1) { // buscando por "exit" or "cd"
        token = strtok(programs[0], " ");
        char *program;
        program = token;

        char* path;
        if (strncmp(program, "exit", 4) == 0) { // se o programa é "exit", saia
            // printf("Exiting...\n");
            exit(0);
        } else if (strcmp(program, "cd") == 0) { // se o programa é "cd", mude o diretório
            path = strtok(NULL, "");
            if (path == NULL || strcmp(path, "~") == 0) {
                path = getenv("HOME");
            }
            if(chdir(path) != 0) {
                fprintf(stderr, "Error: No such file or directory.\n");
            }
            return;
        }
        /* se não é "exit" ou "cd", então execute o programa */
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
    } else {
        nPipeFunction(programs, number_commands);
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
	}
	return 0;
}
