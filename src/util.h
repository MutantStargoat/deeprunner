#ifndef UTIL_H_
#define UTIL_H_

#include <stdlib.h>

#if defined(__WATCOMC__) || defined(_WIN32) || defined(__DJGPP__)
#include <malloc.h>
#else
#include <alloca.h>
#endif

/* Non-failing versions of malloc/calloc/realloc. They never return 0, they call
 * demo_abort on failure. Use the macros, don't call the *_impl functions.
 */
#define malloc_nf(sz)	malloc_nf_impl(sz, __FILE__, __LINE__)
void *malloc_nf_impl(size_t sz, const char *file, int line);
#define calloc_nf(n, sz)	calloc_nf_impl(n, sz, __FILE__, __LINE__)
void *calloc_nf_impl(size_t num, size_t sz, const char *file, int line);
#define realloc_nf(p, sz)	realloc_nf_impl(p, sz, __FILE__, __LINE__)
void *realloc_nf_impl(void *p, size_t sz, const char *file, int line);
#define strdup_nf(s)	strdup_nf_impl(s, __FILE__, __LINE__)
char *strdup_nf_impl(const char *s, const char *file, int line);

int match_prefix(const char *str, const char *prefix);

#endif	/* UTIL_H_ */
