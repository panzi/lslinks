#include <string.h>
#include <ctype.h>
#include <errno.h>
#include "css_tokenizer.h"

static bool print_qstr(const char *str, FILE *fp);

void lslinks_css_tokenizer_init(struct lslinks_css_tokenizer *tokenizer, const char *data, size_t length) {
	tokenizer->token  = LSLINKS_CSS_BOF;
	tokenizer->value  = NULL;
	tokenizer->number = 0;
	tokenizer->unit   = NULL;
	tokenizer->unirng_start = 0;
	tokenizer->unirng_end   = 0;
	tokenizer->data   = data;
	tokenizer->length = length;
	tokenizer->index  = 0;
	lslinks_bytes_init(&tokenizer->buffer);
}

void lslinks_css_tokenizer_cleanup(struct lslinks_css_tokenizer *tokenizer) {
	lslinks_bytes_cleanup(&tokenizer->buffer);
}

static const char *find(const char *needle, const char *start, const char *end) {
	// TODO
	return end;
}

enum lslinks_css_token lslinks_css_tokenizer_next(struct lslinks_css_tokenizer *tokenizer) {
	// TODO: http://dev.w3.org/csswg/css-syntax/
	
	const size_t index  = tokenizer->index;
	const size_t length = tokenizer->length;
	if (index >= length) {
		return (tokenizer->token = LSLINKS_CSS_EOF);
	}

	const char const *data = tokenizer->data;
	const char const *end  = data + length;
	const char *ptr = data + index;

	while (ptr < end) {
		if (isspace(*ptr)) {
			++ ptr;
		}
		else if (*ptr == '/') {
			const char *next = ptr + 1;
			if (next < end && *next == '*') {
				ptr = find("*/", ptr + 2, end);
				if (ptr >= end) {
					tokenizer->index = length;
					return (tokenizer->token = LSLINKS_CSS_ERROR);
				}
				ptr += 2;
			}

			// TODO
		}
	}

	if (ptr > data + index) {
		return (tokenizer->token = LSLINKS_CSS_WHITESPACE);
	}

	// TODO

	return LSLINKS_CSS_EOF;
}

const char *utf8_decode_codepoint(const char *str, size_t size, uint32_t *cpptr) {
	if (size == 0) {
		return NULL;
	}

	uint8_t byte = *str;

	if (byte >= 0xfc) {
		return NULL;
	}

	uint32_t cp = 0;

#define decode_next_byte \
		byte = *str; \
		if ((byte & 0xc0) != 0x80) { \
			return NULL; \
		} \
		cp <<= 6; \
		cp |= byte & 0x3f; \
		++ str;

	++ str;
	if ((byte & 0xfe) == 0xfc) {
		if (size < 6) {
			return NULL;
		}

		cp = byte & 0x01;

		decode_next_byte;
		decode_next_byte;
		decode_next_byte;
		decode_next_byte;
		decode_next_byte;
	}
	else if ((byte & 0xfc) == 0xf8) {
		if (size < 5) {
			return NULL;
		}

		cp = byte & 0x03;

		decode_next_byte;
		decode_next_byte;
		decode_next_byte;
		decode_next_byte;
	}
	else if ((byte & 0xf8) == 0xf0) {
		if (size < 4) {
			return NULL;
		}

		cp = byte & 0x07;

		decode_next_byte;
		decode_next_byte;
		decode_next_byte;
	}
	else if ((byte & 0xf0) == 0xe0) {
		if (size < 3) {
			return NULL;
		}

		cp = byte & 0x0f;

		decode_next_byte;
		decode_next_byte;
	}
	else if ((byte & 0xe0) == 0xc0) {
		if (size < 2) {
			return NULL;
		}

		cp = byte & 0x1f;

		decode_next_byte;
	}
	else {
		cp = byte;
	}

#undef decode_next_byte

	if (cpptr) *cpptr = cp;

	return str;
}

size_t utf8_encode_codepoint(uint32_t codepoint, char *buf, size_t size) {
	if (codepoint > 0x7FFFFFFF) {
		return 0;
	}

	size_t i;

#define encode_first_byte(prefix) \
		if (size > i) { \
			buf[i] = prefix | codepoint; \
		}

#define encode_prev_byte \
		if (size > i) { \
			buf[i] = 0xc0 | codepoint; \
		} \
		codepoint >>= 6; \
		-- i;

	if (codepoint > 0x3FFFFFF) {
		i = 5;
		encode_prev_byte;
		encode_prev_byte;
		encode_prev_byte;
		encode_prev_byte;
		encode_prev_byte;
		encode_first_byte(0xfc);
		return 6;
	}
	else if (codepoint > 0x1FFFFF) {
		i = 4;
		encode_prev_byte;
		encode_prev_byte;
		encode_prev_byte;
		encode_prev_byte;
		encode_first_byte(0xf8);
		return 5;
	}
	else if (codepoint > 0xFFFF) {
		i = 3;
		encode_prev_byte;
		encode_prev_byte;
		encode_prev_byte;
		encode_first_byte(0xf0);
		return 4;
	}
	else if (codepoint > 0x07FF) {
		i = 2;
		encode_prev_byte;
		encode_prev_byte;
		encode_first_byte(0xe0);
		return 3;
	}
	else if (codepoint > 0x007F) {
		i = 1;
		encode_prev_byte;
		encode_first_byte(0xc0);
		return 2;
	}
	else {
		if (size > 0) {
			buf[0] = codepoint;
		}
		return 1;
	}

#undef encode_first_byte
#undef encode_next_byte
}

bool print_qstr(const char *str, FILE *fp) {
	char qchar = '"';
	if (strchr(str, '"') != NULL) {
		qchar = '\'';
	}
	if (fputc(qchar, fp) == EOF) {
		return false;
	}
	const char *end = str + strlen(str);
	for (const char *ptr = str; *ptr;) {
		char ch = *ptr;

		if (ch == qchar || ch == '\\' || ch == '\n' || ch == '\r' || ch == '\f') {
			uint32_t cp = 0;
			const char *next = utf8_decode_codepoint(ptr, end - ptr, &cp);
			if (!next) {
				errno = EINVAL;
				return false;
			}
			else if (fprintf(fp, "\\%06X", cp) <= 0) {
				return false;
			}
			else {
				ptr = next;
			}
		}
		else if (fputc(ch, fp) == EOF) {
			return false;
		}
		else {
			++ ptr;
		}
	}
	return fputc(qchar, fp) != EOF;
}

bool lslinks_css_tokenizer_print(struct lslinks_css_tokenizer *tokenizer, FILE *fp) {
	switch (tokenizer->token) {
	case LSLINKS_CSS_ERROR:
	case LSLINKS_CSS_EOF:
	case LSLINKS_CSS_BOF:
		return false;

	case LSLINKS_CSS_WHITESPACE:
		return fputc(' ', fp) != EOF;

	case LSLINKS_CSS_IDENT:
		return fwrite(tokenizer->value, strlen(tokenizer->value), 1, fp) == 1;

	case LSLINKS_CSS_FUNCTION:
		return fprintf(fp, "%s(", tokenizer->value) > 0;

	case LSLINKS_CSS_AT_KEYWORD:
		return fprintf(fp, "@%s", tokenizer->value) > 0;

	case LSLINKS_CSS_HASH:
		return fprintf(fp, "#%s", tokenizer->value) > 0;

	case LSLINKS_CSS_STRING:
		return print_qstr(tokenizer->value, fp);

	case LSLINKS_CSS_URL:
		return fputs("url(", fp) <= 0 && print_qstr(tokenizer->value, fp) && fputc(')', fp) != EOF;

	case LSLINKS_CSS_NUMBER:
		return fprintf(fp, "%g", tokenizer->number) > 0;

	case LSLINKS_CSS_DIMENSION:
		return fprintf(fp, "%g%s", tokenizer->number, tokenizer->unit) > 0;

	case LSLINKS_CSS_PERCENTAGE:
		return fprintf(fp, "%g%%", tokenizer->number) > 0;

	case LSLINKS_CSS_UNICODE_RANGE:
		return fprintf(fp, "U+%06X-%06X", tokenizer->unirng_start, tokenizer->unirng_end) > 0;

	case LSLINKS_CSS_INCLUDE_MATCH:
		return fputs("~=", fp) > 0;

	case LSLINKS_CSS_DASH_MATCH:
		return fputs("|=", fp) > 0;

	case LSLINKS_CSS_PREFIX_MATCH:
		return fputs("^=", fp) > 0;

	case LSLINKS_CSS_SUFFIX_MATCH:
		return fputs("$=", fp) > 0;

	case LSLINKS_CSS_SUBSTRING_MATCH:
		return fputs("*=", fp) > 0;

	case LSLINKS_CSS_COLUMN:
		return fputs("||", fp) > 0;

	case LSLINKS_CSS_CDO:
		return fputs("<!--", fp) > 0;

	case LSLINKS_CSS_CDC:
		return fputs("-->", fp) > 0;

	case LSLINKS_CSS_COLON:
		return fputc(':', fp) != EOF;

	case LSLINKS_CSS_SEMICOLON:
		return fputc(';', fp) != EOF;

	case LSLINKS_CSS_COMMA:
		return fputc(',', fp) != EOF;

	case LSLINKS_CSS_LEFT_SQUARE:
		return fputc('[', fp) != EOF;

	case LSLINKS_CSS_RIGHT_SQUARE:
		return fputc(']', fp) != EOF;

	case LSLINKS_CSS_LEFT_PAREN:
		return fputc('(', fp) != EOF;

	case LSLINKS_CSS_RIGHT_PAREN:
		return fputc(')', fp) != EOF;

	case LSLINKS_CSS_LEFT_CURLY:
		return fputc('{', fp) != EOF;

	case LSLINKS_CSS_RIGHT_CURLY:
		return fputc('}', fp) != EOF;

	}
	return false;
}
