#include <cstdio>
#include <cstdlib>
#include <string>

int main(int argc, char **argv)
{
	int curr_id;

	if (argc < 6) {
		printf("Usage: translate [start id] [call] [rev_call1] [rev_call2] [filespec]\n\n");
		printf("[call] is e.g., \"GLOBALS->wedge_t->\"\n");
		printf("[rev_call1] is e.g., \"GLOBALS->game_t->\"\n");
		printf("[rev_call2] is e.g., \"GLOBALS->english_game_t->\"\n\n");
		printf("(these examples are for Wedge, where TRANSLATEs come from wedge_t and REVERSE from game_t)\n");
		exit(0);
	}

	curr_id = atoi(argv[1]);
	const char *call = argv[2];
	const char *rev_call1 = argv[3];
	const char *rev_call2 = argv[4];

	FILE *symbols = fopen("symbols.txt", "ab");
	if (symbols == NULL) {
		printf("Couldn't append to symbols.txt\n");
		exit(1);
	}

	for (int filenum = 5; filenum < argc; filenum++) {
		FILE *f = fopen(argv[filenum], "rb");
		if (f == NULL) {
			continue;
		}
		std::string outname = argv[filenum];
		outname += ".tmp";
		FILE *out = fopen(outname.c_str(), "wb");
		if (out == NULL) {
			fclose(f);
			printf("Failed to open output file %s\n", outname.c_str());
			exit(1);
		}

		char line[10000];

		while (fgets(line, 10000, f) != NULL) {
#ifdef DEBUG
			printf("line=%s", line);
#endif
			std::string l = line;
			size_t pos = 0;
			size_t start_pos = pos;
			while (true) {
				pos = l.find("TRANSLATE", pos);

#ifdef DEBUG
				printf("line from start_pos=%s", line+start_pos);
				printf("start_pos=%u, pos=%u after find (npos=%u)\n", start_pos, pos, std::string::npos);
#endif

				if (pos != std::string::npos) {
					
					bool is_reverse;
					if (pos < 8) {
						is_reverse = false;
					}
					else {
						is_reverse = l.substr(pos-8, 8) == "REVERSE_";
					}

#ifdef DEBUG
					printf("is_reverse=%d\n", is_reverse);
#endif

					if (is_reverse == false) {
						for (size_t p = start_pos; p < pos; p++) {
							fprintf(out, "%c", l[p]);
						}
						pos += 11; // Skip TRANSLATE("
						char ttext[10000];
						int i = 0;
						while (pos < l.length()) {
							char c = l[pos];
							if (c == '"') {
								if (l[pos-1] != '\\') {
									pos += 5; // Skip ")END
									break;
								}
								else {
									i--; // overwrite the \ with "
								}
							}
							ttext[i++] = c;
							pos++;
						}
						ttext[i] = 0;
						fprintf(out, "%stranslate(%d)/* Originally: %s */", call, curr_id, ttext);
						fprintf(symbols, "%d:%s\n", curr_id++, ttext);

#ifdef DEBUG
						printf("read '%s' as #%d\n", ttext, curr_id-1);
#endif

					}
					else {
						for (size_t p = start_pos; p < pos-8; p++) {
							fprintf(out, "%c", l[p]);
						}
						pos += 10; // Skip TRANSLATE( // part of REVERSE_TRANSLATE
						char ttext[10000];
						int i = 0;
						int paren_open = 0;
						while (pos < l.length()) {
							char c = l[pos];
							if (c == '(') {
								paren_open++;
							}
							if (c == ')') {
								if (paren_open == 0) {
									pos += 4; // Skip )END
									break;
								}
								else {
									paren_open--;
								}
							}
							ttext[i++] = c;
							pos++;
						}
						ttext[i] = 0;
						fprintf(out, "%stranslate(%sget_id(%s))", rev_call1, rev_call2, ttext);

#ifdef DEBUG
						printf("read reverse, '%s'", ttext);
#endif
					}
					
					start_pos = pos;
				}
				else {
#ifdef DEBUG
					printf("end of line\n");
#endif
					break;
				}

#ifdef DEBUG
				printf("end of loop and (pos == npos)=%d\n", pos == std::string::npos);
#endif
			}

			for (size_t p = start_pos; p < l.length(); p++) {
				fprintf(out, "%c", l[p]);
			}

			fflush(out);
		}
	
		fclose(f);
		fclose(out);
	}

	fclose(symbols);

	return 0;
}
