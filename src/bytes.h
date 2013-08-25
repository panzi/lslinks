#ifndef LSLINKS_BYTES_H__
#define LSLINKS_BYTES_H__

#include <stdio.h>
#include "common.h"

struct lslinks_bytes {
	char *data;
	size_t size;
	size_t capacity;
	size_t increment;
};

#define LSLINKS_BYTES_INIT { NULL, 0, 0, BUFSIZ }

void lslinks_bytes_init(struct lslinks_bytes *bytes);
bool lslinks_bytes_init2(struct lslinks_bytes *bytes, size_t increment);
void lslinks_bytes_cleanup(struct lslinks_bytes *bytes);
bool lslinks_bytes_ensure(struct lslinks_bytes *bytes, size_t size);
bool lslinks_bytes_append(struct lslinks_bytes *bytes, const void *data, size_t size);
bool lslinks_bytes_append_char(struct lslinks_bytes *bytes, char ch);
bool lslinks_bytes_append_str(struct lslinks_bytes *bytes, const char *str);
bool lslinks_bytes_append_nil(struct lslinks_bytes *bytes);
bool lslinks_bytes_readall(struct lslinks_bytes *bytes, FILE *fp);

char *lslinks_readall(FILE *fp);
char *lslinks_strcat(const char *str1, const char *str2);

#endif // LSLINKS_BYTES_H__
