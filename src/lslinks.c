#include <stdlib.h>
#include <string.h>

#include "lslinks.h"
#include "url.h"
#include <gumbo.h>

struct lslinks_data {
	int tags;
	const char *base;
	bool owns_base;
	void *data;
	lslinks_callback callback;
};

static bool lslinks_find(struct lslinks_data *data, GumboNode* node);

#define GET_ATTR(NAME) \
	if ((attr = gumbo_get_attribute(&node->v.element.attributes, (NAME)))) { \
		char *url = lslinks_absurl(attr->value, data->base); \
		if (!url) return false; \
		if (!data->callback(data->data, gumbo_normalized_tagname(node->v.element.tag), url)) { \
			free(url); \
			return false; \
		} \
		free(url); \
	}

bool lslinks_find(struct lslinks_data *data, GumboNode* node) {
	if (node->type != GUMBO_NODE_ELEMENT) {
		return true;
	}

	GumboAttribute *attr;
	switch (node->v.element.tag) {
		case GUMBO_TAG_A:
			if (data->tags & LSLINKS_A) {
				GET_ATTR("href");
			}
			break;
			
		case GUMBO_TAG_IMG:
			if (data->tags & LSLINKS_IMG) {
				GET_ATTR("src");
			}
			break;
			
		case GUMBO_TAG_LINK:
			if (data->tags & LSLINKS_LINK) {
				GET_ATTR("href");
			}
			break;
			
		case GUMBO_TAG_FRAME:
			if (data->tags & LSLINKS_FRAME) {
				GET_ATTR("src");
			}
			break;
			
		case GUMBO_TAG_IFRAME:
			if (data->tags & LSLINKS_IFRAME) {
				GET_ATTR("src");
			}
			break;
			
		case GUMBO_TAG_EMBED:
			if (data->tags & LSLINKS_EMBED) {
				GET_ATTR("src");
			}
			break;
			
		case GUMBO_TAG_OBJECT:
			if (data->tags & LSLINKS_OBJECT) {
				GET_ATTR("data");
			}
			break;
			
		case GUMBO_TAG_AUDIO:
			if (data->tags & LSLINKS_AUDIO) {
				GET_ATTR("src");
			}
			break;
			
		case GUMBO_TAG_VIDEO:
			if (data->tags & LSLINKS_VIDEO) {
				GET_ATTR("src");
			}
			break;
			
		case GUMBO_TAG_SOURCE:
			if (data->tags & LSLINKS_SOURCE) {
				GET_ATTR("src");
			}
			break;
			
		case GUMBO_TAG_SCRIPT:
			if (data->tags & LSLINKS_SCRIPT) {
				GET_ATTR("src");
			}
			break;
			
		case GUMBO_TAG_STYLE:
			if (data->tags & LSLINKS_STYLE) {
				GET_ATTR("src");
			}
			break;
			
		case GUMBO_TAG_APPLET:
			if (data->tags & LSLINKS_APPLET) {
				GET_ATTR("code");
			}
			break;
			
		case GUMBO_TAG_AREA:
			if (data->tags & LSLINKS_AREA) {
				GET_ATTR("href");
			}
			break;
			
		case GUMBO_TAG_BASE:
			if ((attr = gumbo_get_attribute(&node->v.element.attributes, "href"))) {
				char *url = lslinks_absurl(attr->value, data->base);
				if (!url) return false;
				if (data->owns_base) {
					free((void*)data->base);
				}
				data->base = url;
				data->owns_base = true;
				if (data->tags & LSLINKS_BASE && !data->callback(data->data, gumbo_normalized_tagname(node->v.element.tag), url)) {
					free(url);
					return false;
				}
				free(url);
			}
			break;
			
		case GUMBO_TAG_FORM:
			if (data->tags & LSLINKS_FORM) {
				GET_ATTR("action");
			}
			break;
	}
	
	GumboVector* children = &node->v.element.children;
	for (size_t i = 0; i < children->length; ++ i) {
		if (!lslinks_find(data, (GumboNode*)(children->data[i]))) {
			return false;
		}
	}
	
	return true;
}

int lslinks_parse_method(const char *s) {
	if (strcasecmp("GET",s) == 0) {
		return LSLINKS_GET;
	}
	else if (strcasecmp("POST",s) == 0) {
		return LSLINKS_POST;
	}
	else if (strcasecmp("PUT",s) == 0) {
		return LSLINKS_PUT;
	}
	else if (strcasecmp("DELETE",s) == 0) {
		return LSLINKS_DELETE;
	}
	else if (strcasecmp("PATCH",s) == 0) {
		return LSLINKS_PATCH;
	}
	else {
		return -1;
	}
}

int lslinks_parse_tags(const char *s) {
	int tags = 0;
	const char *ptr = s;

	while (*ptr) {
		const char *end = strchr(ptr, ',');
		bool neg = *ptr == '-';
		if (neg) ++ ptr;
		size_t n = end ? end - ptr : strlen(ptr);
		int tag = 0;

		if (strncasecmp("a",ptr,n) == 0) {
			tag = LSLINKS_A;
		}
		else if (strncasecmp("img",ptr,n) == 0) {
			tag = LSLINKS_IMG;
		}
		else if (strncasecmp("link",ptr,n) == 0) {
			tag = LSLINKS_LINK;
		}
		else if (strncasecmp("frame",ptr,n) == 0) {
			tag = LSLINKS_FRAME;
		}
		else if (strncasecmp("iframe",ptr,n) == 0) {
			tag = LSLINKS_IFRAME;
		}
		else if (strncasecmp("embed",ptr,n) == 0) {
			tag = LSLINKS_EMBED;
		}
		else if (strncasecmp("object",ptr,n) == 0) {
			tag = LSLINKS_OBJECT;
		}
		else if (strncasecmp("audio",ptr,n) == 0) {
			tag = LSLINKS_AUDIO;
		}
		else if (strncasecmp("video",ptr,n) == 0) {
			tag = LSLINKS_VIDEO;
		}
		else if (strncasecmp("source",ptr,n) == 0) {
			tag = LSLINKS_SOURCE;
		}
		else if (strncasecmp("script",ptr,n) == 0) {
			tag = LSLINKS_SCRIPT;
		}
		else if (strncasecmp("style",ptr,n) == 0) {
			tag = LSLINKS_STYLE;
		}
		else if (strncasecmp("applet",ptr,n) == 0) {
			tag = LSLINKS_APPLET;
		}
		else if (strncasecmp("area",ptr,n) == 0) {
			tag = LSLINKS_AREA;
		}
		else if (strncasecmp("base",ptr,n) == 0) {
			tag = LSLINKS_BASE;
		}
		else if (strncasecmp("form",ptr,n) == 0) {
			tag = LSLINKS_FORM;
		}
		else if (strncasecmp("*",ptr,n) == 0) {
			tag = LSLINKS_ALL;
		}
		else {
			return -1;
		}

		if (neg) {
			tags &= ~tag;
		}
		else {
			tags |= tag;
		}

		if (!end) break;
		ptr = end + 1;
	}

	return tags;
}

bool lslinks(GumboNode* node, int tags, const char *base, void *data, lslinks_callback callback) {
	struct lslinks_data opts = { tags, base, false, data, callback };
	bool ok = lslinks_find(&opts, node);
	if (opts.owns_base) {
		free((void*)opts.base);
	}
	return ok;
}

bool lslinks_file(FILE *fp, int tags, const char *base, void *data, lslinks_callback callback) {
	char *str = lslinks_readall(fp);
	if (!str) {
		return false;
	}
	
	bool ok = lslinks_str(str, tags, base, data, callback);
	free(str);

	return ok;
}

bool lslinks_str(const char *str, int tags, const char *base, void *data, lslinks_callback callback) {
	GumboOutput *doc = gumbo_parse(str);
	if (!doc) {
		return false;
	}

	bool ok = lslinks(doc->root, tags, base, data, callback);
	gumbo_destroy_output(&kGumboDefaultOptions, doc);	

	return ok;
}

bool lslinks_print_link(void *data, const char *tagname, const char *url) {
	const struct lslinks_print_options *opts = (const struct lslinks_print_options *)data;

	if (opts->tagname) {
		fwrite(tagname, strlen(tagname), 1, opts->fp);
		fwrite(" ", 1, 1, opts->fp);
	}

	fwrite(url, strlen(url), 1, opts->fp);
	fwrite(&opts->delim, 1, 1, opts->fp);

	return true;
}
