#ifndef __UTIL_H__
#define __UTIL_H__

#define err(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__)
#define die(fmt, ...) do { fprintf(stderr, fmt, ##__VA_ARGS__); exit(1); } while(0)
#define LENGTH(A) (sizeof(A) / sizeof(A[0]))

/* linked list macros */
#define LIST_APPEND(LIST, NODE) \
	do { \
		if(!(LIST)) { \
			(LIST) = NODE; \
			(LIST)->next = NULL; \
			(LIST)->prev = NODE; \
		} else { \
			NODE->prev = (LIST)->prev; \
			NODE->next = NULL; \
			NODE->prev->next = NODE; \
			(LIST)->prev = NODE; \
		} \
	} while(0)

#define LIST_FOREACH(LIST, NODE) \
	for(NODE=LIST; NODE; NODE=NODE->next)
#endif
