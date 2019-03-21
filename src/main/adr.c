#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "adr.h"

struct input_parameters parse_command_line_parameters (int argc, char *argv[]) {
	struct input_parameters parameters;
	unsigned short int i;
	_Bool input_file_read;
	FILE *temp_file;
	char *input_string;
	// Allocate memory for the parameters.
	parameters.target_path = malloc(DEFAULT_PATH_LENGTH);
	parameters.file_limit = malloc(sizeof(_Bool));
	parameters.max_files = malloc(sizeof(unsigned short int));
	input_string = malloc(DEFAULT_NUMBER_LENGTH*sizeof(char));
	parameters.range = DEFAULT_PROFILE_BYTE_LENGTH;
	parameters.short_mode = SHORT_MODE;
	parameters.profile_count = DEFAULT_PROFILE_COUNT;

	// Initialize local parameters.
	input_file_read = FALSE;
	*(parameters.file_limit) = FALSE;
	parameters.debug_mode = FALSE;
	*(parameters.max_files)  = (unsigned short int)1; //Setting default value- Arpan 

	// Loop through the rest of the parameters.
	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-i")) {
			// Ready to read in input filename from the parameter list.
			if ((i + 1) < argc) {
				// Copy the supplied filename.
				strncpy(parameters.target_path, argv[++i], DEFAULT_PATH_LENGTH);
				input_file_read = TRUE;

				// Is this a valid path/file?
				temp_file = fopen(parameters.target_path, "rb");
				if (temp_file == NULL) {
					fprintf(stderr, "Could not open %s. Exiting.\n",
							parameters.target_path);
					exit(EXIT_FAILURE);
				} else {
					fclose(temp_file);
					temp_file = NULL;
				}
			} else {
				// No more parameters left. That's a problem.
				fprintf(stderr, "No parameter given after \"-i\". Exiting.\n");
				exit(EXIT_FAILURE);
			}
		} else if (!strcmp(argv[i], "-m")) {
			// We have a file limit. Handle appropriately.
			if ((i + 1) < argc) {
				// Grab the needed information from the parameter list.
				*(parameters.file_limit) = TRUE;
				strcpy(input_string,argv[++i]);
				*(parameters.max_files) =
						(unsigned short int) atoi(input_string);
			} else {
				// Not enough parameters left to do this. Exit.
				fprintf(stderr, "No parameter given after \"-m\". Exiting.\n");
				exit(EXIT_FAILURE);
			}
		} else if (!strcmp(argv[i], "-r")) {
			// A value for range gates is supplied. Overwrite the default.
			if ((i + 1) < argc) {
				// Grab the needed information from the parameter list.
				strcpy(input_string,argv[++i]);
				parameters.range = (unsigned short int) atoi(input_string);
			} else {
				// Not enough parameters left to do this. Exit.
				fprintf(stderr, "No parameter given after \"-r\". Exiting.\n");
				exit(EXIT_FAILURE);
			}
		} else if (!strcmp(argv[i], "-sm")) {
			// A value for short mode is supplied. Overwrite the default.
			if ((i + 1) < argc) {
				// Grab the needed information from the parameter list.
				strcpy(input_string,argv[++i]);
				parameters.short_mode = (unsigned short int) atoi(input_string);
			} else {
				// Not enough parameters left to do this. Exit.
				fprintf(stderr, "No parameter given after \"-sm\". Exiting.\n");
				exit(EXIT_FAILURE);
			}
		}else if (!strcmp(argv[i], "-p")) {
			// A value for profile count is supplied. Overwrite the default.
			if ((i + 1) < argc) {
				// Grab the needed information from the parameter list.
				strcpy(input_string,argv[++i]);
				parameters.profile_count = (int) atoi(input_string);
			} else {
				// Not enough parameters left to do this. Exit.
				fprintf(stderr, "No parameter given after \"-p\". Exiting.\n");
				exit(EXIT_FAILURE);
			}
		}else if (!strcmp(argv[i], "-h")) {
			// Print out help information.
			printf("usage: adr.exe -i infile [-m max_files] [-p max_profiles]\n");
			printf("\t-i infile\tinfile is the input file to begin processing\n");
			printf("\t-m max_files\t(optional) process only max_files files\n");
			printf("\t-r range\t(optional) provide the range value\n");
			printf("\t-sm short_mode\t(optional) provide the short mode value\n");
			printf("\t-p max_profiles\t(optional) process only max_profiles profiles\n");
		} else if (!strcmp(argv[i], "-v")) {
			// Print out version information.
			printf("ARENA data reader (adr) version %s\n", VERSION);
			exit(EXIT_SUCCESS);
		} else if (!strcmp(argv[i], "-d")) {
			// Print out debug information.
			parameters.debug_mode = TRUE;
		} else {
			// Unknown argument.
			fprintf(stderr, "usage: adr.exe -i infile [-sm short_mode] [-r range] [-m max_files] [-p max_profiles] \n");
			fprintf(stderr,
					"\t-i infile\tinfile is the input file to begin processing\n");

			fprintf(stderr, "\t-sm short_mode\t(optional) provide the short mode value\n");
			fprintf(stderr, "\t-r range\t(optional) provide the range value\n");
			fprintf(stderr,
					"\t-m max_files\t(optional) process only max_files files\n");
			fprintf(stderr, "\t-p max_profiles\t(optional) process only max_profiles profiles\n");
			exit(EXIT_FAILURE);
		}
	}

	// If we never got an input file, we can't continue.
	if (!input_file_read) {
		fprintf(stderr,
				"Input filename not given on the command line. Exiting.\n");
		exit(EXIT_FAILURE);
	}

	// Clean up.
	free(input_string);

	return parameters;
}

unsigned long long data_to_num (unsigned char *data, unsigned short int
								length) {
	unsigned long long num;
	unsigned short int i;

	num = 0;

	for (i = 0; i < length; i++) {
		num += data[i] * pow(256, i);
	}

	return num;
}

_Bool check_sync (unsigned char *read_string) {
	if ((read_string[0] == 0) &&
		(read_string[1] == 0) &&
		(read_string[2] == 0) &&
		(read_string[3] == 128) &&
		(read_string[4] == 0) &&
		(read_string[5] == 0) &&
		(read_string[6] == 128) &&
		(read_string[7] == 127)) {
		return TRUE;
	} else {
		return FALSE;
	}
}

unsigned long long int resync (FILE *input_file, unsigned char *byte, struct input_parameters parameters) {
	// Return value is the number of bytes that were read in to find the next sync word.
	// A return value of 0 means that we reached the end of the data without finding another.
	unsigned long long int offset;

	// Initialize variables.
	offset = 0;

	// Notify that we are resyncing.
	if (parameters.debug_mode) {
		printf("Searching for the sync word...");
	}

	// Read bytes until a the first byte of a proper sync word is received.
	while (TRUE) {
		if (fread(byte, sizeof(unsigned char), 1, input_file) != 1) {
			if (parameters.debug_mode) {
				printf("reached end of data.\n");
			}
			offset = 0;
			break;
		}

		if (*byte == 0) {
			// Get the next byte and check the value.
			if (fread(byte, sizeof(unsigned char), 1, input_file) != 1) {
				if (parameters.debug_mode) {
					printf("reached end of data.\n");
				}
				offset = 0;
				break;
			}

			// If it is correct, do the same thing for the next byte.
			if (*byte == 0) {
				// Get the next byte.
				if (fread(byte, sizeof(unsigned char), 1, input_file) != 1) {
					if (parameters.debug_mode) {
						printf("reached end of data.\n");
					}
					offset = 0;
					break;
				}

				// If correct, do it again.
				if (*byte == 0) {
					// Get the next byte.
					if (fread(byte, sizeof(unsigned char), 1, input_file) != 1) {
						if (parameters.debug_mode) {
							printf("reached end of data.\n");
						}
						offset = 0;
						break;
					}

					// If correct, do it for the fourth byte.
					if (*byte == 128) {
						// Get the next byte.
						if (fread(byte, sizeof(unsigned char), 1, input_file) != 1) {
							if (parameters.debug_mode) {
								printf("reached end of data.\n");
							}
							offset = 0;
							break;
						}

						// If correct, do it for the fifth byte.
						if (*byte == 0) {
							// Get the next byte.
							if (fread(byte, sizeof(unsigned char), 1, input_file) != 1) {
								if (parameters.debug_mode) {
									printf("reached end of data.\n");
								}
								offset = 0;
								break;
							}

							// If correct, do it for the sixth byte.
							if (*byte == 0) {
								// Get the next byte.
								if (fread(byte, sizeof(unsigned char), 1, input_file) != 1) {
									if (parameters.debug_mode) {
										printf("reached end of data.\n");
									}
									offset = 0;
									break;
								}

								// If correct, do it for the seventh byte.
								if (*byte == 128) {
									// Get the next byte.
									if (fread(byte, sizeof(unsigned char), 1, input_file) != 1) {
										if (parameters.debug_mode) {
											printf("reached end of data.\n");
										}
										offset = 0;
										break;
									}

									// If correct, do it for the eighth byte.
									if (*byte == 127) {
										// WE HAVE IT!
										if (parameters.debug_mode) {
											printf("found it!\n");
										}

										// Break out of this loop at this point and we'll be where the code expects a proper sync word to have just been read.
										offset += 8;
										break;

									// If incorrect, reset the file pointer seven bytes.
									} else {
										fseek(input_file, (long int) ((-7) * sizeof(unsigned char)), SEEK_CUR);
										offset++;
									}

								// If incorrect, reset the file pointer six bytes.
								} else {
									fseek(input_file, (long int) ((-6) * sizeof(unsigned char)), SEEK_CUR);
									offset++;
								}

							// If incorrect, reset the file pointer five bytes.
							} else {
								fseek(input_file, (long int) ((-5) * sizeof(unsigned char)), SEEK_CUR);
								offset++;
							}

						// If incorrect, reset the file pointer four bytes.
						} else {
							fseek(input_file, (long int) ((-4) * sizeof(unsigned char)), SEEK_CUR);
							offset++;
						}

					// If incorrect, reset the file pointer three bytes.
					} else {
						fseek(input_file, (long int) ((-3) * sizeof(unsigned char)), SEEK_CUR);
						offset++;
					}

				// If incorrect, reset file pointer two bytes.
				} else {
					fseek(input_file, (long int) ((-2) * sizeof(unsigned char)), SEEK_CUR);
					offset++;
				}

			// If it is incorrect, reset the file pointer to "back up" this
			//   byte.
			} else {
				fseek(input_file, (long int) ((-1) * sizeof(unsigned char)), SEEK_CUR);
				offset++;
			}
		} else {
			offset++;
		}
	}

	return offset;
}
