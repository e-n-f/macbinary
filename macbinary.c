#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

unsigned int read32be(unsigned char *c) {
	return (c[0] << 24) |
	       (c[1] << 16) |
	       (c[2] << 8) |
	       (c[3] << 0);
}

int process(FILE *f, char *fname) {
	unsigned char header[128];

	if (fread(header, sizeof(unsigned char), 128, f) != 128) {
		fprintf(stderr, "Short header read for %s\n", fname);
		return EXIT_FAILURE;
	}

	if (header[0] != 0) {
		fprintf(stderr, "Wrong byte 0 for %s\n", fname);
		return EXIT_FAILURE;
	}
	if (header[82] != 0) {
		fprintf(stderr, "Wrong byte 82 for %s\n", fname);
		return EXIT_FAILURE;
	}

	int namelen = header[1];
	if (namelen > 63) {
		fprintf(stderr, "Impossible name length %d for %s\n", namelen, fname);
		return EXIT_FAILURE;
	}

	char name[64];
	memcpy(name, header + 2, namelen);
	name[namelen] = 0;

	char type[5] = "    ";
	char creator[5] = "    ";

	memcpy(type, header + 65, 4);
	memcpy(creator, header + 69, 4);

	int len = read32be(header + 83);

	fprintf(stderr, "converting %s (%s/%s) %d bytes\n", name, type, creator, len);

	int i;
	for (i = 0; i < len; i++) {
		int c = getc(f);
		putc(c, stdout);
	}

	return EXIT_SUCCESS;
}

int main(int argc, char **argv) {
	extern int optind;
	extern char *optarg;
	int i;
	int success = EXIT_SUCCESS;

	while ((i = getopt(argc, argv, "")) != -1) {

	}

	if (optind < argc) {
		for (i = optind; i < argc; i++) {
			FILE *f = fopen(argv[i], "rb");
			if (f == NULL) {
				perror(argv[i]);
			} else {
				success = success | process(f, argv[i]);
				fclose(f);
			}
		}
	} else {
		success = success | process(stdin, "standard input");
	}

	return success;
}