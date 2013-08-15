#ifndef LSLINKS_LSLINKS_H__
#define LSLINKS_LSLINKS_H__

#include <stdio.h>
#include <gumbo.h>
#include "common.h"

enum lslinks_tag {
	LSLINKS_A      = 1 <<  0,
	LSLINKS_IMG    = 1 <<  1,
	LSLINKS_LINK   = 1 <<  2,
	LSLINKS_FRAME  = 1 <<  3,
	LSLINKS_IFRAME = 1 <<  4,
	LSLINKS_EMBED  = 1 <<  5,
	LSLINKS_OBJECT = 1 <<  6,
	LSLINKS_AUDIO  = 1 <<  7,
	LSLINKS_VIDEO  = 1 <<  8,
	LSLINKS_SOURCE = 1 <<  9,
	LSLINKS_SCRIPT = 1 << 10,
	LSLINKS_STYLE  = 1 << 11,
	LSLINKS_APPLET = 1 << 12,
	LSLINKS_AREA   = 1 << 13,
	LSLINKS_BASE   = 1 << 14,
	LSLINKS_FORM   = 1 << 15,

	// TODO: <meta name="og:*"> etc.
};
#define LSLINKS_ALL ~(~0 << 15)

struct lslinks_print_options {
	bool tagname;
	char delim;
	FILE *fp;
};

enum lslinks_method {
	LSLINKS_GET,
	LSLINKS_POST,
	LSLINKS_PUT,
	LSLINKS_DELETE,
	LSLINKS_PATCH
};

typedef bool (*lslinks_callback)(void *data, const char *tagname, const char *href);

bool lslinks(GumboNode* node, int tags, const char *base, void *data, lslinks_callback callback);
bool lslinks_file(FILE *fp, int tags, const char *base, void *data, lslinks_callback callback);
bool lslinks_str(const char *str, int tags, const char *base, void *data, lslinks_callback callback);
bool lslinks_print_link(void *data, const char *tagname, const char *url);
int lslinks_parse_tags(const char *s);
int lslinks_parse_method(const char *s);

#endif //  LSLINKS_LSLINKS_H__
