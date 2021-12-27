#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
int main(void)
{
	pid_t childpid1;
	pid_t childpid2;
	if ((childpid1=fork())==-1) 
	{
		perror("Fork error");
		exit(0);
	}
	
	if (childpid1 == 0)
	{
		printf("\nchild1: selfpid = %d  group = %d  parent pid = %d\n", getpid(), getpgrp(), getppid());
		return 0;
	}
	
	if ((childpid2=fork())==0) 
	{
		printf("\nchild2:   selfpid = %d   group = %d   parent pid = %d\n", getpid(), getpgrp(), getppid());
		return 0;
	}
		
	printf("parent: selfpid = %d group = %d child1 pid = %d child2 pid = %d\n", getpid(), getpgrp(), childpid1, childpid2);
	int status;
	pid_t pid = wait(&status);
	printf("\nchild has finished: PID = %d status = %d ", pid, status);
	if (WIFEXITED(status))
		printf(" exit code = %d\n", WEXITSTATUS(status));
	pid = wait(&status);
	printf("\nchild has finished: PID = %d status = %d ", pid, status);
	if (WIFEXITED(status))
		printf("exit code = %d\n", WEXITSTATUS(status));
			
	return EXIT_SUCCESS;
}
