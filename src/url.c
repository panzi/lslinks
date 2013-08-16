#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>

#include "url.h"

#define IS_FILEURL(URL) (strncasecmp("file:",(URL),5) == 0)

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

bool lslinks_is_fileurl(const char *url) {
	return IS_FILEURL(url);
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
	if (IS_FILEURL(absurl)) {
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
	struct lslinks_bytes bytes;

	if (!lslinks_bytes_init2(&bytes, 64)) {
		return NULL;
	}

	if (!lslinks_absurl_bytes(url, base, &bytes) ||
	    !lslinks_bytes_append_nil(&bytes)) {
		lslinks_bytes_cleanup(&bytes);
		return NULL;
	}

	return bytes.data;
}

bool lslinks_absurl_bytes(const char *url, const char *base, struct lslinks_bytes *bytes) {
	const char *ptr;
	size_t urllen;

	while (isspace(*url)) ++ url;
	
	if (*url) {
		const char *urlend = url + strlen(url);

		while (urlend > url) {
			const char *next = urlend - 1;
			if (!isspace(*next)) break;
			urlend = next;
		}

		urllen = urlend - url;
	}
	else {
		return lslinks_bytes_append_str(bytes, base);
	}

	if (lslinks_is_absurl(url)) {
		return lslinks_bytes_append(bytes, url, urllen);
	}

	switch (url[0]) {
		case '/':
			if (url[1] == '/') {
				ptr = strchr(base, ':');
				if (!ptr) {
					errno = EINVAL;
					return false;
				}
				return
					lslinks_bytes_append(bytes, base, ptr - base + 1) &&
					lslinks_bytes_append(bytes, url, urllen);
			}
			else if (IS_FILEURL(base)) {
				return
					lslinks_bytes_append_str(bytes, "file://") &&
					lslinks_bytes_append(bytes, url, urllen);
			}
			else {
				ptr = lslinks_url_path(base);
				return ptr &&
					lslinks_bytes_append(bytes, base, ptr - base) &&
					lslinks_bytes_append(bytes, url, urllen);
			}
			
		case '#':
			ptr = strchr(base, '#');
			if (!ptr) {
				ptr = base;
			}
			return
				lslinks_bytes_append_str(bytes, ptr) &&
				lslinks_bytes_append(bytes, url, urllen);
			
		case '?':
			ptr = base;
			while (*ptr && *ptr != '?' && *ptr != '#') ++ ptr;
			return
				lslinks_bytes_append(bytes, base, ptr - base) &&
				lslinks_bytes_append(bytes, url, urllen);

		default:
			ptr = lslinks_url_path_leaf(base);
			return ptr &&
				lslinks_bytes_append(bytes, base, ptr - base) &&
				lslinks_bytes_append_str(bytes, "/") &&
				lslinks_bytes_append(bytes, url, urllen) &&
				lslinks_bytes_append_nil(bytes);
	}
}

char *lslinks_path_to_url(const char *path) {
	if (path[0] == '/') {
		char *encoded = lslinks_encode_uri(path);
		if (!encoded) {
			return NULL;
		}

		char *url = lslinks_strcat("file://",encoded);
		free(encoded);
		return url;
	}

	char pwd[PATH_MAX + 2];
	if (!realpath(".", pwd)) {
		return NULL;
	}
	strncat(pwd, "/", PATH_MAX + 2);

	char *encoded = lslinks_encode_uri(pwd);
	if (!encoded) {
		return NULL;
	}

	char *base = lslinks_absurl(encoded, "file:///");
	free(encoded);

	if (!base) {
		return NULL;
	}

	encoded = lslinks_encode_uri(path);
	if (!encoded) {
		free(base);
		return NULL;
	}

	char *url = lslinks_absurl(encoded, base);
	free(encoded);
	free(base);

	return url;
}

char *lslinks_url_to_path(const char *absurl) {
	if (!IS_FILEURL(absurl)) {
		errno = EINVAL;
		return NULL;
	}

	const char *ptr = absurl + 5;
	if (ptr[0] == '/' && ptr[1] == '/') {
		++ ptr;
		if (ptr[1] == '/') {
			++ ptr;
		}
	}

	return lslinks_decode_uri(ptr);
}

// https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/encodeURI?redirectlocale=en-US&redirectslug=JavaScript%2FReference%2FGlobal_Objects%2FencodeURI
//
// Reserved characters  ; , / ? : @ & = + $
// Unescaped characters alphabetic, decimal digits, - _ . ! ~ * ' ( )
// Score                #
// 
bool lslinks_encode_uri_bytes(const char *s, struct lslinks_bytes *bytes) {
	if (!lslinks_bytes_ensure(bytes, strlen(s))) {
		return false;
	}

	char buf[3] = "%\0\0";
	for (const char *ptr = s;; ++ ptr) {
		const char *last = ptr;

		for (;; ++ ptr) {
			char ch = *ptr;
			if (ch != '!' && ch != '#' && ch != '$' && ch != '&' &&
			    (ch < '(' || ch > ';') && ch != '=' && (ch < '?' || ch > 'Z') &&
			    ch != '_' && (ch < 'a' || ch > 'z') && ch != '~') {
				break;
			}
		}

		if (ptr != last && !lslinks_bytes_append(bytes, last, ptr - last)) {
			return false;
		}

		if (!*ptr) {
			break;
		}

		int hi = *ptr >> 4;
		int lo = *ptr & 0xf;
		
		buf[1] = hi > 9 ? 'A' + hi - 10 : '0' + hi;
		buf[2] = lo > 9 ? 'A' + lo - 10 : '0' + lo;

		if (!lslinks_bytes_append(bytes, buf, sizeof(buf))) {
			return false;
		}
	}

	return true;
}

bool lslinks_decode_uri_bytes(const char *s, struct lslinks_bytes *bytes) {
	if (!lslinks_bytes_ensure(bytes, strlen(s))) {
		return false;
	}
	
	for (const char *ptr = s;; ++ ptr) {
		const char *last = ptr;

		while (*ptr && *ptr != '%') ++ ptr;
		
		if (ptr != last && !lslinks_bytes_append(bytes, last, ptr - last)) {
			return false;
		}

		if (!*ptr) {
			break;
		}

		if (isxdigit(ptr[1]) && isxdigit(ptr[2])) {
			int hi = ptr[1];
			int lo = ptr[2];

			if      (hi >= 'a') hi -= 'a' - 10;
			else if (hi >= 'A') hi -= 'A' - 10;
			else                hi -= '0';
			
			if      (lo >= 'a') lo -= 'a' - 10;
			else if (lo >= 'A') lo -= 'A' - 10;
			else                lo -= '0';

			char ch = (hi << 4) | lo;
			if (!lslinks_bytes_append(bytes, &ch, 1)) {
				return false;
			}

			ptr += 2;
		}
		else if (!lslinks_bytes_append(bytes, ptr, 1)) {
			return false;
		}
	}

	return true;
}

char *lslinks_encode_uri(const char *s) {
	struct lslinks_bytes bytes;

	if (!lslinks_bytes_init2(&bytes, 32)) {
		return NULL;
	}

	if (!lslinks_encode_uri_bytes(s, &bytes) ||
	    !lslinks_bytes_append_nil(&bytes)) {
		lslinks_bytes_cleanup(&bytes);
		return NULL;
	}

	return bytes.data;
}

char *lslinks_decode_uri(const char *s) {
	struct lslinks_bytes bytes;

	if (!lslinks_bytes_init2(&bytes, 32)) {
		return NULL;
	}

	if (!lslinks_decode_uri_bytes(s, &bytes) ||
	    !lslinks_bytes_append_nil(&bytes)) {
		lslinks_bytes_cleanup(&bytes);
		return NULL;
	}

	return bytes.data;
}
