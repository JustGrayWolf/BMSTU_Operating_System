#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(void)
{
	pid_t childpid1;
	pid_t childpid2;
	int fd[2];
	
	if(pipe(fd)!=0)
	{
        	perror("Pipe error");
        	exit(0);
    	}
    	
	if ((childpid1=fork())==-1 ) 
	{
		perror("Fork error");
		exit(0);
	}
	
	if (childpid1 == 0)
	{
		char *msg = "12345 54321 ";
		int msglen = strlen(msg) + 1;
		close(fd[0]);
		write(fd[1], msg, msglen);
		return 0;
	}
	
	if ((childpid2 = fork()) == 0)
	{
		char *msg = "fdfb,e,vdfkb,kgrb k,fktr,kfggtrg";
		int msglen = strlen(msg) + 1;
		close(fd[0]);
		write(fd[1], msg, msglen);
		return 0;
	}
	
	char *str[256];
	close(fd[1]);
	int size = read(fd[0], str, sizeof(str));
	printf("%s\n",str); 
	close(fd[1]);
	size = read(fd[0], str, sizeof(str));
	printf("%s\n",str);
	int status;
	pid_t pid = wait(&status);
	printf("\nchild has finished: PID = %d status = %d ", pid, status);
	if (WIFEXITED(status))
		printf("exit code = %d\n", WEXITSTATUS(status));
	pid = wait(&status);
	printf("\nchild has finished: PID = %d status = %d ", pid, status);
	if (WIFEXITED(status))
		printf("exit code = %d\n", WEXITSTATUS(status));
	close(fd[0]);
	
	return EXIT_SUCCESS;
}
