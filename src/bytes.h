#ifndef LSLINKS_BYTES_H__
#define LSLINKS_BYTES_H__

#include <stdio.h>
#include "common.h"

struct lslinks_bytes {
	char *data;
	size_t size;
	size_t capacity;
};

#define LSLINKS_BYTES_INIT { NULL, 0, 0 }

void lslinks_bytes_init(struct lslinks_bytes *bytes);
void lslinks_bytes_cleanup(struct lslinks_bytes *bytes);
bool lslinks_bytes_ensure(struct lslinks_bytes *bytes, size_t size);
bool lslinks_bytes_append(struct lslinks_bytes *bytes, void *data, size_t size);
bool lslinks_bytes_readall(struct lslinks_bytes *bytes, FILE *fp);

char *lslinks_readall(FILE *fp);

#endif // LSLINKS_BYTES_H__
