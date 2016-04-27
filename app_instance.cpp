#include <CoreWindow.h>
#include <stdio.h>

#define MAX_INSTANCES 100


/* Get the application instance by creating a named semamphore.  If the semaphore already
 * exists then that application instance is running.
 * The first instance number is 0, then 1, 2, ...
 * If any errors, the function return is non-zero and the application instance returned is 0.
 */
int get_app_instance(char *name, int *app_instance)
{
	int i;
	DWORD errornum;
	HANDLE h_named_sem;
	char sem_name[100];

	*app_instance = 0;
	for (i = 0; i < MAX_INSTANCES; ++i) {
		sprintf(sem_name, "%s_sem%d", name, i);
		h_named_sem = CreateSemaphore(0, 1, 1, sem_name);
		if (!h_named_sem)
			return(1);			/* Cannot create handle to new or existing semaphore. */

		errornum = GetLastError();
		if (!errornum) {
			*app_instance = i;
			return(0);
		}
		else
			CloseHandle(h_named_sem);	/* Semaphore already exists. */
	}

	/* Too many instances. */
	return(1);
}
