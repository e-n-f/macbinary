#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

int text = 0;
int named = 0;

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

	int textual = text && (strcmp(type, "TEXT") == 0);

	int len = read32be(header + 83);

	fprintf(stderr, "converting %s (%s/%s) %d bytes\n", name, type, creator, len);

	FILE *out = stdout;

	if (named) {
		char *cp;
		for (cp = name; *cp; cp++) {
			if (*cp == '/') {
				*cp = ':';
			}
		}

		out = fopen(name, "w");
		if (out == NULL) {
			fprintf(stderr, "Can't open %s to extract %s: %s\n",
			        name, fname, strerror(errno));
			return EXIT_FAILURE;
		}
	}

	int i;
	for (i = 0; i < len; i++) {
		int c = getc(f);

		if (textual) {
			if (c == '\r') {
				c = '\n';
			} else if (c == '\n') {
				c = '\r';
			}
		}

		putc(c, out);
	}

	if (out != stdout) {
		fclose(out);
	}

	return EXIT_SUCCESS;
}

void usage(char **argv) {
	fprintf(stderr, "Usage: %s [-tO] [file ...]\n", argv[0]);
	fprintf(stderr, "\n");
	fprintf(stderr, "-t: Swap \\r and \\n for TEXT type files\n");
	fprintf(stderr, "-O: Write output to file named in archive, not standard output\n");
}

int main(int argc, char **argv) {
	extern int optind;
	extern char *optarg;
	int i;
	int success = EXIT_SUCCESS;

	while ((i = getopt(argc, argv, "tO")) != -1) {
		switch (i) {
		case 't':
			text = 1;
			break;

		case 'O':
			named = 1;
			break;

		default:
			usage(argv);
			exit(EXIT_FAILURE);
		}
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
