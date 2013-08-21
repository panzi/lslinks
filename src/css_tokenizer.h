#ifndef LSLINKS_CSS_TOKENIZER_H__
#define LSLINKS_CSS_TOKENIZER_H__

#include <stdint.h>
#include <stdio.h>

#include "bytes.h"

enum lslinks_css_token {
	LSLINKS_CSS_ERROR = -2, // only possible error: ENOMEM
	LSLINKS_CSS_EOF   = -1,
	LSLINKS_CSS_BOF   =  0,
	LSLINKS_CSS_WHITESPACE,
	LSLINKS_CSS_IDENT,
	LSLINKS_CSS_FUNCTION,
	LSLINKS_CSS_AT_KEYWORD,
	LSLINKS_CSS_HASH,
	LSLINKS_CSS_STRING, // also bad-string
	LSLINKS_CSS_URL,    // also bad-url
	LSLINKS_CSS_NUMBER,
	LSLINKS_CSS_DIMENSION,
	LSLINKS_CSS_PERCENTAGE,
	LSLINKS_CSS_UNICODE_RANGE,
	LSLINKS_CSS_INCLUDE_MATCH,
	LSLINKS_CSS_DASH_MATCH,
	LSLINKS_CSS_PREFIX_MATCH,
	LSLINKS_CSS_SUFFIX_MATCH,
	LSLINKS_CSS_SUBSTRING_MATCH,
	LSLINKS_CSS_COLUMN,
	LSLINKS_CSS_CDO,
	LSLINKS_CSS_CDC,
	LSLINKS_CSS_COLON,
	LSLINKS_CSS_SEMICOLON,
	LSLINKS_CSS_COMMA,
	LSLINKS_CSS_LEFT_SQUARE,
	LSLINKS_CSS_RIGHT_SQUARE,
	LSLINKS_CSS_LEFT_PAREN,
	LSLINKS_CSS_RIGHT_PAREN,
	LSLINKS_CSS_LEFT_CURLY,
	LSLINKS_CSS_RIGHT_CURLY
};

struct lslinks_css_tokenizer {
	enum lslinks_css_token token;
	union {
		const char *value;
		struct {
			double number;
			const char *unit;
		};
		struct {
			uint32_t unirng_start;
			uint32_t unirng_end;
		};
	};

	const char *data;
	size_t      length;
	size_t      index;
	struct lslinks_bytes buffer;
};

void lslinks_css_tokenizer_init(struct lslinks_css_tokenizer *tokenizer, const char *data, size_t length);
void lslinks_css_tokenizer_cleanup(struct lslinks_css_tokenizer *tokenizer);
enum lslinks_css_token lslinks_css_tokenizer_next(struct lslinks_css_tokenizer *tokenizer);
bool lslinks_css_tokenizer_print(struct lslinks_css_tokenizer *tokenizer, FILE *fp);

#endif
