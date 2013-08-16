#include <stdlib.h>
#include <limits.h>
#include <stdio.h>

#include "url.h"

int main (int argc, char *argv[]) {
	if (argc <= 1) {
		printf(
			"joinurl - build an abosulute url from a base url and an relative url\n"
			"Usage: %s BASE-URL [RELATIVE-URL]...\n",
			argc > 0 ? argv[0] : "joinurl");
		return EXIT_FAILURE;
	}

	int i = 1;
	char *url = NULL;

	if (!lslinks_is_absurl(argv[1])) {
		i   = 2;
		url = lslinks_path_to_url(argv[1]);
		if (!url) {
			perror(argv[1]);
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
