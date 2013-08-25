#ifndef LSLINKS_UNICODE_H__
#define LSLINKS_UNICODE_H__

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "bytes.h"

#define LSLINKS_UTF8_MAX_BYTES 6

enum {
	LSLINKS_U_NULL                      = 0x000000,
	LSLINKS_U_BACKSPACE                 = 0x000008,
	LSLINKS_U_CHARACTER_TABULATION      = 0x000009,
	LSLINKS_U_LINE_FEED                 = 0x00000A,
	LSLINKS_U_LINE_TABULATION           = 0x00000B,
	LSLINKS_U_FORM_FEED                 = 0x00000C,
	LSLINKS_U_CARRIAGE_RETURN           = 0x00000D,
	LSLINKS_U_SHIFT                     = 0x00000E,
	LSLINKS_U_INFORMATION_SEPARATOR_ONE = 0x00001F,
	LSLINKS_U_SPACE                     = 0x000020,
	LSLINKS_U_HYPHEN_MINUS              = 0x00002D,
	LSLINKS_U_DIGIT_ZERO                = 0x000030,
	LSLINKS_U_DIGIT_NINE                = 0x000039,
	LSLINKS_U_LATIN_CAPITAL_LETTER_A    = 0x000041,
	LSLINKS_U_LATIN_CAPITAL_LETTER_F    = 0x000046,
	LSLINKS_U_LATIN_CAPITAL_LETTER_Z    = 0x00005A,
	LSLINKS_U_LOW_LINE                  = 0x00005F,
	LSLINKS_U_LATIN_SMALL_LETTER_A      = 0x000061,
	LSLINKS_U_LATIN_SMALL_LETTER_F      = 0x000066,
	LSLINKS_U_LATIN_SMALL_LETTER_Z      = 0x00007A,
	LSLINKS_U_DELETE                    = 0x00007F,
	LSLINKS_U_CONTROL                   = 0x000080,
	LSLINKS_U_FIRST_SURROGATE           = 0x00D800,
	LSLINKS_U_LAST_SURROGATE            = 0x00DFFF,
	LSLINKS_U_REPLACEMENT               = 0x00FFFD,
	LSLINKS_U_MAX_ALLOWD                = 0x10FFFF
};

typedef uint32_t lslinks_codepoint;

const char *lslinks_utf8_decode_codepoint(const char *buf, size_t size, lslinks_codepoint *cpptr);
const char *lslinks_utf8_decode_codepoint_css(const char *buf, size_t size, lslinks_codepoint *cpptr);
size_t      lslinks_utf8_encode_codepoint(lslinks_codepoint codepoint, char *buf, size_t size);

bool lslinks_bytes_append_utf8_char(struct lslinks_bytes *bytes, lslinks_codepoint codepoint);

#define LSLINKS_IS_SPACE(CP) \
	((CP) == LSLINKS_U_LINE_FEED || \
	 (CP) == LSLINKS_U_CHARACTER_TABULATION || \
	 (CP) == LSLINKS_U_SPACE)

#define LSLINKS_IS_SURROGATE(CP) \
	((CP) >= LSLINKS_U_FIRST_SURROGATE && (CP) <= LSLINKS_U_LAST_SURROGATE)

#define LSLINKS_IS_HEX(CP) \
	(((CP) >= '0' && (CP) <= '9') || \
	 ((CP) >= 'a' && (CP) <= 'f') || \
	 ((CP) >= 'A' && (CP) <= 'F'))

#define LSLINKS_UNHEX(CP) \
	((CP) >= 'a' ? (CP) - 'a' + 10 : \
	 (CP) >= 'A' ? (CP) - 'A' + 10 : \
	 (CP) - '0')

#define LSLINKS_IS_NON_ASCII(CP) \
	((CP) >= LSLINKS_U_CONTROL)

#define LSLINKS_IS_IDENT_START2(CP) \
	(((CP) >= 'a' && (CP) <= 'z') || ((CP) >= 'A' && (CP) <= 'Z') || \
	 (CP) == '_' || LSLINKS_IS_NON_ASCII(CP))

#define LSLINKS_IS_IDENT_START(CP) \
	(LSLINKS_IS_IDENT_START2(CP) || (CP) == '-')

#define LSLINKS_IS_IDENT(CP) \
	(((CP) >= 'a' && (CP) <= 'z') || ((CP) >= 'A' && (CP) <= 'Z') || \
	 ((CP) >= '0' && (CP) <= '9') || \
	 (CP) == '-' || (CP) == '_' || LSLINKS_IS_NON_ASCII(CP))

#define LSLINKS_IS_DIGIT(CP) \
	((CP) >= '0' && (CP) <= '9')

#define LSLINKS_IS_NUMBER_START(CP) \
	((CP) == '+' || (CP) == '-' || (CP) == '.' || \
	 ((CP) >= '0' && (CP) <= '9'))
	
#define LSLINKS_IS_NUMBER_START2(CP) \
	(LSLINKS_IS_DIGIT(CP) || (CP) == '.')

bool lslinks_is_space(lslinks_codepoint codepoint);

#endif
