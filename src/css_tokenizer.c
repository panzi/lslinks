#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>

#include "css_tokenizer.h"

static bool print_qstr(const char *str, FILE *fp);
// static const char *findstr(const char *needle, const char *start, const char *end);
// static const char *skipws(const char *data, size_t size);

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

/*
const char *skipws(const char *data, size_t size) {
	const char *ptr = data;
	const char *end = data + size;
	lslinks_codepoint cp = 0;

	while (ptr < end) {
		const char *next = lslinks_utf8_decode_codepoint_css(ptr, end - ptr, &cp);

		if (!next || !LSLINKS_IS_SPACE(cp)) {
			break;
		}
		
		ptr = next;
	}

	return ptr;
}
*/

const char *skip_ws_and_comment(const char *start, const char *end) {
	// because all related characters are 7-bit ASCII and discarded anyway
	// I can ignore the fact that it's actually UTF-8
	const char *ptr = start;
	const char *cmt_end = end - 1;

	while (ptr < end) {
		char ch = *ptr;
		if (ch == '/' && ptr < cmt_end && ptr[1] == '*') {
			ptr += 2;
			for (;;) {
				if (ptr >= cmt_end) {
					ptr = end;
					break;
				}
				else if (ptr[0] == '*' && ptr[1] == '/') {
					ptr += 2;
					break;
				}
				++ ptr;
			}
		}
		else if (!LSLINKS_IS_SPACE(ch)) {
			break;
		}
		else {
			++ ptr;
		}
	}

	return ptr;
}

bool get_number(struct lslinks_css_tokenizer *tokenizer) {
	const char *start = tokenizer->data + tokenizer->index;
	const char *end   = tokenizer->data + tokenizer->length;
	const char *ptr   = start;

	if (ptr >= end) {
		return false;
	}

	char ch = *ptr;
	bool neg = false;
	if (ch == '-' || ch == '+') {
		neg = ch == '-';
		++ ptr;

		if (ptr >= end) {
			tokenizer->index = ptr - tokenizer->data;
			return false;
		}

		ch = *ptr;
	}

	uint64_t number   = 0;
	uint64_t decimal  = 0;
	size_t   decimal_places = 0;
	int64_t  exponent = 0;

	// TODO: check for overflow
	if (LSLINKS_IS_DIGIT(ch)) {
		do {
			number *= 10;
			number += *ptr - '0';
			++ ptr;
		} while (ptr < end && LSLINKS_IS_DIGIT(*ptr));
	}
	
	if (ptr < end && *ptr == '.') {
		++ ptr;

		if (ptr >= end || !LSLINKS_IS_DIGIT(*ptr)) {
			tokenizer->index = ptr - tokenizer->data;
			return false;
		}

		do {
			decimal *= 10;
			decimal += *ptr - '0';
			++ decimal_places;
			++ ptr;
		} while (ptr < end && LSLINKS_IS_DIGIT(*ptr));
	}

	if (ptr < end && (*ptr == 'e' || *ptr == 'E')) {
		++ ptr;
		
		if (ptr >= end) {
			tokenizer->index = ptr - tokenizer->data;
			return false;
		}

		bool exp_neg = false;
		if (*ptr == '-' || *ptr == '+') {
			exp_neg = *ptr == '-';
			++ ptr;

			if (ptr >= end) {
				tokenizer->index = ptr - tokenizer->data;
				return false;
			}

			ch = *ptr;
		}

		if (ptr >= end || !LSLINKS_IS_DIGIT(*ptr)) {
			tokenizer->index = ptr - tokenizer->data;
			return false;
		}

		do {
			exponent *= 10;
			exponent += *ptr - '0';
			++ ptr;
		} while (ptr < end && LSLINKS_IS_DIGIT(*ptr));

		if (exp_neg) {
			exponent = -exponent;
		}
	}

	// TODO
	// tokenizer->number = ...

	return true;
}

bool get_escape(struct lslinks_css_tokenizer *tokenizer) {
	if (tokenizer->index >= tokenizer->length) {
		errno = EINVAL;
		return false;
	}

	const char *start = tokenizer->data + tokenizer->index;
	const char *end   = tokenizer->data + tokenizer->length;
	const char *ptr   = start;
	lslinks_codepoint cp = 0;
	lslinks_codepoint ch = 0;

	ptr = lslinks_utf8_decode_codepoint_css(ptr, end - ptr, &ch);
	
	if (LSLINKS_IS_HEX(ch)) {
		cp = LSLINKS_UNHEX(ch);

		while (ptr < end && ptr - start < 6) {
			const char *next = lslinks_utf8_decode_codepoint_css(ptr, end - ptr, &ch);
			if (LSLINKS_IS_HEX(ch)) {
				cp *= 16;
				cp += LSLINKS_UNHEX(ch);
				ptr = next;
			}
			else if (LSLINKS_IS_SPACE(ch)) {
				ptr = next;
				break;
			}
			else {
				break;
			}
		}
	}
	else if (ch == LSLINKS_U_LINE_FEED) {
		tokenizer->index = tokenizer->data - ptr;
		errno = EINVAL;
		return false;
	}
	else {
		cp = ch;
	}

	if (LSLINKS_IS_SURROGATE(cp)) {
		cp = LSLINKS_U_REPLACEMENT;
	}

	tokenizer->index = tokenizer->data - ptr;
	return lslinks_bytes_append_utf8_char(&tokenizer->buffer, cp);
}

bool get_ident(struct lslinks_css_tokenizer *tokenizer) {
	lslinks_codepoint cp = 0;
	struct lslinks_bytes *buf = &tokenizer->buffer;
	const char *start = tokenizer->data + tokenizer->index;
	const char *end   = tokenizer->data + tokenizer->length;
	const char *ptr   = lslinks_utf8_decode_codepoint_css(start, end - start, &cp);

	if (cp == '-') {
		if (ptr >= end) {
			tokenizer->index = ptr - tokenizer->data;
			errno = EINVAL;
			return false;
		}
		
		if (!lslinks_bytes_append_char(buf, '-')) {
			tokenizer->index = ptr - tokenizer->data;
			return false;
		}

		ptr = lslinks_utf8_decode_codepoint_css(ptr, end - ptr, &cp);
	}

	if (!LSLINKS_IS_IDENT_START2(cp)) {
		if (!lslinks_bytes_append_utf8_char(buf, cp)) {
			tokenizer->index = ptr - tokenizer->data;
			return false;
		}
	}
	else if (cp == '\\') {
		tokenizer->index = ptr - tokenizer->data;
		if (!get_escape(tokenizer)) {
			return false;
		}
		ptr = tokenizer->data + tokenizer->index;
	}
	else {
		tokenizer->index = ptr - tokenizer->data;
		errno = EINVAL;
		return false;
	}

	while (ptr < end) {
		const char *next = lslinks_utf8_decode_codepoint_css(ptr, end - ptr, &cp);

		if (LSLINKS_IS_IDENT(cp)) {
			ptr = next;
			tokenizer->index = ptr - tokenizer->data;
			if (!lslinks_bytes_append_utf8_char(buf, cp)) {
				return false;
			}
		}
		else if (cp == '\\') {
			tokenizer->index = ptr - tokenizer->data;
			if (!get_escape(tokenizer)) {
				return false;
			}
			ptr = tokenizer->data + tokenizer->index;
		}
		else {
			break;
		}
	}

	return true;
}

/*
const char *findstr(const char *needle, const char *start, const char *end) {
	size_t needle_len = strlen(needle);
	if (needle + needle_len >= end) {
		return end;
	}

	const char *last = end - needle_len;
	for (const char *ptr = start; ptr <= last; ++ ptr) {
		if (memcmp(ptr, needle, needle_len) == 0) {
			return ptr;
		}
	}

	return end;
}
*/

enum lslinks_css_token lslinks_css_tokenizer_next(struct lslinks_css_tokenizer *tokenizer) {
	// TODO: http://dev.w3.org/csswg/css-syntax/
	
	const size_t index  = tokenizer->index;
	const size_t length = tokenizer->length;
	if (index >= length) {
		return (tokenizer->token = LSLINKS_CSS_EOF);
	}

	const char const *data = tokenizer->data;
	const char const *end  = data + length;
	const char *ptr = skip_ws_and_comment(data + index, end);

	if (ptr > data + index) {
		tokenizer->index = ptr - data;
		return (tokenizer->token = LSLINKS_CSS_WHITESPACE);
	}

	if (ptr >= data + length) {
		tokenizer->index = ptr - data;
		return (tokenizer->token = LSLINKS_CSS_EOF);
	}

	lslinks_codepoint cp = 0;
	const char *next = lslinks_utf8_decode_codepoint_css(ptr, end - ptr, &cp);
	struct lslinks_bytes *buf = &tokenizer->buffer;

	buf->size = 0;
	if ((cp == 'U' || cp == 'u') && next < end && *next == '+') {
		// TODO
	}
	else if (LSLINKS_IS_IDENT_START(cp) || cp == '\\') {
		tokenizer->index = next - data;
		if (!get_ident(tokenizer)) {
			return (tokenizer->token = LSLINKS_CSS_ERROR);
		}
		ptr = tokenizer->data + tokenizer->index;

		if (ptr < end && *ptr == '(') {
			++ ptr;
			if (strncmp("url", buf->data, buf->size) == 0) {
				buf->size = 0;
				// TODO: parse url
			}
			else {
				tokenizer->index = ptr - data;
				if (!lslinks_bytes_append_nil(buf)) {
					return (tokenizer->token = LSLINKS_CSS_ERROR);
				}
				return (tokenizer->token = LSLINKS_CSS_FUNCTION);
			}
		}
		else {
			tokenizer->index = ptr - data;
			// TODO: might still be url-unquoted
			if (!lslinks_bytes_append_nil(buf)) {
				return (tokenizer->token = LSLINKS_CSS_ERROR);
			}
			tokenizer->value = buf->data;
			return (tokenizer->token = LSLINKS_CSS_IDENT);
		}
	}
	else if (cp == ':') {
		tokenizer->index = next - data;
		return (tokenizer->token = LSLINKS_CSS_COLON);
	}
	else if (cp == ',') {
		tokenizer->index = next - data;
		return (tokenizer->token = LSLINKS_CSS_COMMA);
	}
	else if (cp == ';') {
		tokenizer->index = next - data;
		return (tokenizer->token = LSLINKS_CSS_SEMICOLON);
	}
	else if (cp == '(') {
		tokenizer->index = next - data;
		return (tokenizer->token = LSLINKS_CSS_LEFT_PAREN);
	}
	else if (cp == ')') {
		tokenizer->index = next - data;
		return (tokenizer->token = LSLINKS_CSS_RIGHT_PAREN);
	}
	else if (cp == '[') {
		tokenizer->index = next - data;
		return (tokenizer->token = LSLINKS_CSS_LEFT_SQUARE);
	}
	else if (cp == ']') {
		tokenizer->index = next - data;
		return (tokenizer->token = LSLINKS_CSS_RIGHT_SQUARE);
	}
	else if (cp == '{') {
		tokenizer->index = next - data;
		return (tokenizer->token = LSLINKS_CSS_LEFT_CURLY);
	}
	else if (cp == '}') {
		tokenizer->index = next - data;
		return (tokenizer->token = LSLINKS_CSS_RIGHT_CURLY);
	}
	else if (cp == '>') {
		tokenizer->index = next - data;
		return (tokenizer->token = LSLINKS_CSS_GREATER_THAN);
	}
	else if (cp == '~') {
		if (next >= end && *next == '=') {
			tokenizer->index = next + 1 - data;
			return (tokenizer->token = LSLINKS_CSS_INCLUDE_MATCH);
		}
		else {
			tokenizer->index = next - data;
			return (tokenizer->token = LSLINKS_CSS_TILDE);
		}
	}
	else if (cp == '*') {
		if (next >= end && *next == '=') {
			tokenizer->index = next + 1 - data;
			return (tokenizer->token = LSLINKS_CSS_SUBSTRING_MATCH);
		}
		else {
			tokenizer->index = next - data;
			return (tokenizer->token = LSLINKS_CSS_ASTERISK);
		}
	}
	else if (LSLINKS_IS_NUMBER_START(cp)) {
		tokenizer->index = next - data;
		if (cp == '-' && next < end && *next == '-') {
			ptr = next + 1;
			tokenizer->index = ptr - data;
			if (ptr >= end || *ptr != '>') {
				return (tokenizer->token = LSLINKS_CSS_ERROR);
			}
			else {
				return (tokenizer->token = LSLINKS_CSS_CDC);
			}
		}
		else if (cp == '+' && next < end && !LSLINKS_IS_NUMBER_START2(*next)) {
			return (tokenizer->token = LSLINKS_CSS_PLUS);
		}
		else if (!get_number(tokenizer)) {
			return (tokenizer->token = LSLINKS_CSS_ERROR);
		}
	}
	else if (next >= end) {
		tokenizer->index = next - data;
		return (tokenizer->token = LSLINKS_CSS_ERROR);
	}
	else if (cp == '@') {
		tokenizer->index = next - data;
		if (!get_ident(tokenizer) || !lslinks_bytes_append_nil(buf)) {
			return (tokenizer->token = LSLINKS_CSS_ERROR);
		}

		tokenizer->value = buf->data;
		return (tokenizer->token = LSLINKS_CSS_AT_KEYWORD);
	}
	else if (cp == '#') {
		tokenizer->index = next - data;
		if (!get_ident(tokenizer) || !lslinks_bytes_append_nil(buf)) {
			return (tokenizer->token = LSLINKS_CSS_ERROR);
		}

		tokenizer->value = buf->data;
		return (tokenizer->token = LSLINKS_CSS_HASH);
	}
	else if (cp == '"') {
		// TODO
	}
	else if (cp == '\'') {
		// TODO
	}
	else if (cp == '|') {
		if (*next == '=') {
			tokenizer->index = next + 1 - data;
			return (tokenizer->token = LSLINKS_CSS_DASH_MATCH);
		}
		else {
			tokenizer->index = next - data;
			return (tokenizer->token = LSLINKS_CSS_ERROR);
		}
	}
	else if (cp == '^') {
		if (*next == '=') {
			tokenizer->index = next + 1 - data;
			return (tokenizer->token = LSLINKS_CSS_PREFIX_MATCH);
		}
		else {
			tokenizer->index = next - data;
			return (tokenizer->token = LSLINKS_CSS_ERROR);
		}
	}
	else if (cp == '$') {
		if (*next == '=') {
			tokenizer->index = next + 1 - data;
			return (tokenizer->token = LSLINKS_CSS_SUFFIX_MATCH);
		}
		else {
			tokenizer->index = next - data;
			return (tokenizer->token = LSLINKS_CSS_ERROR);
		}
	}
	else if (cp == '<') {
		// TODO
	}
	else {
		// TODO
	}

	// TODO

	return LSLINKS_CSS_EOF;
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
			lslinks_codepoint cp = 0;
			const char *next = lslinks_utf8_decode_codepoint(ptr, end - ptr, &cp);
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
