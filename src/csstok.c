#include <stdio.h>
#include <stdlib.h>

#include "css_tokenizer.h"

bool print_css_tokens(FILE *fp) {
	struct lslinks_bytes buffer = LSLINKS_BYTES_INIT;
	struct lslinks_css_tokenizer tokenizer;

	if (!lslinks_bytes_readall(&buffer, fp)) {
		lslinks_bytes_cleanup(&buffer);
		return false;
	}

	lslinks_css_tokenizer_init(&tokenizer, buffer.data, buffer.size);

	while (lslinks_css_tokenizer_next(&tokenizer) != LSLINKS_CSS_EOF) {
		if (tokenizer.token == LSLINKS_CSS_ERROR ||
		    !lslinks_css_tokenizer_print(&tokenizer, stdout)) {
			lslinks_css_tokenizer_cleanup(&tokenizer);
			lslinks_bytes_cleanup(&buffer);
			return false;
		}
	}

	lslinks_css_tokenizer_cleanup(&tokenizer);
	lslinks_bytes_cleanup(&buffer);
	return true;
}

// const char *utf8_decode_codepoint(const char *str, size_t size, uint32_t *cpptr);

int main (int argc, char *argv[]) {
	/*
	struct lslinks_bytes buffer = LSLINKS_BYTES_INIT;
	
	if (!lslinks_bytes_readall(&buffer, stdin)) {
		lslinks_bytes_cleanup(&buffer);
		return EXIT_FAILURE;
	}

	const char *ptr = buffer.data;
	const char *end = ptr + buffer.size;

	while (ptr < end) {
		uint32_t cp = 0;
		const char *next = utf8_decode_codepoint(ptr, end - ptr, &cp);
		if (next) {
			fprintf(stdout, "U+%06X\n", cp);
			ptr = next;
		}
		else {
			uint8_t byte = *ptr;
			fprintf(stderr, "illegal UTF-8 code point: 0x%02X\n", (unsigned int)byte);
			lslinks_bytes_cleanup(&buffer);
			return EXIT_FAILURE;
		}
	}

	lslinks_bytes_cleanup(&buffer);
	*/

	if (argc > 1) {
		for (size_t i = 1; i < argc; ++ i) {
			FILE *fp = fopen(argv[i], "rb");
			if (!fp) {
				perror(argv[i]);
				return EXIT_FAILURE;
			}
			if (!print_css_tokens(fp)) {
				perror(argv[i]);
				fclose(fp);
				return EXIT_FAILURE;
			}
			fclose(fp);
		}
	}
	else if (!print_css_tokens(stdin)) {
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
