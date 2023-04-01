#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "util.h"

void *malloc_nf_impl(size_t sz, const char *file, int line)
{
	void *p;
	if(!(p = malloc(sz))) {
		fprintf(stderr, "%s:%d failed to allocate %lu bytes\n", file, line, (unsigned long)sz);
		abort();
	}
	return p;
}

void *calloc_nf_impl(size_t num, size_t sz, const char *file, int line)
{
	void *p;
	if(!(p = calloc(num, sz))) {
		fprintf(stderr, "%s:%d failed to allocate %lu bytes\n", file, line, (unsigned long)(num * sz));
		abort();
	}
	return p;
}

void *realloc_nf_impl(void *p, size_t sz, const char *file, int line)
{
	if(!(p = realloc(p, sz))) {
		fprintf(stderr, "%s:%d failed to realloc %lu bytes\n", file, line, (unsigned long)sz);
		abort();
	}
	return p;
}

char *strdup_nf_impl(const char *s, const char *file, int line)
{
	int len;
	char *res;

	len = strlen(s);
	if(!(res = malloc(len + 1))) {
		fprintf(stderr, "%s:%d failed to duplicate string\n", file, line);
		abort();
	}
	memcpy(res, s, len + 1);
	return res;
}


int match_prefix(const char *str, const char *prefix)
{
	while(*str && *prefix) {
		if(*str++ != *prefix++) {
			return 0;
		}
	}
	return *prefix ? 0 : 1;
}
