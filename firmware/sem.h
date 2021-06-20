#ifndef SEM_H_
#define SEM_H_

#include <stdint.h>

typedef struct __sem {
	uint32_t value;
} sem_t;

void sem_wait(sem_t *);
void sem_post(sem_t *);

#endif // SEM_H_
