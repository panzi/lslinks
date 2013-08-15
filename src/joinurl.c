#include <stdlib.h>
#include <limits.h>
#include <stdio.h>

#include "url.h"

int main (int argc, char *argv[]) {
	if (argc <= 1) {
		printf("usage: %s [URL...]\n", argc > 0 ? argv[0] : "joinurl");
		return EXIT_FAILURE;
	}

	int i = 1;
	char *url = NULL;

	if (!lslinks_is_absurl(argv[1])) {
		i = 2;

		char actualpath[PATH_MAX + 1];
		realpath(argv[1], actualpath);

		url = lslinks_absurl(actualpath, "file:///");
		if (!url) {
			perror(actualpath);
			return EXIT_FAILURE;
		}
	}

	for (; i < argc; ++ i) {
		char *newurl = lslinks_absurl(argv[i], url);

		if (!newurl) {
			perror(argv[i]);
			free(url);
			return EXIT_FAILURE;
		}

		free(url);
		url = newurl;
	}

	puts(url);
	free(url);

	return EXIT_SUCCESS;
}
