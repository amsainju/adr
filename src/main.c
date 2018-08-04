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
	unsigned short int i, file_id, files_processed;
	MATFile *mat_file;
	FILE *input_file;
	unsigned char *byte_in, *header;
	_Bool new_file;
	unsigned long int radar_byte_counter, segment_counter;
	enum radar_header_segments current_segment;
	unsigned char *sync_string, *pps_counter_string, *format_string,
		*profile_length_string;
	unsigned long long int pps_counter, profile_byte_length, profile_count;
	enum data_format_type format;

	// Initialize variables.
	files_processed = 0;
	radar_byte_counter = 0;
	segment_counter = 0;
	current_segment = SYNC;
	profile_count = 0;

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

			// ----- RADAR_HEADER_TYPE through PPS_FRACTIONAL_COUNTER -----
			// We don't use these values, so just eat them up quickly.
			// Nothing is needed here.

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
			if (	(current_segment == PROFILE_DATA_FORMAT) &&
					(profile_count == 0)) {
				// Store the current byte in the string.
				*(format_string+segment_counter) = *byte_in;

				// If the last byte, convert and store the type.
				if (segment_counter == (PROFILE_DATA_FORMAT_LENGTH-1)) {
					if (format_string[2] == 0) {
						format = SIGNED_16;
					} else if (format_string[2] == 1) {
						format = UNSIGNED_16;
					} else if (format_string[2] == 2) {
						format = SIGNED_32;
					} else if (format_string[2] == 3) {
						format = FLOATING_32;
					} else {
						fprintf(stderr, "Unknown data format type.\n");
						exit(EXIT_FAILURE);
					}
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

	printf("Radar bytes: %lu\n", radar_byte_counter);
	printf("Profiles processed: %I64u\n", profile_count);

	// Clean up.
	matClose(mat_file);

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

	return 0;
}
