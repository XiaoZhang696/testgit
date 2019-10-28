#include <error.h>
#include <errno.h>
#include <string.h>
sem_t* g_semid = 0;
sem_t* Initsem(int iValue)
{
	sem_t* semid;
	semid = sem_open(SEM_NAME,O_CREATE,0644,iValue);
	if(semid == SEM_FAILED)
	{
		ZERR("sem %s open failed,err:%s\n"SEM_NAME,strerror(errno));
	}
	else
	{
		ZDBG("sem %s open success,semid is %x\n",semid);
	}
	return semid;
}
void semp(sem_t* semid)
{
	ZDBG("begin wait sem %x\n",semid);
	sem_wait(semid);
	ZDBG("sem %x wait success\n",semid);
}
void semv(sem_t* semid)
{
	ZDBG("begin release sem %x\n",semid);
	sem_post(semid);
	ZDBG("sem %x release success\n",semid);
}

int semdestroy(sem_t* semid)
{
	ZDBG("sem %x destroy\n",semid);
	return sem_destroy(semid);
}
int semunlink()
{
	return sem_unlink(SEM_NAME)
}


int semopen()
{
	g_semid = Initsem(1);
	if(semid == SEM_FAILED)
	{
		return -1;
	}
	else{
		return 1;
	}
}
void sigwait()
{
	semp(g_semid);
}
void sigrelease()
{
	semv(g_semid);
}
int semclose()
{
	return semdestroy(g_semid);
}
