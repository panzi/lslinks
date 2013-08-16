#ifndef LSLINKS_URL_H__
#define LSLINKS_URL_H__

#include "common.h"
#include "bytes.h"

bool        lslinks_is_absurl(const char *url);
bool        lslinks_is_fileurl(const char *url);
const char *lslinks_url_path(const char *absurl);
const char *lslinks_url_path_leaf(const char *absurl);
bool        lslinks_absurl_bytes(const char *url, const char *base, struct lslinks_bytes *bytes);
char       *lslinks_absurl(const char *url, const char *base);
char       *lslinks_path_to_url(const char *path);
char       *lslinks_url_to_path(const char *absurl);
bool        lslinks_encode_uri_bytes(const char *s, struct lslinks_bytes *bytes);
char       *lslinks_encode_uri(const char *s);
bool        lslinks_decode_uri_bytes(const char *s, struct lslinks_bytes *bytes);
char       *lslinks_decode_uri(const char *s);

#endif // LSLINKS_URL_H__
