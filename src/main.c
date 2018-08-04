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
	unsigned long int radar_byte_counter;

	// Initialize variables.
	files_processed = 0;
	radar_byte_counter = 0;

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
			if (get_next_byte(input_file, byte_in, new_file, header)) {
				break;
			}

			radar_byte_counter++;
			new_file = FALSE;
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

	return 0;
}
