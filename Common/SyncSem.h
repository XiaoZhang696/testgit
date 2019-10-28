#include <semaphore.h>
#include <fcntl.h>
#include <Log.h>
#define SEM_NAME "PROGRESS_SEM"

int semunlink();
int semopen();
void sigwait();
void sigrelease();
int semclose();

