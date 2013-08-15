#ifndef LSLINKS_URL_H__
#define LSLINKS_URL_H__

#include "common.h"

bool lslinks_is_absurl(const char *url);
const char *lslinks_url_path(const char *absurl);
const char *lslinks_url_path_leaf(const char *absurl);
char *lslinks_absurl(const char *url, const char *base);

#endif // LSLINKS_URL_H__
