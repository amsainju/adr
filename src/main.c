#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>

#include "adr.h"
#include "mat.h"
typedef unsigned long long int ULongLongInt;
//typedef long int LongInt;
typedef unsigned int LongInt;
typedef short int ShortInt;
//typedef unsigned long int ULongInt;
typedef unsigned int ULongInt;
typedef unsigned short int UShortInt;
typedef unsigned int UInt;
int main (int argc, char **argv) {
	char *input_file_basename;
	char *path;
	char *preamble;
	char *extended_path;
	char *config_path;
	char *system_path;
	char *id_string;
	char *data_path;
	char *id_num_string;
	char *mat_filename;
	char *input_filename;
	char *radar_filename; //Added by Arpan
	char *input_filename_withchannel;
	enum data_format_type format;
	enum radar_header_segments current_segment;
	FILE *input_file;
	FILE *radar_file;
	float *short_profiles_f;
	float *long_profiles_f;
	float sample_f;
	LongInt *short_profiles_s32;
	LongInt *long_profiles_s32;
	LongInt sample_s32r;
	LongInt sample_s32i;
	MATFile *mat_file;
	mxArray *short_pps_array;
	mxArray *long_pps_array;
	mxArray *short_array;
	mxArray *long_array;
	ShortInt *short_profiles_s16;
	ShortInt *long_profiles_s16;
	ShortInt sample_s16;
	struct input_parameters parameters;
	unsigned char *header;
	unsigned char *byte_in;
	unsigned char *sync_string;
	unsigned char *pps_counter_string;
	unsigned char *format_string;
	unsigned char *sample_string;
	unsigned char *profile_length_string;
	UInt packet_bytes;
	UInt arena_payload_length;
	UInt samples_per_profile;
	UInt profile_structure_length;
	ULongInt segment_counter;
	ULongInt radar_byte_counter;
	ULongLongInt profile_count;
	ULongLongInt pps_counter;
	ULongLongInt profile_byte_length;
	ULongLongInt *short_pps;
	ULongLongInt *long_pps;
	ULongLongInt short_index;
	ULongLongInt long_index;
	ULongLongInt short_profile_count;
	ULongLongInt long_profile_count;
	UShortInt i;
	UShortInt file_id;
	UShortInt files_processed;
	UShortInt mode;
	UShortInt sample_size;
	UShortInt *short_profiles_u16;
	UShortInt *long_profiles_u16;
	UShortInt sample_byte_count;
	UShortInt sample_u16;

	// Parse input parameters.
	parameters.debug_mode = FALSE;
	parameters.file_limit = FALSE;
	parameters = parse_command_line_parameters(argc, argv);

	// Initialize variables.
	files_processed = 0;
	current_segment = SYNC;
	segment_counter = 0;
	profile_count = 0;
	mode = 1;
	pps_counter = 0;
	format = SIGNED_16;
	sample_size = 1;
	profile_byte_length = 0;
	samples_per_profile = 0;
	profile_structure_length = 0;
	sample_byte_count = 0;
	short_index = 0;
	long_index = 0;
	short_profile_count = 0;
	long_profile_count = 0;
	radar_byte_counter = 0;

	// Allocate memory.
	input_file_basename = malloc(DEFAULT_PATH_LENGTH*sizeof(char));
	path = malloc(DEFAULT_PATH_LENGTH*sizeof(char));
	preamble = malloc((PREAMBLE_LENGTH+1)*sizeof(char));
	extended_path = malloc(DEFAULT_PATH_LENGTH*sizeof(char));
	config_path = malloc(DEFAULT_PATH_LENGTH*sizeof(char));
	system_path = malloc(DEFAULT_PATH_LENGTH*sizeof(char));
	id_string = malloc(DEFAULT_PATH_LENGTH*sizeof(char));
	data_path = malloc(DEFAULT_PATH_LENGTH*sizeof(char));
	id_num_string = malloc(DEFAULT_NUMBER_LENGTH*sizeof(char));
	mat_filename = malloc(DEFAULT_PATH_LENGTH*sizeof(char));
	input_filename = malloc(DEFAULT_PATH_LENGTH*sizeof(char));
	radar_filename = malloc(DEFAULT_PATH_LENGTH*sizeof(char));
	input_filename_withchannel = malloc(DEFAULT_PATH_LENGTH*sizeof(char));
	header = malloc(HEADER_LENGTH*sizeof(unsigned char));
	byte_in = malloc(sizeof(unsigned char));
	sync_string = malloc(SYNC_LENGTH*sizeof(unsigned char));
	pps_counter_string = malloc(PPS_COUNTER_LENGTH*sizeof(unsigned char));
	format_string = malloc(PROFILE_DATA_FORMAT_LENGTH*sizeof(unsigned char));
	sample_string = malloc(sizeof(unsigned char));
	profile_length_string = malloc(PROFILE_LENGTH_LENGTH*sizeof(unsigned char));
	short_profiles_s16 = malloc(sizeof(ShortInt));
	long_profiles_s16 = malloc(sizeof(ShortInt));
	short_profiles_u16 = malloc(sizeof(UShortInt));
	long_profiles_u16 = malloc(sizeof(UShortInt));
	short_profiles_s32 = malloc(sizeof(LongInt));
	long_profiles_s32 = malloc(sizeof(LongInt));
	short_profiles_f = malloc(sizeof(float));
	long_profiles_f = malloc(sizeof(float));
	short_pps = malloc(sizeof(ULongLongInt));
	long_pps = malloc(sizeof(ULongLongInt));

	// Extract relevant information from path.
	strncpy(input_file_basename, basename(parameters.target_path), DEFAULT_PATH_LENGTH);
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
	strncpy(id_string, input_file_basename+ID_STRING_OFFSET, DEFAULT_PATH_LENGTH);
	for (i = 0; id_string[i] != '_'; i++);
	strncpy(data_path, extended_path, DEFAULT_PATH_LENGTH);
	strncat(data_path, id_string, DEFAULT_PATH_LENGTH);
	strncat(data_path, "_", 1);
	id_num_string[0] = '\0';
	for (i = i + 1; id_string[i] != '.'; i++) {
		strncat(id_num_string, id_string+i, 1);
	}
	file_id = atoi(id_num_string);
	for (i = 0; id_string[i] != '_'; i++);
	id_string[i] = '\0';

	//
	// Read through files and strip the radar packets, storing them in a temporary file.
	//

	// Set up first input filename.
	strncpy(input_filename, path, DEFAULT_PATH_LENGTH);
	strncat(input_filename, input_file_basename, DEFAULT_PATH_LENGTH);

	//Set up radar temp filename. Added by Arpan
	strncpy(radar_filename,input_file_basename,DEFAULT_PATH_LENGTH);
	strncat(radar_filename,".temp",DEFAULT_PATH_LENGTH);

	// Open temporary radar data file.
	//radar_file = fopen(RADAR_FILENAME, "wb");
	radar_file = fopen(radar_filename, "wb"); //Modified by Arpan
	if (radar_file == NULL) {
		fprintf(stderr, "Could not create temporary file to store radar data.\n");
		exit(EXIT_FAILURE);
	}

	// While loop to open successive files.
	while (TRUE) {
		// Open input file.
		input_file = fopen(input_filename, "rb");
		if (input_file == NULL) {
			if (parameters.debug_mode) {
				printf("%s not found. End of data.\n", input_filename);
			}
			break;
		}

		// Initialize our counter.
		packet_bytes = 0;

		// Loop to read UDP packets.
		while (TRUE) {
			// Read UDP packet header.
			if (fread(header, sizeof(unsigned char), HEADER_LENGTH, input_file) != HEADER_LENGTH) {
				// We didn't successfully read this header. This file is complete.
				// Bounce out of this loop so the next file can be processed.
				break;
			} else {
				// We did read this header successfully.
				packet_bytes += HEADER_LENGTH;

				// Extract the packet length from the header.
				arena_payload_length = data_to_num(header+ARENA_PAYLOAD_LENGTH_OFFSET, ARENA_PAYLOAD_LENGTH_LENGTH) - 16;
			}

			// Now we know there is a UDP payload of arena_payload_length bytes available. Get them.
			for (i = 0; i < arena_payload_length; i++) {
				if (fread(byte_in, sizeof(unsigned char), 1, input_file) != 1) {
					// Could not read this byte.
					break;
				} else {
					// Byte in 'byte_in'.
					// Write it to the radar file.
					if (fwrite(byte_in, sizeof(unsigned char), 1, radar_file) != 1) {
						// Write was unsuccessful.
						fprintf(stderr, "Error writing to radar file.\n");
						exit(EXIT_FAILURE);
					}
				}
			}

			// UDP payload successfully read. Move on to the next one.
		}

		//
		// Either all of the UDP packets have been read or there was a problem reading one. Either way, look for the next file.
		//

		// Close up the processed file.
		fclose(input_file);

		// Have we hit our file limit?
		files_processed++;
		if (parameters.file_limit && (*(parameters.max_files) == files_processed)) {
			if (parameters.debug_mode) {
				printf("Hit the file limit after processing %d files.\n", files_processed);
				break;
			}
			break; // process only the input file check if need to remove later -  Arpan 
		}

		// Update input_filename for the next file in the sequence.
		input_filename[strlen(input_filename)-8] = '\0';
		sprintf(id_num_string, "%04d", ++file_id);
		strncat(input_filename, id_num_string, DEFAULT_PATH_LENGTH-strlen(input_filename));
		strncat(input_filename, ".dat", DEFAULT_PATH_LENGTH-strlen(input_filename)-4);
		// At this point, execution will loop back to try to open the file with filename input_filename.
	}

	// Close temporary radar data file.
	fclose(radar_file);


	//
	// Now that the radar file contains only arena payload packets, we can parse to extract profiles.
	//

	// Open the radar data file.
	radar_file = fopen(radar_filename, "rb");
	//radar_file = fopen(RADAR_FILENAME, "rb");
	if (radar_file == NULL) {
		fprintf(stderr, "Temporary radar file could not be opened for reading.\n");
		exit(EXIT_FAILURE);
	}

	// Loop to read one radar byte at a time.
	while (TRUE) {
		// Read in the next byte and bounce out if we're at the end of the file.
		if(fread(byte_in, sizeof(unsigned char), 1, radar_file) != 1) {
			// Read was unsuccessful. Assume end of data.
			break;
		}

		// Process SYNC segment.
		if (current_segment == SYNC) {
			// Store the current byte in the sync string.
			*(sync_string + segment_counter) = *byte_in;

			// If the last byte, check it against the expected sync word.
			if (segment_counter == (SYNC_LENGTH - 1)) {
				if (!check_sync(sync_string)) {
					// Sync not found! Search for it.
					if (resync(radar_file, byte_in, parameters) == 0) {
						// Sync word was never found. We're done.
						break;
					}
				}
			}
		}

		// Process RADAR_HEADER_TYPE through RADAR_HEADER_LENGTH segments.
		// We don't need these values, so eat them up quickly.

		// Process MODE segment.
		if (current_segment == MODE) {
			// This is only one byte, so we have what we need.
			mode = (UShortInt) *byte_in;

			// Check that we are expecting this mode.
			if ((mode != LONG_MODE) && (mode != SHORT_MODE)) {
				fprintf(stderr, "Unknown mode encountered.\n");
				exit(EXIT_FAILURE);
			}
		}

		// Process SUBCHANNEL_DATA_SOURCE through PPS_FRACTIONAL_COUNTER segments.
		// We don't need these values, so eat them up quickly.

		// Process PPS_COUNTER segment.
		if (current_segment == PPS_COUNTER) {
			// Store the current byte in the string.
			*(pps_counter_string + segment_counter) = *byte_in;

			// If the last byte, convert and store the number.
			if (segment_counter == (PPS_COUNTER_LENGTH - 1)) {
				pps_counter = data_to_num(pps_counter_string, PPS_COUNTER_LENGTH);
			}
		}

		// Process PROFILE_DATA_FORMAT segment.
		// We will only need this once.
		// While we're here (for this first profile), we'll reallocate for the sample string.
		if ((current_segment == PROFILE_DATA_FORMAT) && (profile_count == 0)) {
			// Store the current byte in the string.
			*(format_string + segment_counter) = *byte_in;

			// If the last byte, convert and store the type.
			if (segment_counter == (PROFILE_DATA_FORMAT_LENGTH - 1)) {
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

				// Also allocate memory for temporary sample storage.
				sample_string = realloc(sample_string, sample_size * sizeof(unsigned char));
			}
		}

		// Process PROFILE_LENGTH segment.
		if (current_segment == PROFILE_LENGTH) {
			// Store the current byte in the string.
			*(profile_length_string + segment_counter) = *byte_in;

			// If the last byte, convert and store the length.
			if (segment_counter == (PROFILE_LENGTH_LENGTH - 1)) {
				// In this case, we're using a default value due to an ARENA software bug.
				// The actual value can be parsed from one of the XML files, but we are always
				//   using the maximum length.
				profile_byte_length = parameters.range * sample_size;

				// While here, allocate data for the profile matrices.
				if (profile_count == 0) {
					samples_per_profile = profile_byte_length / sample_size;
					profile_structure_length = samples_per_profile * DEFAULT_PROFILE_COUNT;
					if (format == SIGNED_16) {
						short_profiles_s16 = realloc(short_profiles_s16, profile_structure_length);
						long_profiles_s16 = realloc(long_profiles_s16, profile_structure_length);
					} else if (format == UNSIGNED_16) {
						short_profiles_u16 = realloc(short_profiles_u16, profile_structure_length);
						long_profiles_u16 = realloc(long_profiles_u16, profile_structure_length);
					} else if (format == SIGNED_32) {
						short_profiles_s32 = realloc(short_profiles_s32, profile_structure_length);
						long_profiles_s32 = realloc(long_profiles_s32, profile_structure_length);
					} else if (format == FLOATING_32) {
						short_profiles_f = realloc(short_profiles_f, profile_structure_length);
						long_profiles_f = realloc(long_profiles_f, profile_structure_length);
					}
					short_pps = realloc(short_pps, sizeof(ULongLongInt) *	profile_structure_length);
					long_pps = realloc(long_pps, sizeof(ULongLongInt) * profile_structure_length);
					// TODO: if we run out of space, reallocate.
				}
			}
		}

		// Process PROFILE_DATA segment.
		if (current_segment == PROFILE_DATA) {
			// Store the current byte in the string.
			*(sample_string + sample_byte_count++) = *byte_in;

			// If the last byte of a sample,
			//  - reset the sample_byte_count.
			//  - convert the sample according to its data format, and
			//  - store the converted data in the profile structure.
			if (sample_byte_count == sample_size) {
				// Reset the sample_byte_count.
				sample_byte_count = 0;

				// Convert the sample according to its data format and store it in the profile structure.
				if (format == SIGNED_16) {
					sample_s16 =
							(*(sample_string+1) << 8) + *(sample_string);
					if (mode == SHORT_MODE) {
						*(short_profiles_s16+short_index++) = sample_s16;
					} else if (mode == LONG_MODE) {
						*(long_profiles_s16+long_index++) = sample_s16;
					}
				} else if (format == UNSIGNED_16) {
					sample_u16 =
							(*(sample_string+1) << 8) + *(sample_string);
					if (mode == SHORT_MODE) {
						*(short_profiles_u16+short_index++) = sample_u16;
					} else if (mode == LONG_MODE) {
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
					if (mode == SHORT_MODE) {
						*(short_profiles_s32+short_index++) = sample_s32r;
						*(short_profiles_s32+short_index++) = sample_s32i;
					} else if (mode == LONG_MODE) {
						*(long_profiles_s32+long_index++) = sample_s32r;
						*(long_profiles_s32+long_index++) = sample_s32i;
					}
				} else if (format == FLOATING_32) {
					sample_f =
							(*(sample_string+3) << 24) +
							(*(sample_string+2) << 16) +
							(*(sample_string+1) << 8) +
							*(sample_string);
					if (mode == SHORT_MODE) {
						*(short_profiles_f+short_index++) = sample_f;
					} else if (mode == LONG_MODE) {
						*(long_profiles_f+long_index++) = sample_f;
					}
				}
			}

			// If the last byte of the profile, increment the appropriate profile counter and store the PPS counter.
			if (segment_counter == (profile_byte_length -1 )) {
				if (mode == SHORT_MODE) {
					*(short_pps + short_profile_count++) = pps_counter;
				} else if (mode == LONG_MODE) {
					*(long_pps + long_profile_count++) = pps_counter;
				}
			}
		}

		// Update informational variables.
		radar_byte_counter++;

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

	// Close temporary radar data file.
	fclose(radar_file);
	remove(radar_filename); //Added by Arpan to remove the temp radar file
	//
	// At this point, radar data has been processed. We just need to copy the appropriate data set to the output matrices and file.
	//
	// Open the output file.
	strncpy(mat_filename, path, DEFAULT_PATH_LENGTH);
	strncpy(input_filename_withchannel,input_file_basename,(int)(strlen(input_file_basename))-4);
	strncat(mat_filename,input_filename_withchannel,strlen(input_filename_withchannel));
	//strncat(mat_filename, preamble,strlen(preamble));
	//mat_filename[strlen(mat_filename)-1] = '\0';

	//strncpy(mat_filename, preamble, DEFAULT_PATH_LENGTH);
	//mat_filename[strlen(preamble)-1] = '\0';

	strncat(mat_filename, ".mat", 4);
	//printf("matfilename = %s\n",mat_filename);
	mat_file = matOpen(mat_filename, "w");
	if (mat_file == NULL) {
		fprintf(stderr, "Cannot create mat file.\n");
		exit(EXIT_FAILURE);
	}

	// Create output array structures.
	short_pps_array = mxCreateNumericMatrix(1, short_profile_count, mxUINT64_CLASS, mxREAL);
	 if (short_pps_array == NULL) {
      printf("%s : Out of memory on line %d\n", __FILE__, __LINE__); 
      printf("Unable to create mxArray.\n");
      return(EXIT_FAILURE);
 	 }
	long_pps_array = mxCreateNumericMatrix(1, long_profile_count, mxUINT64_CLASS, mxREAL);
	if (long_pps_array == NULL) {
      printf("%s : Out of memory on line %d\n", __FILE__, __LINE__); 
      printf("Unable to create mxArray.\n");
      return(EXIT_FAILURE);
  	}
	if (format == SIGNED_16) {
		short_array = mxCreateNumericMatrix(samples_per_profile, short_profile_count, mxINT16_CLASS, mxREAL);
		if (short_array == NULL) {
	      printf("%s : Out of memory on line %d\n", __FILE__, __LINE__); 
	      printf("Unable to create mxArray.\n");
	      return(EXIT_FAILURE);
			}
		long_array = mxCreateNumericMatrix(samples_per_profile, long_profile_count, mxINT16_CLASS, mxREAL);
		if (long_array == NULL) {
	      printf("%s : Out of memory on line %d\n", __FILE__, __LINE__); 
	      printf("Unable to create mxArray.\n");
	      return(EXIT_FAILURE);
		}
	} 
	else if (format == UNSIGNED_16) {
		short_array = mxCreateNumericMatrix(samples_per_profile, short_profile_count, mxUINT16_CLASS, mxREAL);
		if (short_array == NULL) {
      		printf("%s : Out of memory on line %d\n", __FILE__, __LINE__); 
	      	printf("Unable to create mxArray.\n");
	      	return(EXIT_FAILURE);
	 	 }
		long_array = mxCreateNumericMatrix(samples_per_profile, long_profile_count, mxUINT16_CLASS, mxREAL);
		if (long_array == NULL) {
      		printf("%s : Out of memory on line %d\n", __FILE__, __LINE__); 
	      	printf("Unable to create mxArray.\n");
	      	return(EXIT_FAILURE);
	 	 }
	} else if (format == SIGNED_32) {
		short_array = mxCreateNumericMatrix(samples_per_profile, short_profile_count, mxINT32_CLASS, mxCOMPLEX);
		if (short_array == NULL) {
      		printf("%s : Out of memory on line %d\n", __FILE__, __LINE__); 
	      	printf("Unable to create mxArray.\n");
	      	return(EXIT_FAILURE);
	 	 }
		long_array = mxCreateNumericMatrix(samples_per_profile, long_profile_count, mxINT32_CLASS, mxCOMPLEX);
		if (long_array == NULL) {
      		printf("%s : Out of memory on line %d\n", __FILE__, __LINE__); 
	      	printf("Unable to create mxArray.\n");
	      	return(EXIT_FAILURE);
	 	 }
	} else if (format == FLOATING_32) {
		short_array = mxCreateNumericMatrix(samples_per_profile, short_profile_count, mxSINGLE_CLASS, mxREAL);
		if (short_array == NULL) {
      		printf("%s : Out of memory on line %d\n", __FILE__, __LINE__); 
	      	printf("Unable to create mxArray.\n");
	      	return(EXIT_FAILURE);
	 	 }
		long_array = mxCreateNumericMatrix(samples_per_profile, long_profile_count, mxSINGLE_CLASS, mxREAL);
		if (long_array == NULL) {
      		printf("%s : Out of memory on line %d\n", __FILE__, __LINE__); 
	      	printf("Unable to create mxArray.\n");
	      	return(EXIT_FAILURE);
	 	 }
	}

	// Write internal profile array structures to output array structures.
	memcpy((void *) mxGetPr(short_pps_array), (void *) short_pps, 1*short_profile_count*sizeof(ULongLongInt));
	memcpy((void *) mxGetPr(long_pps_array), (void *) long_pps, 1*long_profile_count*sizeof(ULongLongInt));
	if (format == SIGNED_16) {
		memcpy((void *) mxGetPr(short_array), (void *) short_profiles_s16, samples_per_profile*short_profile_count*sizeof(ShortInt));
		memcpy((void *) mxGetPr(long_array), (void *) long_profiles_s16, samples_per_profile*long_profile_count*sizeof(ShortInt));
	} 
	else if (format == UNSIGNED_16) {
		memcpy((void *) mxGetPr(short_array), (void *) short_profiles_u16, samples_per_profile*short_profile_count*sizeof(UShortInt));
		memcpy((void *) mxGetPr(long_array), (void *) long_profiles_u16, samples_per_profile*long_profile_count*sizeof(UShortInt));
	} else if (format == SIGNED_32) {
		memcpy(	(void *) mxGetComplexInt32s(short_array), (void *) short_profiles_s32, samples_per_profile*short_profile_count*sizeof(LongInt)*2);
		memcpy( (void *) mxGetComplexInt32s(long_array), (void *) long_profiles_s32, samples_per_profile*long_profile_count*sizeof(LongInt)*2);
	} else if (format == FLOATING_32) {
		memcpy( (void *) mxGetPr(short_array), (void *) short_profiles_f, samples_per_profile*short_profile_count*sizeof(float));
		memcpy( (void *) mxGetPr(long_array), (void *) long_profiles_f, samples_per_profile*long_profile_count*sizeof(float));
	}
	// Write output array structures to output file.
	if (matPutVariable(mat_file, SHORT_PPS_VARIABLE_NAME, short_pps_array) != 0) {
		fprintf(stderr, "Error writing %s to .mat file.\n", SHORT_PPS_VARIABLE_NAME);
		exit(EXIT_FAILURE);
	}
	if (matPutVariable(mat_file, LONG_PPS_VARIABLE_NAME, long_pps_array) != 0) {
		fprintf(stderr, "Error writing %s to .mat file.\n", LONG_PPS_VARIABLE_NAME);
		exit(EXIT_FAILURE);
	}
	if (matPutVariable(mat_file, SHORT_PROFILES_VARIABLE_NAME, short_array) != 0) {
		fprintf(stderr, "Error writing %s to .mat file.\n", SHORT_PROFILES_VARIABLE_NAME);
		exit(EXIT_FAILURE);
	}
	if (matPutVariable(mat_file, LONG_PROFILES_VARIABLE_NAME, long_array) != 0) {
		fprintf(stderr, "Error writing %s to .mat file.\n", LONG_PROFILES_VARIABLE_NAME);
		exit(EXIT_FAILURE);
	}
	printf("Matfile %s created\n",mat_filename);
	// Print out debug information.
	if (parameters.debug_mode) {
		printf("Radar bytes: %lu\n", radar_byte_counter);
		printf("Profiles processed: %I64u\n", profile_count);
		printf("\t\tShort: %I64u\n", short_profile_count);
		printf("\t\t\tIndex: %I64u\n", short_index);
		printf("\t\tLong: %I64u\n", long_profile_count);
		printf("\t\t\tIndex: %I64u\n", long_index);
	}

	// Clean up.
	free(input_file_basename);
	free(path);
	free(preamble);
	free(extended_path);
	free(config_path);
	free(system_path);
	free(id_string);
	free(data_path);
	free(id_num_string);
	free(mat_filename);
	free(input_filename);
	free(header);
	free(byte_in);
	free(sync_string);
	free(pps_counter_string);
	free(format_string);
	free(sample_string);
	free(profile_length_string);
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

	mxDestroyArray(short_pps_array);
	mxDestroyArray(long_pps_array);
	mxDestroyArray(short_array);
	mxDestroyArray(long_array);

	if (matClose(mat_file) != 0) {
		fprintf(stderr, "Error closing .mat file.\n");
		exit(EXIT_FAILURE);
	}
	return 0;
}
