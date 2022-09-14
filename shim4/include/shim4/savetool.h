#ifndef NOO_SAVETOOL_H
#define NOO_SAVETOOL_H

#include <stdio.h>
#include <stdlib.h>
#include <zlib.h>

int readle32(FILE *f)
{
	int bytes[4];
	for (int i = 0; i < 4; i++) {
		bytes[i] = fgetc(f);
	}
	return bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);
}

void writele32(FILE *f, int i)
{
	fputc(i & 0xff, f);
	fputc((i >> 8) & 0xff, f);
	fputc((i >> 16) & 0xff, f);
	fputc(i >> 24, f);
}

int do_compress(const char *infilename, const char *outfilename, int write_size)
{
	int ret = -1;

	FILE *in = fopen(infilename, "rb");
	if (in == NULL) {
		printf("Can't open %s!\n", infilename);
	}
	else {
		fseek(in, 0, SEEK_END);
		int sz = (int)ftell(in);
		fseek(in, 0, SEEK_SET);
		char *bytes = (char *)malloc(sz);
		if (bytes == NULL) {
			printf("Bad allocation!\n");
		}
		else {
			if (fread(bytes, sz, 1, in) != 1) {
				printf("Read failed!\n");
			}
			else {
				FILE *out = fopen(outfilename, "wb");
				if (out != NULL) {
					if (write_size) {
						writele32(out, sz);
					}
					uLongf compressed_size = compressBound(sz);
					char *compressed = (char *)malloc(compressed_size);
					if (compressed == NULL) {
						printf("Bad allocation!\n");
					}
					else {
						if (compress((Bytef *)compressed, &compressed_size, (Bytef *)bytes, sz) == Z_OK) {
							fwrite(compressed, compressed_size, 1, out);
							ret = (int)compressed_size + (write_size ? 4 : 0);
						}
						else {
							printf("Error compressing!\n");
						}
						free(compressed);
					}
					fclose(out);
				}
				else {
					printf("Can't open %s!\n", outfilename);
				}
			}
			free(bytes);
		}
		fclose(in);
	}

	return ret;
}

int do_decompress(const char *infilename, const char *outfilename)
{
	int ret = -1;

	FILE *in = fopen(infilename, "rb");
	if (in != NULL) {
		fseek(in, 0, SEEK_END);
		int compressed_size = (int)ftell(in) - 4;
		fseek(in, 0, SEEK_SET);
		uLongf uncompressed_size = readle32(in);
		char *compressed = (char *)malloc(compressed_size);
		if (compressed == NULL) {
			printf("Bad allocation!\n");
		}
		else {
			if (fread(compressed, compressed_size, 1, in) == 1) {
				char *uncompressed = (char *)malloc(uncompressed_size);
				if (uncompressed == NULL) {
					printf("Bad allocation!\n");
				}
				else {
					if (uncompress((Bytef *)uncompressed, &uncompressed_size, (Bytef *)compressed, compressed_size) == Z_OK) {
						FILE *out = fopen(outfilename, "wb");
						if (out != NULL) {
							fwrite(uncompressed, uncompressed_size, 1, out);
							fclose(out);
							ret = (int)uncompressed_size + 4;
						}
						else {
							printf("Can't open %s!\n", outfilename);
						}
					}
					else {
						printf("Error uncompressing!\n");
					}
					free(uncompressed);
				}
			}
			else {
				printf("Size is wrong!\n");
			}
			free(compressed);
		}
		fclose(in);
	}
	else {
		printf("Can't open %s!\n", infilename);
	}

	return ret;
}

#endif // NOO_SAVETOOL_H
