#include "sem.h"
#include "task.h"
#include <stddef.h>

void sem_wait(sem_t *sem)
{
	uint32_t val;
	//task_func_t func;
	do {
		for (val = __ldrex(&sem->value); val == 0; val = __ldrex(&sem->value)) {
			/*if ((func = get_task()) != NULL) {
				func();
			}*/
			// надо бы сюда добавить функции для того чтобы планировщик знал что какой-то процесс тупит в ожидании семафора
		}
		--val;
	} while(__strex(val, &sem->value) == 1);
}

void sem_post(sem_t *sem)
{
	uint32_t val;
	do {
		val = __ldrex(&sem->value);
		++val;
	} while (__strex(val, &sem->value) == 1);
}
