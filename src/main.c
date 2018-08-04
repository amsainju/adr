#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>

#include "adr.h"
#include "mat.h"

int main (int argc, char **argv) {
	struct input_parameters parameters;
	char *input_file_basename, *path, *preamble, *extended_path, *config_path,
		*system_path, *data_path, *id_string, *id_num_string, *mat_filename,
		*input_filename;
	unsigned short int i, file_id, files_processed, sample_byte_count,
		sample_size, mode;
	MATFile *mat_file;
	FILE *input_file;
	unsigned char *byte_in, *header;
	_Bool new_file;
	unsigned long int radar_byte_counter, segment_counter;
	enum radar_header_segments current_segment;
	unsigned char *sync_string, *pps_counter_string, *format_string,
		*profile_length_string, *sample_string;
	unsigned long long int pps_counter, profile_byte_length, profile_count;
	enum data_format_type format;
	short int sample_s16, *short_profiles_s16, *long_profiles_s16;
	unsigned short int sample_u16, *short_profiles_u16, *long_profiles_u16;
	long int sample_s32r, sample_s32i, *short_profiles_s32, *long_profiles_s32;
	float sample_f, *short_profiles_f, *long_profiles_f;
	unsigned int samples_per_profile;
	unsigned int profile_structure_length;
	unsigned long long int *short_pps, *long_pps;
	unsigned long long int short_index, long_index, short_profile_count,
		long_profile_count;
	mxArray *short_pps_array, *long_pps_array, *short_array, *long_array;

	// Initialize variables.
	files_processed = 0;
	radar_byte_counter = 0;
	segment_counter = 0;
	current_segment = SYNC;
	profile_count = 0;
	sample_byte_count = 0;
	short_index = 0;
	long_index = 0;
	short_profile_count = 0;
	long_profile_count = 0;
	format = SIGNED_16;
	profile_byte_length = 0;
	pps_counter = 0;
	mode = 1;
	sample_size = 1;
	short_pps_array = NULL;
	long_pps_array = NULL;
	short_array = NULL;
	long_array = NULL;
	samples_per_profile = 0;

	// Parse input parameters.
	parameters = parse_command_line_parameters(argc, argv);

	// Allocate memory.
	input_file_basename = malloc(DEFAULT_PATH_LENGTH*sizeof(char));
	path = malloc(DEFAULT_PATH_LENGTH*sizeof(char));
	preamble = malloc((PREAMBLE_LENGTH+1)*sizeof(char));
	extended_path = malloc(DEFAULT_PATH_LENGTH*sizeof(char));
	config_path = malloc(DEFAULT_PATH_LENGTH*sizeof(char));
	system_path = malloc(DEFAULT_PATH_LENGTH*sizeof(char));
	data_path = malloc(DEFAULT_PATH_LENGTH*sizeof(char));
	id_string = malloc(DEFAULT_PATH_LENGTH*sizeof(char));
	id_num_string = malloc(DEFAULT_NUMBER_LENGTH*sizeof(char));
	mat_filename = malloc(DEFAULT_PATH_LENGTH*sizeof(char));
	input_filename = malloc(DEFAULT_PATH_LENGTH*sizeof(char));
	byte_in = malloc(sizeof(char));
	header = malloc(HEADER_LENGTH*sizeof(char));
	sync_string = malloc(SYNC_LENGTH*sizeof(char));
	pps_counter_string = malloc(PPS_COUNTER_LENGTH*sizeof(char));
	format_string = malloc(PROFILE_DATA_FORMAT_LENGTH*sizeof(char));
	profile_length_string = malloc(PROFILE_LENGTH_LENGTH*sizeof(char));
	short_profiles_s16 = malloc(1);
	long_profiles_s16 = malloc(1);
	short_profiles_u16 = malloc(1);
	long_profiles_u16 = malloc(1);
	short_profiles_s32 = malloc(1);
	long_profiles_s32 = malloc(1);
	short_profiles_f = malloc(1);
	long_profiles_f = malloc(1);
	long_pps = malloc(1);
	short_pps = malloc(1);
	sample_string = malloc(1);

	// Extract relevant information from path.
	strncpy(input_file_basename, basename(parameters.target_path),
			DEFAULT_PATH_LENGTH);
	strncpy(path, parameters.target_path, DEFAULT_PATH_LENGTH);
	path[strlen(path)-strlen(input_file_basename)] = '\0';
	strncpy(preamble, input_file_basename, PREAMBLE_LENGTH+1);
	preamble[PREAMBLE_LENGTH] = '\0';
	strncpy(extended_path, path, DEFAULT_PATH_LENGTH);
	strncat(extended_path, preamble, DEFAULT_PATH_LENGTH);
	strncpy(config_path, extended_path, DEFAULT_PATH_LENGTH);
	strcat(config_path, "config.xml");
	strncpy(system_path, extended_path, DEFAULT_PATH_LENGTH);
	strcat(system_path, "system.xml");
	strncpy(id_string, input_file_basename+ID_STRING_OFFSET,
			DEFAULT_PATH_LENGTH);
	for (i = 0; id_string[i] != '_'; i++);
	id_string[i] = '\0';
	strncpy(data_path, extended_path, DEFAULT_PATH_LENGTH);
	strncat(data_path, id_string, DEFAULT_PATH_LENGTH);
	strncat(data_path, "_", 1);
	for (i = i + 1; id_string[i] != '.'; i++) {
		strncat(id_num_string, id_string+i, 1);
	}
	file_id = atoi(id_num_string);

	// Open output file.
	strncpy(mat_filename, preamble, DEFAULT_PATH_LENGTH);
	mat_filename[strlen(preamble)-1] = '\0';
	strncat(mat_filename, ".mat", 4);
	mat_file = matOpen(mat_filename, "w");
	if (mat_file == NULL) {
		fprintf(stderr, "Cannot create mat file.\n");
		exit(EXIT_FAILURE);
	}

	// Set up first input filename.
	strncpy(input_filename, path, DEFAULT_PATH_LENGTH);
	strncat(input_filename, input_file_basename, DEFAULT_PATH_LENGTH);

	// Process data bytes from input files.
	while (TRUE) {
		// Are we at our file limit?
		if (parameters.file_limit &&
		    (*(parameters.max_files) == files_processed)) {
			break;
		}

		// Open file.
		input_file = fopen(input_filename, "rb");
		if (input_file == NULL) {
			// A file not being opened might be expected (at the end of the
			//   file list without a file limit). Therefore, we just end
			//   normally with no error.
			break;
		}
		new_file = TRUE;

		while (TRUE) {
			// Read in the next byte and bounce out if we're at the end of data.
			if (get_next_byte(input_file, byte_in, new_file, header)) {
				break;
			}

			// ----- SYNC -----
			if (current_segment == SYNC) {
				// Store the current byte in the string.
				*(sync_string+segment_counter) = *byte_in;

				// If the last byte, check it against the expected sync word.
				if (segment_counter == (SYNC_LENGTH - 1)) {
					if (!check_sync(sync_string)) {
						fprintf(stderr, "Problem reading in sync string.\n");
						exit(EXIT_FAILURE);
					}
				}
			}

			// ----- RADAR_HEADER_TYPE through RADAR_HEADER_LENGTH -----
			// We don't use these values, so just eat them up quickly.
			// Nothing is needed here.

			// ----- MODE -----
			if (current_segment == MODE) {
				// This is only one byte, so you have what you need.
				mode = (unsigned short int) *byte_in;
				if ((mode != 0) && (mode != 3)) {
					fprintf(stderr, "Unknown mode encountered.\n");
					exit(EXIT_FAILURE);
				}
			}

			// ----- SUBCHANNEL_DATA_SOURCE through PPS_FRACTIONAL_COUNTER -----
			// We don't use these values either.

			// ----- PPS_COUNTER -----
			if (current_segment == PPS_COUNTER) {
				// Store the current byte in the string.
				*(pps_counter_string+segment_counter) = *byte_in;

				// If the last byte, convert and store the number.
				if (segment_counter == (PPS_COUNTER_LENGTH-1)) {
					pps_counter = data_to_num(pps_counter_string,
							PPS_COUNTER_LENGTH);
				}
			}

			// ----- PROFILE_DATA_FORMAT -----
			// We only need this once.
			// This will also reallocate the memory for each sample only once.
			if (	(current_segment == PROFILE_DATA_FORMAT) &&
					(profile_count == 0)) {
				// Store the current byte in the string.
				*(format_string+segment_counter) = *byte_in;

				// If the last byte, convert and store the type.
				if (segment_counter == (PROFILE_DATA_FORMAT_LENGTH-1)) {
					if (format_string[2] == 0) {
						format = SIGNED_16;
						sample_size = 2;
					} else if (format_string[2] == 1) {
						format = UNSIGNED_16;
						sample_size = 2;
					} else if (format_string[2] == 2) {
						format = SIGNED_32;
						sample_size = 8;
					} else if (format_string[2] == 3) {
						format = FLOATING_32;
						sample_size = 8;
					} else {
						fprintf(stderr, "Unknown data format type.\n");
						exit(EXIT_FAILURE);
					}

					// Allocate memory for temporary sample storage.
					sample_string = realloc(sample_string,
							sample_size*sizeof(char));
				}
			}

			// ----- PROFILE_LENGTH -----
			if (current_segment == PROFILE_LENGTH) {
				// Store the current byte in the string.
				*(profile_length_string+segment_counter) = *byte_in;

				// If the last byte, convert and store.
				if (segment_counter == (PROFILE_LENGTH_LENGTH-1)) {
					profile_byte_length = data_to_num(profile_length_string,
							PROFILE_LENGTH_LENGTH);

					// Also here, if we're on the first profile, go ahead and
					//   allocate data for the profile matrices.
					if (profile_count == 0) {
						samples_per_profile = profile_byte_length / sample_size;
						profile_structure_length =
								samples_per_profile * DEFAULT_PROFILE_COUNT;
						if (format == SIGNED_16) {
							short_profiles_s16 = realloc(short_profiles_s16,
									profile_structure_length);
							long_profiles_s16 = realloc(long_profiles_s16,
									profile_structure_length);
						} else if (format == UNSIGNED_16) {
							short_profiles_u16 = realloc(short_profiles_u16,
									profile_structure_length);
							long_profiles_u16 = realloc(long_profiles_u16,
									profile_structure_length);
						} else if (format == SIGNED_32) {
							short_profiles_s32 = realloc(short_profiles_s32,
									profile_structure_length);
							long_profiles_s32 = realloc(long_profiles_s32,
									profile_structure_length);
						} else if (format == FLOATING_32) {
							short_profiles_f = realloc(short_profiles_f,
									profile_structure_length);
							long_profiles_f = realloc(long_profiles_f,
									profile_structure_length);
						}
						short_pps = realloc(short_pps,
								sizeof(unsigned long long int) *
								profile_structure_length);
						long_pps = realloc(long_pps,
								sizeof(unsigned long long int) *
								profile_structure_length);
						// TODO: if we run out of space, reallocate.
					}
				}
			}

			// ----- PROFILE_DATA -----
			if (current_segment == PROFILE_DATA) {
				// Store the current byte in the string.
				*(sample_string+sample_byte_count++) = *byte_in;

				// If the last byte of a sample,
				//   - reset the sample_byte_count,
				//   - convert the sample according to its data format, and
				//   - store the converted data in the profile structure.
				if (sample_byte_count == sample_size) {
					// Reset the sample_byte_count.
					sample_byte_count = 0;

					// Convert the sample according to its data format and
					//   store in the profile structure.
					if (format == SIGNED_16) {
						sample_s16 =
								(*(sample_string+1) << 8) + *(sample_string);
						if (mode == 0) {
							*(short_profiles_s16+short_index++) = sample_s16;
						} else if (mode == 3) {
							*(long_profiles_s16+long_index++) = sample_s16;
						}
					} else if (format == UNSIGNED_16) {
						sample_u16 =
								(*(sample_string+1) << 8) + *(sample_string);
						if (mode == 0) {
							*(short_profiles_u16+short_index++) = sample_u16;
						} else if (mode == 3) {
							*(long_profiles_u16+long_index++) = sample_u16;
						}
					} else if (format == SIGNED_32) {
						sample_s32r =
								(*(sample_string+3) << 24) +
								(*(sample_string+2) << 16) +
								(*(sample_string+1) << 8) +
								*(sample_string);
						sample_s32i =
								(*(sample_string+7) << 24) +
								(*(sample_string+6) << 16) +
								(*(sample_string+5) << 8) +
								*(sample_string+4);
						if (mode == 0) {
							*(short_profiles_s32+short_index++) = sample_s32r;
							*(short_profiles_s32+short_index++) = sample_s32i;
						} else if (mode == 3) {
							*(long_profiles_s32+long_index++) = sample_s32r;
							*(long_profiles_s32+long_index++) = sample_s32i;
						}
					} else if (format == FLOATING_32) {
						sample_f =
								(*(sample_string+3) << 24) +
								(*(sample_string+2) << 16) +
								(*(sample_string+1) << 8) +
								*(sample_string);
						if (mode == 0) {
							*(short_profiles_f+short_index++) = sample_f;
						} else if (mode == 3) {
							*(long_profiles_f+long_index++) = sample_f;
						}
					}
				}

				// If the last byte of the profile, increment the appropriate
				//   profile counter and store the PPS counter.
				if (segment_counter == (profile_byte_length-1)) {
					if (mode == 0) {
						*(short_pps+short_profile_count++) = pps_counter;
					} else if (mode == 3) {
						*(long_pps+long_profile_count++) = pps_counter;
					}
				}
			}

			// Update informational variables.
			radar_byte_counter++;
			new_file = FALSE;

			// Update the segment counter and our current segment.
			if (	(	(current_segment == SYNC) &&
						(segment_counter == (SYNC_LENGTH-1))) ||
					(	(current_segment == RADAR_HEADER_TYPE) &&
						(segment_counter == (RADAR_HEADER_TYPE_LENGTH-1))) ||
					(	(current_segment == RADAR_HEADER_LENGTH) &&
						(segment_counter == (RADAR_HEADER_LENGTH_LENGTH-1))) ||
					(	(current_segment == MODE) &&
						(segment_counter == (MODE_LENGTH-1))) ||
					(	(current_segment == SUBCHANNEL_DATA_SOURCE) &&
						(segment_counter ==
								(SUBCHANNEL_DATA_SOURCE_LENGTH-1))) ||
					(	(current_segment == RESERVED_A) &&
						(segment_counter == (RESERVED_A_LENGTH-1))) ||
					(	(current_segment == ENCODER) &&
						(segment_counter == (ENCODER_LENGTH-1))) ||
					(	(current_segment == RESERVED_B) &&
						(segment_counter == (RESERVED_B_LENGTH-1))) ||
					(	(current_segment == RELATIVE_COUNTER) &&
						(segment_counter == (RELATIVE_COUNTER_LENGTH-1))) ||
					(	(current_segment == PROFILE_COUNTER) &&
						(segment_counter == (PROFILE_COUNTER_LENGTH-1))) ||
					(	(current_segment == PPS_FRACTIONAL_COUNTER) &&
						(segment_counter ==
								(PPS_FRACTIONAL_COUNTER_LENGTH-1))) ||
					(	(current_segment == PPS_COUNTER) &&
						(segment_counter == (PPS_COUNTER_LENGTH-1))) ||
					(	(current_segment == PROFILE_DATA_FORMAT) &&
						(segment_counter == (PROFILE_DATA_FORMAT_LENGTH-1))) ||
					(	(current_segment == PROFILE_LENGTH) &&
						(segment_counter == (PROFILE_LENGTH_LENGTH-1))) ||
					(	(current_segment == PROFILE_DATA) &&
						(segment_counter == (profile_byte_length-1)))) {
				segment_counter = 0;
				if (current_segment == PROFILE_DATA) {
					current_segment = SYNC;
					profile_count++;
				} else {
					current_segment++;
				}
			} else {
				segment_counter++;
			}
		}

		// Close file.
		fclose(input_file);

		// Increment file count and filename.
		files_processed++;
		input_filename[strlen(input_filename)-8] = '\0';
		sprintf(id_num_string, "%04d", ++file_id);
		strncat(input_filename, id_num_string,
				DEFAULT_PATH_LENGTH-strlen(input_filename));
		strncat(input_filename, ".dat",
				DEFAULT_PATH_LENGTH-strlen(input_filename));
	}

	// Create output array structures.
	short_pps_array = mxCreateDoubleMatrix(1, short_profile_count, mxREAL);
	long_pps_array = mxCreateDoubleMatrix(1, long_profile_count, mxREAL);
	short_array = mxCreateDoubleMatrix(samples_per_profile,
			short_profile_count, mxCOMPLEX);
	long_array = mxCreateDoubleMatrix(samples_per_profile,
			long_profile_count, mxCOMPLEX);

	// Write output array structures to output file.
	if (matPutVariable(mat_file, SHORT_PPS_VARIABLE_NAME, short_pps_array) !=
			0) {
		fprintf(stderr, "Error writing %s to .mat file.\n",
				SHORT_PPS_VARIABLE_NAME);
		exit(EXIT_FAILURE);
	}
	if (matPutVariable(mat_file, LONG_PPS_VARIABLE_NAME, long_pps_array) != 0) {
		fprintf(stderr, "Error writing %s to .mat file.\n",
				LONG_PPS_VARIABLE_NAME);
		exit(EXIT_FAILURE);
	}
	if (matPutVariable(mat_file, SHORT_PROFILES_VARIABLE_NAME, short_array) !=
			0) {
		fprintf(stderr, "Error writing %s to .mat file.\n",
				SHORT_PROFILES_VARIABLE_NAME);
		exit(EXIT_FAILURE);
	}
	if (matPutVariable(mat_file, LONG_PROFILES_VARIABLE_NAME, long_array) !=
			0) {
		fprintf(stderr, "Error writing %s to .mat file.\n",
				LONG_PROFILES_VARIABLE_NAME);
		exit(EXIT_FAILURE);
	}

	// Print out debug information.
	if (DEBUG_MODE) {
		printf("Radar bytes: %lu\n", radar_byte_counter);
		printf("Profiles processed: %I64u\n", profile_count);
		printf("\t\tShort: %I64u\n", short_profile_count);
		printf("\t\t\tIndex: %I64u\n", short_index);
		printf("\t\tLong: %I64u\n", long_profile_count);
		printf("\t\t\tIndex: %I64u\n", long_index);
	}

	// Clean up.
	if (matClose(mat_file) != 0) {
		fprintf(stderr, "Error closing .mat file.\n");
		exit(EXIT_FAILURE);
	}

	mxDestroyArray(short_pps_array);
	mxDestroyArray(long_pps_array);
	mxDestroyArray(short_array);
	mxDestroyArray(long_array);

	free(parameters.target_path);
	free(parameters.file_limit);
	free(parameters.max_files);
	free(input_file_basename);
	free(path);
	free(preamble);
	free(extended_path);
	free(config_path);
	free(system_path);
	free(data_path);
	free(id_string);
	free(id_num_string);
	free(mat_filename);
	free(input_filename);
	free(byte_in);
	free(header);
	free(sync_string);
	free(pps_counter_string);
	free(format_string);
	free(profile_length_string);
	free(sample_string);
	free(short_profiles_s16);
	free(long_profiles_s16);
	free(short_profiles_u16);
	free(long_profiles_u16);
	free(short_profiles_s32);
	free(long_profiles_s32);
	free(short_profiles_f);
	free(long_profiles_f);
	free(short_pps);
	free(long_pps);

	return 0;
}
