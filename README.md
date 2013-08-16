lslinks - list links from a HTML document
=========================================

lslinks is a utility to list links from a HTML document. The document can be
read from a local file, stdin or a web site.

Also included: joinurl, a utility that builds an absolute url from a base url
and a number of relative urls. If the first argument isn't an absolute url it
is assumed to be an file path and converted into a file url. Only Unix style
file urls are supported.

Setup
-----

	wget https://github.com/panzi/lslinks/archive/master.zip
	unzip master.zip
	cd lslinks-master
	make -2j
	sudo make install PREFIX=/usr

Example Usage
-------------

	$ lslinks http://example.com/
	http://www.iana.org/domains/example
	$

For more information see `man lslinks` and `man joinurl`.

Dependencies
------------

 * [gumbo HTML5 parser](https://github.com/google/gumbo-parser)
 * [libcurl](http://curl.haxx.se/libcurl/)
