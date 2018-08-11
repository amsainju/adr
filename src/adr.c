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

	// Initialize local parameters.
	input_file_read = FALSE;
	*(parameters.file_limit) = FALSE;
	parameters.debug_mode = FALSE;

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
				strncpy(input_string, argv[++i], DEFAULT_NUMBER_LENGTH);
				*(parameters.max_files) =
						(unsigned short int) atoi(input_string);
			} else {
				// Not enough parameters left to do this. Exit.
				fprintf(stderr, "No parameter given after \"-m\". Exiting.\n");
				exit(EXIT_FAILURE);
			}
		} else if (!strcmp(argv[i], "-h")) {
			// Print out help information.
			printf("usage: adr.exe -i infile [-m max_files] [-p max_profiles]\n");
			printf("\t-i infile\tinfile is the input file to begin processing\n");
			printf("\t-m max_files\t(optional) process only max_files files\n");
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
			fprintf(stderr, "usage: adr.exe -i infile [-m max_files] [-p max_profiles]\n");
			fprintf(stderr,
					"\t-i infile\tinfile is the input file to begin processing\n");
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

_Bool get_next_byte (FILE *input_file, unsigned char *byte, _Bool new_file,
		unsigned char *header) {
	_Bool end_file;

	static unsigned long int byte_count, packet_byte_count,
		arena_payload_length;

	// If this is a new file, reset our counts.
	if (new_file) {
		byte_count = 0;
		packet_byte_count = 0;
	}

	if (packet_byte_count == 0) {
		// If this is the first byte of a packet, read the header.
		if(fread(header, sizeof(char), HEADER_LENGTH, input_file) !=
				HEADER_LENGTH) {
			// If the header wasn't read successfully, then we're likely at the
			//   end of the file. Return.
			return TRUE;
		} else {
			// If the header was read, increment our byte count.
			byte_count += HEADER_LENGTH;

			// Extract the ARENA payload length.
			arena_payload_length = data_to_num(
					header+ARENA_PAYLOAD_LENGTH_OFFSET,
					ARENA_PAYLOAD_LENGTH_LENGTH);
		}
	}

	// Read one byte to return to the main program.
	if(fread(byte, sizeof(char), 1, input_file) != 1) {
		end_file = TRUE;
	} else {
		end_file = FALSE;
		byte_count++;
		packet_byte_count++;
	}

	// If that was the last byte in the packet, reset the packet byte count.
	if (packet_byte_count == (arena_payload_length-16)) {
		packet_byte_count = 0;
	}

	return end_file;
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

_Bool resync (FILE *input_file, unsigned char *byte, _Bool new_file,
		unsigned char *header, struct input_parameters parameters) {
	_Bool end_data;

	end_data = FALSE;

	// Notify that we are resyncing.
	if (parameters.debug_mode) {
		printf("Searching for the sync word...");
	}

	// Read bytes until a the first byte of a proper sync word is received.
	while (TRUE) {
		if (get_next_byte(input_file, byte, new_file, header)) {
			if (parameters.debug_mode) {
				printf("reached end of data.\n");
			}
			end_data = TRUE;
			break;
		}

		if (*byte == 0) {
			// Get the next byte and check the value.
			if (get_next_byte(input_file, byte, new_file, header)) {
				if (parameters.debug_mode) {
					printf("reached end of data.\n");
				}
				end_data = TRUE;
				break;
			}

			// If it is correct, do the same thing for the next byte.
			if (*byte == 0) {
				// Get the next byte.
				if (get_next_byte(input_file, byte, new_file, header)) {
					if (parameters.debug_mode) {
						printf("reached end of data.\n");
					}
					end_data = TRUE;
					break;
				}

				// If correct, do it again.
				if (*byte == 0) {
					// Get the next byte.
					if (get_next_byte(input_file, byte, new_file, header)) {
						if (parameters.debug_mode) {
							printf("reached end of data.\n");
						}
						end_data = TRUE;
						break;
					}

					// If correct, do it for the fourth byte.
					if (*byte == 128) {
						// Get the next byte.
						if (get_next_byte(input_file, byte, new_file, header)) {
							if (parameters.debug_mode) {
								printf("reached end of data.\n");
							}
							end_data = TRUE;
							break;
						}

						// If correct, do it for the fifth byte.
						if (*byte == 0) {
							// Get the next byte.
							if (get_next_byte(input_file, byte, new_file, header)) {
								if (parameters.debug_mode) {
									printf("reached end of data.\n");
								}
								end_data = TRUE;
								break;
							}

							// If correct, do it for the sixth byte.
							if (*byte == 0) {
								// Get the next byte.
								if (get_next_byte(input_file, byte, new_file, header)) {
									if (parameters.debug_mode) {
										printf("reached end of data.\n");
									}
									end_data = TRUE;
									break;
								}

								// If correct, do it for the seventh byte.
								if (*byte == 128) {
									// Get the next byte.
									if (get_next_byte(input_file, byte, new_file, header)) {
										if (parameters.debug_mode) {
											printf("reached end of data.\n");
										}
										end_data = TRUE;
										break;
									}

									// If correct, do it for the eighth byte.
									if (*byte == 127) {
										// WE HAVE IT!
										if (parameters.debug_mode) {
											printf("found it!\n");
										}

										// Break out of this loop at this point and we'll be where the code expects a proper sync word to have just been read.
										break;

									// If incorrect, reset the file pointer seven bytes.
									} else {
										fseek(input_file, (long int) ((-7) * sizeof(unsigned char)), SEEK_CUR);
									}

								// If incorrect, reset the file pointer six bytes.
								} else {
									fseek(input_file, (long int) ((-6) * sizeof(unsigned char)), SEEK_CUR);
								}

							// If incorrect, reset the file pointer five bytes.
							} else {
								fseek(input_file, (long int) ((-5) * sizeof(unsigned char)), SEEK_CUR);
							}

						// If incorrect, reset the file pointer four bytes.
						} else {
							fseek(input_file, (long int) ((-4) * sizeof(unsigned char)), SEEK_CUR);
						}

					// If incorrect, reset the file pointer three bytes.
					} else {
						fseek(input_file, (long int) ((-3) * sizeof(unsigned char)), SEEK_CUR);
					}

				// If incorrect, reset file pointer two bytes.
				} else {
					fseek(input_file, (long int) ((-2) * sizeof(unsigned char)), SEEK_CUR);
				}

			// If it is incorrect, reset the file pointer to "back up" this
			//   byte.
			} else {
				fseek(input_file, (long int) ((-1) * sizeof(unsigned char)), SEEK_CUR);
			}
		}
	}

	return end_data;
}
