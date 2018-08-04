#ifndef ADR_H_
#define ADR_H_

#define TRUE							1
#define FALSE							0

#define DEFAULT_PATH_LENGTH				512
#define DEFAULT_NUMBER_LENGTH			32
#define PREAMBLE_LENGTH					16

#define ID_STRING_OFFSET				16

#define HEADER_LENGTH					32
#define ARENA_PAYLOAD_LENGTH_OFFSET		12
#define ARENA_PAYLOAD_LENGTH_LENGTH		4

#define SYNC_LENGTH						8

enum data_format_type {
	SIGNED_16,
	UNSIGNED_16,
	SIGNED_32,
	FLOATING_32
};

struct input_parameters {
	char *target_path;
	_Bool *file_limit;
	unsigned short int *max_files;
};

struct input_parameters parse_command_line_parameters(int, char **);
unsigned long long data_to_num (unsigned char *, unsigned short int);
_Bool get_next_byte (FILE *, unsigned char *, _Bool, unsigned char *);

#endif /* ADR_H_ */
