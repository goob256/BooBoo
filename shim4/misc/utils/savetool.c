#include "shim3/savetool.h"

int main(int argc, char **argv)
{
	if (argc < 4) {
		printf("Usage: %s [-compress|-decompress] <in> <out>\n", argv[0]);
		return 0;
	}

	int is_compress = !strcmp(argv[1], "-compress");

	int ret = 0;

	if (is_compress) {
		return do_compress(argv[2], argv[3], 1) == -1 ? 1 : 0;
	}
	else {
		return do_decompress(argv[2], argv[3]) == -1 ? 1 : 0;
	}
}
