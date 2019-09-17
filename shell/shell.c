/*
 ============================================================================
 Name        : shell.c
 Author      : Eric Gurevich
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

char *getLine(void);
void doStuffLoop();
int parse(char *line, char **parsed);
void eval(char **argv, int argc);
int searchCommandPath(char **argv);
int tryForkExec(char *path, char **argv);
int checkRedirect(char **argv, int argc);
int checkBackground(char **argv, int argc);
void freeStrings(char **argv);
int checkPipe(char **argv, int argc);
int pipeStuff(char **argv1, int argc1, char **argv2, int argc2);

//
void cd(char **argv, int argc);
void clr();
void dir(char **argv, int argc);
void pauseShell();
void printEnviron();
void echo(char **argv, int argc);

extern char **environ;

char *shellpath[PATH_MAX];		//path of shell
char *currdir[PATH_MAX];			//path of current directory
int batchflag = 0;				//indicator for line reading function

int main(int argc, char **argv) {
	//set shell path and environment variable
	strncpy(shellpath,argv[0],strlen(argv[0]) + 1);
	setenv("shell",shellpath,1);

	cd(NULL, 0); //set current directory for prompt

	if (argc > 1) {		//if executing from batch redirect stdin
		fprintf(stdout,"Executing from batch file.\n");
		int batch = open(argv[1], O_RDONLY);
		if (batch < 0) {
			fprintf(stdout, "File open error\n");
			exit(0);
		} else {
			dup2(batch, STDIN_FILENO);
			close(batch);
		}
		batchflag = 1;
	}

	doStuffLoop();

}

char *getLine(void) {		//gets line from stdin

	size_t maxlen = 200;

	char *line = malloc(maxlen);
	char *lineptr = line;

	int c;
	int counter = 0;

	while (1) {
		if (counter >= maxlen - 1) {	//double size of buffer if reading more than maxlen chars
			line = realloc(line, maxlen * 2);
			lineptr = line + counter;
			maxlen *=2;

		}

		c = fgetc(stdin);



		*lineptr = c;
		if (*lineptr == EOF) {
			fprintf(stdout, "Terminating.\n");
			exit(0);
			return line;
		}

		if (*lineptr == '\n') {
			*lineptr = '\0';
			if (batchflag) {		//if executing from batchfile, advance stdin by one character
				fgetc(stdin);
			}
			break;
		}

		lineptr++;
		counter++;
	}

	return line;

}

void doStuffLoop() {	//main loop that does reading, parsing, evaluating
	char *line;

	char *parsed[100];	//max 100 args, because why would you ever need more

	int argc;

	do {
		//read**************

		fprintf(stdout, "%s%s", currdir, ">");
		line = getLine();

		//if eof
		if (*line == EOF) {
			fprintf(stdout, "Terminating.\n");
			exit(0);
		}

		//parse***************

		argc = parse(line,parsed);

		/*//for debugging
		fprintf(stdout, "*--------------*\n%d inputs\n", argc);
		fprintf(stdout, "command: %s\n", parsed[0]);
		fprintf(stdout, "args:\n");

		for (int i = 1; i < argc; i++) {
			fprintf(stdout, "%s\n", parsed[i]);
		}
		fprintf(stdout, "*---------------*\n");*/


		//evaluate******************

		/*checks if background (&). if true, calls checkRedirect() within function. we can stop here.*/
		int checkback = checkBackground(parsed, argc);

		if (checkback == 0) {	//if no background

			/*checks if pipe. if true, calls eval() within function. we can stop here. REDIRECTS AND INTERAL COMMANDS NOT SUPPORTED IF PIPING*/
			int checkpipe = checkPipe(parsed, argc);
			if (checkpipe == 0) {	//if no pipe

				/*checks if redirect in/out. if true, calls eval() within function. we can stop here.
				*ensure < [file] comes before > [file] if redirecting both*/
				int checkredir = checkRedirect(parsed, argc);
				if (checkredir == 0) {	//if no redirects

					//evaluates arguments after checking for &, pipe, redirect
					eval(parsed, argc);
				}
			}
		}

		free(line);

	}
	while (1);

}

int parse(char *line, char **parsed) {		//parse line for arguments
	char *linecpy = malloc(strlen(line) + 1);
	strncpy(linecpy, line, strlen(line) + 1);

	char *tok = strtok(linecpy, " \t\n");

	int count = 0;	//argc

	while (tok != NULL && count < 100) {
		parsed[count] = malloc(strlen(tok) + 1);
		strncpy(parsed[count],tok,strlen(tok) + 1);
		tok = strtok(NULL, " \t\n");
		count++;
	}

	parsed[count] = (char*) NULL;	//indicator for later so that we know where args end

	free(linecpy);
	return count;	//argc
}

void eval(char **argv, int argc) {		//evaluate parsed arguments
	//for debugging
	/*fprintf(stdout, "cmd1[0]: %s\ncmd1[1]:%s\ncmd2[2]:%s\n", argv[0], argv[1], argv[2]);
	fprintf(stdout, "%d\n", argc);*/

	if (!strncmp(argv[0], "quit", 100)) {	//exit
		fprintf(stdout, "Terminating.\n");
		exit(0);
	}
	if (!strncmp(argv[0], "cd", 100)) {
		cd(argv, argc);
		freeStrings(argv); //frees all char*s in array of parsed arguments
		return;
	}
	if (!strncmp(argv[0], "clr", 100)) {
		clr();
		freeStrings(argv);
		return;
	}
	if (!strncmp(argv[0], "dir", 100)) {
		dir(argv, argc);
		freeStrings(argv);
		return;
	}
	if (!strncmp(argv[0], "environ", 100)) {
		printEnviron();
		freeStrings(argv);
		return;
	}
	if (!strncmp(argv[0], "echo", 100)) {
		echo(argv,argc);
		freeStrings(argv);
		return;
	}
	if (!strncmp(argv[0], "help", 100)) {
		char *readmepath = malloc(PATH_MAX);	//get path of shell
		int pathlen = strlen(getenv("shell"));
		strncpy(readmepath, getenv("shell"), pathlen - 5);
		readmepath[pathlen - 4] = '\0';
		strcat(readmepath,"readme");		//concat readme.txt to shell folder
		argv[0] = "more";
		argv[1] = readmepath;
		argv[2] = (char*) NULL;
		searchCommandPath(argv);	//exec more with path as arg
		free(readmepath);
		return;
	}
	if (!strncmp(argv[0], "pause", 100)) {
		pauseShell();
		freeStrings(argv);
		return;
	} else {
		if(searchCommandPath(argv) == -1) {
			fprintf(stdout,"External command not found.\n");
		}
		freeStrings(argv);
	}

}

void cd(char **argv, int argc) {
	if (argc <= 1) {	//just cd, print working directory
		if (getcwd(currdir, sizeof(currdir)) != NULL) {
			fprintf(stdout, "Current working dir: %s\n", currdir);
		} else {
			fprintf(stderr, "getcwd() error");
		}
	} else {	//cd + args, change directory
		if(!chdir(argv[1])) {	//successful
			cd(argv, 1);
		} else {	//unsuccessful
			fprintf(stdout, "No such directory, or other error.\n");
		}
	}
}

void clr() {
	for (int i = 0; i < 100; i++) {
		fprintf(stdout, "\n");
	}
}

void dir(char **argv, int argc) {
	struct dirent *dr;
	DIR *dp;

	if(argc != 2) {
		fprintf(stdout, "Enter dir followed by a directory to view.\n");
		return;
	}

	if ((dp = opendir(argv[1])) == NULL) {
		fprintf(stdout, "No such directory, or other error.\n");
		return;
	}

	while ((dr = readdir(dp)) != NULL) {
		fprintf(stdout, "%s\n", dr->d_name);
	}

	closedir(dp);
}

void pauseShell() {
	char c;
	while(1) {
		sleep(1);
		c = fgetc(stdin);
		if (c == '\n') {
			return;
		}
	}
}

void printEnviron() {
	char **ptr = environ;
	while(*ptr != NULL) {
		fprintf(stdout, "%s\n",*ptr);
		ptr++;
	}
}

void echo(char **argv, int argc) {
	for (int i = 1; i < argc; i++) {
		fprintf(stdout, "%s ", argv[i]);
	}
	fprintf(stdout, "\n");
}

int searchCommandPath(char **argv) {		//searches for external commands
	char *pathpath = malloc(PATH_MAX);
	strncpy(pathpath,getenv("PATH"),strlen(getenv("PATH")));

	char *cmdpath = malloc(PATH_MAX);

	char *tokstr;
	int tryforkexec;

	tokstr = strtok(pathpath, ":");	//paths in PATH environment variable separated by :

	while (tokstr != NULL) {
		strncpy(cmdpath,tokstr,strlen(tokstr) + 1);
		strcat(cmdpath, "/");
		strcat(cmdpath,argv[0]);

		tryforkexec = tryForkExec(cmdpath,argv);

		if (tryforkexec != -1) {
			free(pathpath);
			free(cmdpath);
			return 0;
		}

		tokstr = strtok(NULL, ":");
	}


	free(pathpath);
	free(cmdpath);
	return -1;
}

int tryForkExec(char *path, char **argv) {		//try forking and execing external command
	pid_t pid;
	int status;

	if ((pid = fork()) == -1) {		//fork error
		fprintf(stdout, "Fork error\n");
		return -1;
	} else if(pid == 0) {	//child

		execv(path, argv);
		return -1;

	} else {		//parent
		if (waitpid(pid,&status,0) == -1) {
			return -1;
		}
	}

	return 0;
}

//checks for redirects, ensure < [file] comes before > [file] if redirecting both
int checkRedirect(char **argv, int argc) {
	if (argc < 3) {	//if definitely no redirects, dont do anything
		return 0;
	}

	int inredir = 0;	//booleans if in or out redirected
	int outredir = 0;
	char **newargv;		//new argv and argc
	int newargc;

	char *infrom;		//in path
	char *outto;		//out path

	if (!strncmp(argv[argc - 4], "<", 100) && !strncmp(argv[argc - 2], ">", 100)) {	//both redirect < >
		inredir = 1;
		outredir = 1;

		newargv = malloc(sizeof(argv));
		memcpy(newargv, argv, (argc - 4) * sizeof(newargv[0]));
		newargv[argc - 4] = (char*) NULL;

		newargc = argc - 4;

		infrom = argv[argc - 3];
		outto = argv[argc - 1];

	} else if (!strncmp(argv[argc - 2], "<", 100)) {	//just stdin <
		inredir = 1;

		newargv = malloc(sizeof(argv));
		memcpy(newargv, argv, (argc - 2) * sizeof(newargv[0]));
		newargv[argc - 2] = (char*) NULL;

		newargc = argc - 2;

		infrom = argv[argc - 1];

	} else if (!strncmp(argv[argc - 2], ">", 100)) {	//just stdout >
		outredir = 1;
		newargv = malloc(sizeof(argv));
		memcpy(newargv, argv, (argc - 2) * sizeof(newargv[0]));
		newargv[argc - 2] = (char*) NULL;

		newargc = argc - 2;

		outto = argv[argc - 1];
	} else {
		return 0;	//no redirects
	}

	pid_t pid;
	int status;
	int fd0;
	int fd1;

	if ((pid = fork()) == -1) {		//fork error
		fprintf(stdout, "Fork error\n");
		return -1;
	} else if (pid == 0) {	//child
		if (inredir) {	//if stdin redirected
			fprintf(stdout, "Redirecting in.\n");
			fd0 = open(infrom, O_RDONLY);
			if (fd0 < 0) {
				fprintf(stdout, "File open error\n");
			} else {
				dup2(fd0, STDIN_FILENO);
				close(fd0);
			}
		}

		if (outredir) {		//if stdout redirected
			fprintf(stdout, "Redirecting out.\n");

			fd1 = open(outto, O_WRONLY | O_CREAT | O_TRUNC, 0664);
			if (fd1 < 0) {
				fprintf(stdout, "File open error\n");
			} else {
				dup2(fd1, STDOUT_FILENO);
				close(fd1);
			}
		}

		eval(newargv, newargc);	//evalute using new stdin/stdout

		_exit(0);

		} else {		//parent
			if (waitpid(pid, &status, 0) == -1) {
				return -1;
		}
	}

	free(newargv);
	return 1;	//redirected
}

int checkBackground(char **argv, int argc) {
	if (argc < 2) {	//if definitely no background, dont do anything
		return 0;
	}

	if (!strncmp(argv[argc - 1], "&", 100)) {
		fprintf(stdout,"Running in background.\n");

		char **newargv = malloc(sizeof(argv));
		memcpy(newargv, argv, (argc - 1) * sizeof(newargv[0]));
		newargv[argc] = (char*) NULL;

		int newargc = argc - 1;

		pid_t pid;

		if ((pid = fork()) == -1) {		//fork error
			fprintf(stdout, "Fork error\n");
			return -1;
		} else if (pid == 0) {	//child
			/*checks if pipe. if true, calls eval() within function. we can stop here.*/
			int checkpipe = checkPipe(newargv, newargc);
			if (checkpipe == 0) {	//if no pipe

				/*checks if redirect in/out. if true, calls eval() within function. we can stop here.*/
				int checkredir = checkRedirect(newargv, newargc);
				if (checkredir == 0) {	//if no redirects

					//evaluates arguments after checking for pipe and redirect
					eval(newargv, newargc);
					_exit(0);
				}
			}

		} else {		//parent
			//no waiting
		}
		free(newargv);
		return 1;
	}

	return 0;	//no &
}

//check for pipe character, separate into two argvs
int checkPipe(char **argv, int argc) {
	int pipeindex = 0;
	while(argv[pipeindex] != NULL) {
		if (!strncmp(argv[pipeindex], "|", 100)) {
			break;
		} else {
			pipeindex++;
		}
	}


	if (pipeindex >= argc - 1) {
		return 0;	//no pipe
	}

	fprintf(stdout, "Piping.\n");

	//get argv before pipe
	char **cmd1 = malloc(sizeof(argv));
	memcpy(cmd1, argv, (pipeindex * sizeof(argv[0])));
	cmd1[pipeindex] = (char*) NULL;
	int argc1 = pipeindex;


	//get argv after pipe
	char **cmd2 = malloc(sizeof(argv));
	int argc2 = argc - pipeindex - 1;

	for (int i = 0; i < argc2; i++) {
		cmd2[i] = argv[i + pipeindex + 1];
	}
	cmd2[argc - pipeindex - 1] = (char*) NULL;

	if (pipeStuff(cmd1, argc1, cmd2, argc2) == 1) {
		return 1;
	}
	return -1;
}
//piping only works with external commands, and even then only sometimes works...
//cmd | cat works example but not much else...
int pipeStuff(char **argv1, int argc1, char **argv2, int argc2) {
	int fd[2];

	if (pipe(fd) == -1) {
		fprintf(stdout, "Pipe error\n");
		return -1;
	}

	pid_t pid1;
	pid_t pid2;


	if ((pid1 = fork()) == -1) {		//fork error
		fprintf(stdout, "Fork error\n");
		return -1;
	} else if (pid1 == 0) {	//child = source

		close(fd[0]);	//close read
		dup2(fd[1], STDOUT_FILENO);	//stdout to write end
		close(fd[1]);

		eval(argv1, argc1);

	}	else {	//parent
		if ((pid2 = fork()) == -1) {		//fork error
			fprintf(stdout, "Fork error\n");
			return -1;
		} else if (pid2 == 0){		//another child = destination
			close(fd[1]); //close write
			dup2(fd[0], STDIN_FILENO);	//read end to stdin
			close(fd[0]);

			eval(argv2, argc2);
		}

		wait(NULL);
		wait(NULL);
	}

	return 1; //piped
}

//frees all char*s in array of parsed arguments
void freeStrings(char **argv) {
	for (int i = 0; argv[i] != NULL; i++) {
		free(argv[i]);
	}
}
