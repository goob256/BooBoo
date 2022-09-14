// Prints # of channels in a WAV but probably fails on some WAVs with multiple RIFFs etc

#include <cstdio>

int main(int argc, char **argv)
{
	if (argc < 2) {
		printf("Usage: %s [WAV filename]\n", argv[0]);
		return 1;
	}

	FILE *f = fopen(argv[1], "rb");

	if (f == 0) {
		printf("Error opening %s\n", argv[1]);
		return 1;
	}

	for (int i = 0; i < 22; i++) {
		fgetc(f);
	}

	int b1 = fgetc(f);
	int b2 = fgetc(f);

	int channels = b1 | (b2 << 8);

	printf("(%s) %d channels\n", argv[1], channels);

	return 0;
}
