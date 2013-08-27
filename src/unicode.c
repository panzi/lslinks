#include "unicode.h"

bool lslinks_is_space(lslinks_codepoint codepoint) {
	return LSLINKS_IS_SPACE(codepoint);
}

const char *lslinks_utf8_decode_codepoint(const char *str, size_t size, lslinks_codepoint *cpptr) {
	if (size == 0) {
		return NULL;
	}

	uint8_t byte = *str;

	if (byte >= 0xfc) {
		return NULL;
	}

	lslinks_codepoint cp = 0;

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

size_t lslinks_utf8_encode_codepoint(lslinks_codepoint codepoint, char *buf, size_t size) {
	if (codepoint > LSLINKS_U_MAX_ALLOWD) {
		return 0;
	}

	size_t i;

#define encode_first_byte(prefix) \
		if (size > i) { \
			buf[i] = prefix | codepoint; \
		}

#define encode_prev_byte \
		if (size > i) { \
			buf[i] = 0x80 | (codepoint & 0x3f); \
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

const char *lslinks_utf8_decode_codepoint_css(const char *buf, size_t size, lslinks_codepoint *cpptr) {
	lslinks_codepoint cp = 0;
	const char *ptr = lslinks_utf8_decode_codepoint(buf, size, &cp);

	if (!ptr) {
		// ignore broken utf8?
		cp = LSLINKS_U_REPLACEMENT;
		ptr = buf + 1;
	}
	else {
		switch (cp) {
			case LSLINKS_U_NULL:
				cp = LSLINKS_U_REPLACEMENT;
				break;

			case LSLINKS_U_FORM_FEED:
				cp = LSLINKS_U_LINE_FEED;
				break;

			case LSLINKS_U_CARRIAGE_RETURN:
				{
					const char *next = lslinks_utf8_decode_codepoint(ptr, size - (ptr - buf), &cp);

					if (next && cp == LSLINKS_U_LINE_FEED) {
						ptr = next;
					}
					else {
						cp = LSLINKS_U_LINE_FEED;
					}
				}
				break;
		}
	}

	if (cpptr) *cpptr = cp;

	return ptr;
}

bool lslinks_bytes_append_utf8_char(struct lslinks_bytes *bytes, lslinks_codepoint codepoint) {
	if (!lslinks_bytes_ensure(bytes, LSLINKS_UTF8_MAX_BYTES)) {
		return false;
	}

	size_t rest  = bytes->capacity - bytes->size;
	size_t count = lslinks_utf8_encode_codepoint(codepoint, bytes->data + bytes->size, rest);
	if (count == 0 || count > rest) {
		return false;
	}

	bytes->size += rest;
	return true;
}
