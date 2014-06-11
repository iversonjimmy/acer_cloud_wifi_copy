#ifndef __NOTIFY_H__
#define __NOTIFY_H__

#include "list.h"

typedef void *nb_arg_t;
typedef nb_arg_t (*nb_func_t)(vm_t *, nb_arg_t, nb_arg_t);

typedef struct {
	nb_func_t	nb_func;
	nb_arg_t	nb_arg0;
	nb_arg_t	nb_arg1;
	list_t		nb_list[MAX_CPUS];
	u32		nb_completion_count;
} note_t;

extern void notify_init(void);
extern void notify_all(nb_func_t, nb_arg_t, nb_arg_t);

#endif
