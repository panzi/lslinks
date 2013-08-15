#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "url.h"

bool lslinks_is_absurl(const char *url) {
	const char *ptr = url;
	for (;; ++ ptr) {
		char ch = *ptr;
		if (!isalnum(ch) && ch != '-' && ch != '_' && ch != '+' && ch != '.') {
			break;
		}
	}

	return ptr != url && *ptr == ':';
}

const char *lslinks_url_path(const char *absurl) {
	const char *ptr = strchr(absurl, ':');
	if (!ptr) {
		errno = EINVAL;
		return NULL;
	}

	++ ptr;
	while (*ptr == '/') ++ ptr;
	ptr = strchr(ptr, '/');

	if (!ptr) {
		return absurl + strlen(absurl);
	}

	return ptr;
}

const char *lslinks_url_path_leaf(const char *absurl) {
	const char *ptr, *last;
	if (strncasecmp("file:", absurl, 5) == 0) {
		last = ptr = absurl + 5;
		while (*ptr) {
			if (*ptr == '/') last = ptr;
			++ ptr;
		}
	}
	else {
		last = ptr = lslinks_url_path(absurl);
		if (!ptr) return NULL;
		while (*ptr && *ptr != '#' && *ptr != '?') {
			if (*ptr == '/') last = ptr;
			++ ptr;
		}
	}

	return last;
}

char *lslinks_absurl(const char *url, const char *base) {
	size_t size, prefixlen;
	char *buf;
	const char *ptr;

	while (isspace(*url)) ++ url;
	
	const char *urlend = url + strlen(url);

	while (urlend > url && isspace(*urlend)) -- urlend;

	size_t urllen = urlend - url;

	if (urllen == 0) {
		return strdup(base);
	}
	else if (lslinks_is_absurl(url)) {
		size = urllen + 1;
		buf = malloc(size);
		if (!buf) return NULL;
		memcpy(buf, url, urllen);
		buf[urllen] = '\0';
		return buf;
	}

	switch (url[0]) {
		case '/':
			if (url[1] == '/') {
				ptr = strchr(base, ':');
				if (!ptr) {
					errno = EINVAL;
					return NULL;
				}
				prefixlen = ptr - base + 1;
				size = prefixlen + urllen + 1;
				buf  = malloc(size);
				if (!buf) return NULL;
				memcpy(buf, base, prefixlen);
				memcpy(buf + prefixlen, url, urllen);
				buf[size - 1] = '\0';
				return buf;
			}
			else if (strncasecmp("file:", base, 5) == 0) {
				size = 7 + urllen + 1;
				buf  = malloc(size);
				if (!buf) return NULL;
				memcpy(buf, "file://", 7);
				memcpy(buf + 7, url, urllen);
				buf[size - 1] = '\0';
				return buf;
			}
			else {
				ptr = lslinks_url_path(base);
				if (!ptr) return NULL;
				prefixlen = ptr - base;
				size = prefixlen + urllen + 1;
				buf  = malloc(size);
				if (!buf) return NULL;
				memcpy(buf, base, prefixlen);
				memcpy(buf + prefixlen, url, urllen);
				buf[size - 1] = '\0';
				return buf;
			}
			
		case '#':
			ptr = strchr(base, '#');
			if (!ptr) {
				ptr = base;
			}
			prefixlen = strlen(ptr);
			size = prefixlen + urllen + 1;
			buf  = malloc(size);
			if (!buf) return NULL;
			memcpy(buf, ptr, prefixlen);
			memcpy(buf + prefixlen, url, urllen);
			buf[size - 1] = '\0';
			return buf;
			
		case '?':
			ptr = base;
			while (*ptr && *ptr != '?' && *ptr != '#') ++ ptr;
			prefixlen = ptr - base;
			size = prefixlen + urllen + 1;
			buf  = malloc(size);
			if (!buf) return NULL;
			memcpy(buf, base, prefixlen);
			memcpy(buf + prefixlen, url, urllen);
			buf[size - 1] = '\0';
			return buf;

		default:
			ptr = lslinks_url_path_leaf(base);
			if (!ptr) return NULL;
			prefixlen = ptr - base;
			size = prefixlen + 1 + urllen + 1;
			buf  = malloc(size);
			memcpy(buf, base, prefixlen);
			buf[prefixlen] = '/';
			memcpy(buf + prefixlen + 1, url, urllen);
			buf[size - 1] = '\0';
			return buf;
	}
}
