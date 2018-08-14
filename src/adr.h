#ifndef ADR_H_
#define ADR_H_

#define VERSION							"2.00"

#define TRUE							1
#define FALSE							0

#define LONG_MODE						0
#define SHORT_MODE						3

#define RADAR_FILENAME					"radar.tmp.dat"

#define SHORT_PPS_VARIABLE_NAME			"Short_Chirp_PPS_Count_Values"
#define LONG_PPS_VARIABLE_NAME			"Long_Chirp_PPS_Count_Values"
#define SHORT_PROFILES_VARIABLE_NAME	"Short_Chirp_Profiles"
#define LONG_PROFILES_VARIABLE_NAME		"Long_Chirp_Profiles"

#define DEFAULT_PATH_LENGTH				512
#define DEFAULT_NUMBER_LENGTH			32
#define PREAMBLE_LENGTH					16

#define ID_STRING_OFFSET				16

#define HEADER_LENGTH					32
#define ARENA_PAYLOAD_LENGTH_OFFSET		12
#define ARENA_PAYLOAD_LENGTH_LENGTH		4

#define SYNC_LENGTH						8
#define RADAR_HEADER_TYPE_LENGTH		4
#define RADAR_HEADER_LENGTH_LENGTH		4
#define MODE_LENGTH						1
#define SUBCHANNEL_DATA_SOURCE_LENGTH	1
#define RESERVED_A_LENGTH				6
#define ENCODER_LENGTH					4
#define RESERVED_B_LENGTH				4
#define RELATIVE_COUNTER_LENGTH			8
#define PROFILE_COUNTER_LENGTH			8
#define PPS_FRACTIONAL_COUNTER_LENGTH	8
#define PPS_COUNTER_LENGTH				8
#define PROFILE_DATA_FORMAT_LENGTH		4
#define PROFILE_LENGTH_LENGTH			4

#define DEFAULT_PROFILE_COUNT			32768
#define DEFAULT_UDP_PAYLOAD_LENGTH		8192
#define DEFAULT_PROFILE_BYTE_LENGTH		32768

enum data_format_type {
	SIGNED_16,
	UNSIGNED_16,
	SIGNED_32,
	FLOATING_32
};

enum radar_header_segments {
	SYNC,
	RADAR_HEADER_TYPE,
	RADAR_HEADER_LENGTH,
	MODE,
	SUBCHANNEL_DATA_SOURCE,
	RESERVED_A,
	ENCODER,
	RESERVED_B,
	RELATIVE_COUNTER,
	PROFILE_COUNTER,
	PPS_FRACTIONAL_COUNTER,
	PPS_COUNTER,
	PROFILE_DATA_FORMAT,
	PROFILE_LENGTH,
	PROFILE_DATA
};

struct input_parameters {
	char *target_path;
	_Bool *file_limit;
	unsigned short int *max_files;
	_Bool debug_mode;
	unsigned short int range;
};

struct input_parameters parse_command_line_parameters(int, char **);
unsigned long long data_to_num (unsigned char *, unsigned short int);
_Bool check_sync (unsigned char *);
unsigned long long int resync (FILE *, unsigned char *, struct input_parameters);

#endif /* ADR_H_ */
