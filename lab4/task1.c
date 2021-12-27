#include <sys/types.h>
#include <unistd.h>

int main(void)
{
	pid_t childpid1, childpid2;
	if ((childpid1=fork())==-1) 
	{
		perror("Fork error");
		exit(0);
	}
	
	if (childpid1 == 0)
	{
		printf("\nchild1:  selfpid = %d group = %d  parent pid = %d\n", getpid(), getpgrp(), getppid());
		sleep(2);
		printf("\nchild1:  selfpid = %d group = %d  parent pid = %d\n", getpid(), getpgrp(), getppid());
		return 0;
	}
	if ((childpid2=fork()) == 0)
	{
		printf("\nchild2:  selfpid = %d group = %d  parent pid = %d\n", getpid(), getpgrp(), getppid());
		sleep(2);
		printf("\nchild2:  selfpid = %d group = %d  parent pid = %d\n", getpid(), getpgrp(), getppid());
		return 0;
	}
	
	printf("parent:\n selfpid = %d\n group = %d\n child1 pid = %d\n child2 pid = %d\n\n", getpid(), getpgrp(), childpid1, childpid2);
	sleep(1);
	return 0;
}
