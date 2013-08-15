#include <stdlib.h>
#include <string.h>
#include "bytes.h"

void lslinks_bytes_init(struct lslinks_bytes *bytes) {
	bytes->data = NULL;
	bytes->size = 0;
	bytes->capacity = 0;
}

void lslinks_bytes_cleanup(struct lslinks_bytes *bytes) {
	free(bytes->data);
	bytes->data = NULL;
	bytes->size = 0;
	bytes->capacity = 0;
}

bool lslinks_bytes_ensure(struct lslinks_bytes *bytes, size_t size) {
	size_t new_size = bytes->size + size;
	if (new_size > bytes->capacity) {
		size_t rem = new_size % BUFSIZ;
		size_t new_cap = new_size;
		if (rem) {
			new_cap += BUFSIZ - rem;
		}
		char *ptr = realloc(bytes->data, new_cap);
		if (!ptr) return false;
		bytes->data = ptr;
		bytes->capacity = new_cap;
	}
	return true;
}

bool lslinks_bytes_append(struct lslinks_bytes *bytes, void *data, size_t size) {
	if (!lslinks_bytes_ensure(bytes, size)) {
		return false;
	}
	memcpy(bytes->data + bytes->size, data, size);
	bytes->size += size;
	return true;
}

bool lslinks_bytes_readall(struct lslinks_bytes *bytes, FILE *fp) {
	while (!feof(fp)) {
		if (!lslinks_bytes_ensure(bytes, BUFSIZ)) {
			return false;
		}

		size_t rem = bytes->capacity - bytes->size;
		size_t count = fread(bytes->data + bytes->size, 1, rem, fp);
		bytes->size += count;
		if (count < rem) {
			if (ferror(fp)) {
				return false;
			}
		}
	}

	return true;
}

char *lslinks_readall(FILE *fp) {
	struct lslinks_bytes bytes = LSLINKS_BYTES_INIT;
	
	if (!lslinks_bytes_readall(&bytes, fp) || !lslinks_bytes_append(&bytes, "", 1)) {
		lslinks_bytes_cleanup(&bytes);
		return NULL;
	}

	return bytes.data;
}
