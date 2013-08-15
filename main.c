#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include "lslinks.h"
#include "bytes.h"
#include "url.h"
#include <curl/curl.h>

#define STDIN_URL "file:///dev/stdin"

static size_t receive_data(char *ptr, size_t size, size_t nmemb, void *userdata);
static void usage(const char *appname);

size_t receive_data(char *ptr, size_t size, size_t nmemb, void *userdata) {
	struct lslinks_bytes *bytes = (struct lslinks_bytes *)userdata;
	size_t oldsize = bytes->size;
	lslinks_bytes_append(bytes, ptr, size * nmemb);
	return bytes->size - oldsize;
}

void usage(const char *appname) {
	printf(
		"usage: %s [OPTIONS] [PATH or URL...]\n"
		"\n"
		"OPTIONS:\n"
		"  -h, --help                print this help message\n"
		"  -v, --version             print version\n"
		"  -O, --output=FILE         write output to FILE\n"
		"  -b, --base=URL            use URL as document base (useful when reading from stdin)\n"
		"  -a, --user-agent=AGENT    use AGENT as the user agent\n"
		"  -r, --referrer=URL        use URL as referrer\n"
		"  -c, --cookie-file=FILE    read cookies from FILE\n"
		"  -j, --cookie-jar=FILE     write cookies to FILE\n"
		"  -H, --header=HEADER       send additional request header\n"
		"  -m, --method=METHOD       set the request METHOD (default: GET)\n"
		"                            supported values: GET, POST, PUT, DELETE, PATCH\n"
		"  -t, --tags=TAGS           comma separated list of HTML tags to look at\n"
		"  -n, --tag-name            print tag names\n"
		"  -d, --delim=CHAR          use CHAR to delimite output (default: '\\n')\n"
		"  -0, --zero                use a zero-byte to delimite output\n"
		"\n",
		appname);
}

int main(int argc, char* argv[]) {
	const char *base = NULL;
	const char *output = NULL;
	enum lslinks_method method = LSLINKS_GET;
	const char *agent = "Mozilla/5.0 (compatible) LsLinks/1.0";
	const char *referrer = NULL;
	const char *cookie_file = NULL;
	const char *cookie_jar = NULL;
	struct curl_slist *headers = NULL;
	int tags = LSLINKS_ALL;

	struct lslinks_print_options print_opts = { false, '\n', stdout };
	struct option long_options[] = {
		{"help",        no_argument,       0, 'h'},
		{"version",     no_argument,       0, 'v'},
		{"output",      required_argument, 0, 'O'},
		{"base",        required_argument, 0, 'b'},
		{"user-agent",  required_argument, 0, 'a'},
		{"referrer",    required_argument, 0, 'r'},
		{"cookie-file", required_argument, 0, 'c'},
		{"cookie-jar",  required_argument, 0, 'j'},
		{"header",      required_argument, 0, 'H'},
		{"method",      required_argument, 0, 'm'},
		{"tags",        required_argument, 0, 't'},
		{"tag-name",    no_argument,       0, 'n'},
		{"zero",        no_argument,       0, '0'},
		{"delim",       required_argument, 0, 'd'},
		{0,             0,                 0,  0 }
	};

	for (;;) {
		int opt = getopt_long(argc, argv, "hvO:b:a:r:H:m:t:n0", long_options, NULL);

		if (opt == -1)
			break;

		switch (opt) {
			case 'h':
				usage(argc > 0 ? argv[0] : "lslinks");
				if (headers) curl_slist_free_all(headers);
				return EXIT_SUCCESS;

			case 'v':
				puts(LSLINKS_VERSION);
				if (headers) curl_slist_free_all(headers);
				return EXIT_SUCCESS;

			case 'O':
				output = strcmp(optarg,"-") ? NULL : optarg;
				break;

			case 'b':
				base = optarg;
				break;

			case 'a':
				agent = optarg;
				break;

			case 'r':
				referrer = optarg;
				break;

			case 'c':
				cookie_file = optarg;
				break;

			case 'j':
				cookie_jar = optarg;
				break;

			case 'H':
				headers = curl_slist_append(headers, optarg);
				if (!headers) {
					perror("curl_slist_append");
					return EXIT_FAILURE;
				}
				break;

			case 'm':
				method = lslinks_parse_method(optarg);
				if (method == -1) {
					fprintf(stderr, "illegal method: %s\n", optarg);
					if (headers) curl_slist_free_all(headers);
					return EXIT_FAILURE;
				}
				break;

			case '0':
				print_opts.delim = '\0';
				break;

			case 'd':
				if (strlen(optarg) != 1) {
					fprintf(stderr, "argument for -d must be exactly one character long: %s\n", optarg);
				}
				print_opts.delim = *optarg;
				break;

			case 't':
				tags = lslinks_parse_tags(optarg);
				if (tags == -1) {
					fprintf(stderr, "illegal tag name: %s\n", optarg);
					if (headers) curl_slist_free_all(headers);
					return EXIT_FAILURE;
				}
				break;
				
			case 'n':
				print_opts.tagname = true;
				break;

			case '?':
				fprintf(stderr, "unknown option: -%c\n", opt);
		}
	}

	if (output) {
		print_opts.fp = fopen(output, "wb");
		if (!print_opts.fp) {
			perror(output);
			if (headers) curl_slist_free_all(headers);
			return EXIT_FAILURE;
		}
	}

	if (optind < argc) {
		bool used_curl = false;
		for (; optind < argc; ++ optind) {
			const char *url = argv[optind];
			bool isfileurl = strncasecmp("file:",url,5) == 0;
			
			if (isfileurl || !lslinks_is_absurl(url)) {
				const char *path = url;
				char *fileurl = NULL;

				if (isfileurl) {
					path += 5;
					if (path[0] == '/' && path[1] == '/') {
						++ path;
						if (path[1] == '/') {
							++ url;
						}
					}
				}
				else {
					char actualpath[PATH_MAX + 1];
					realpath(path, actualpath);
					fileurl = malloc(5 + strlen(actualpath) + 1);
					if (!fileurl) {
						perror(url);
						if (output) fclose(print_opts.fp);
						if (headers) curl_slist_free_all(headers);
						if (used_curl) curl_global_cleanup();
						return EXIT_FAILURE;
					}
					strcpy(fileurl, "file://");
					strcat(fileurl, actualpath);
				}

				FILE *input = fopen(path,"rb");

				if (!input) {
					perror(url);
					if (fileurl) free(fileurl);
					if (output) fclose(print_opts.fp);
					if (headers) curl_slist_free_all(headers);
					if (used_curl) curl_global_cleanup();
					return EXIT_FAILURE;
				}

				if (!lslinks_file(input, tags, !base ? (!fileurl ? url : fileurl) : base, &print_opts, &lslinks_print_link)) {
					perror(url);
					fclose(input);
					if (fileurl) free(fileurl);
					if (output) fclose(print_opts.fp);
					if (headers) curl_slist_free_all(headers);
					if (used_curl) curl_global_cleanup();
					return EXIT_FAILURE;
				}

				fclose(input);
			}
			else {
				if (!used_curl) {
					if (curl_global_init(CURL_GLOBAL_DEFAULT) != 0) {
						perror("initializing curl");
						if (output) fclose(print_opts.fp);
						if (headers) curl_slist_free_all(headers);
						return EXIT_FAILURE;
					}
				}

				struct lslinks_bytes bytes = LSLINKS_BYTES_INIT;
				CURL *curl = curl_easy_init();
				if (!curl) {
					perror("initializing curl");
					if (output) fclose(print_opts.fp);
					if (headers) curl_slist_free_all(headers);
					curl_global_cleanup();
					return EXIT_FAILURE;
				}

				curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, receive_data);
				curl_easy_setopt(curl, CURLOPT_WRITEDATA, &bytes);
				curl_easy_setopt(curl, CURLOPT_URL, url);
				curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
				curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
				curl_easy_setopt(curl, CURLOPT_USERAGENT, agent);

				if (referrer) {
					curl_easy_setopt(curl, CURLOPT_REFERER, referrer);
				}

				if (headers) {
					curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
				}

				if (cookie_file) {
					curl_easy_setopt(curl, CURLOPT_COOKIEFILE, cookie_file);
				}
				
				if (cookie_jar) {
					curl_easy_setopt(curl, CURLOPT_COOKIEJAR, cookie_jar);
				}

				switch (method) {
					case LSLINKS_GET:
						break;

					case LSLINKS_POST:
						curl_easy_setopt(curl, CURLOPT_POST, 1L);
						break;

					case LSLINKS_PUT:
						curl_easy_setopt(curl, CURLOPT_PUT, 1L);
						break;

					case LSLINKS_DELETE:
						curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
						break;
						
					case LSLINKS_PATCH:
						curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PATCH");
						break;
				}
				
				CURLcode res = curl_easy_perform(curl);
				if (res != CURLE_OK) {
					fprintf(stderr, "%s: %s\n", url, curl_easy_strerror(res));
					lslinks_bytes_cleanup(&bytes);
				    curl_easy_cleanup(curl);
					curl_global_cleanup();
					if (output) fclose(print_opts.fp);
					if (headers) curl_slist_free_all(headers);
					return EXIT_FAILURE;
				}

				if (!lslinks_bytes_append(&bytes, "", 1)) {
					perror(url);
					lslinks_bytes_cleanup(&bytes);
				    curl_easy_cleanup(curl);
					curl_global_cleanup();
					if (output) fclose(print_opts.fp);
					if (headers) curl_slist_free_all(headers);
					return EXIT_FAILURE;
				}

				if (!lslinks_str(bytes.data, tags, !base ? url : base, &print_opts, &lslinks_print_link)) {
					perror(url);
					lslinks_bytes_cleanup(&bytes);
				    curl_easy_cleanup(curl);
					curl_global_cleanup();
					if (output) fclose(print_opts.fp);
					if (headers) curl_slist_free_all(headers);
					return EXIT_FAILURE;
				}
				lslinks_bytes_cleanup(&bytes);
			    curl_easy_cleanup(curl);
			}
		}

		if (used_curl) curl_global_cleanup();
	}
	else {
		char *buf = lslinks_readall(stdin);

		if (!buf) {
			perror(STDIN_URL);
			if (output) fclose(print_opts.fp);
			if (headers) curl_slist_free_all(headers);
			return EXIT_FAILURE;
		}

		if (!lslinks_str(buf, tags, !base ? STDIN_URL : base, &print_opts, &lslinks_print_link)) {
			perror(STDIN_URL);
			free(buf);
			if (output) fclose(print_opts.fp);
			if (headers) curl_slist_free_all(headers);
			return EXIT_FAILURE;
		}
		free(buf);
	}

	if (output) fclose(print_opts.fp);
	if (headers) curl_slist_free_all(headers);

	return EXIT_SUCCESS;
}
