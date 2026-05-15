#include <stdio.h>
#include <stdlib.h>

#include <stdint.h>

#include <math.h>
#include <time.h>

#include <string.h>


typedef uint16_t int2;
typedef uint32_t int4;

// write 4 bytes with 4 chars
void w4c(FILE *f, char *s){
	fwrite(s, sizeof(char), 4, f);
}

// write a 2 byte unsigned integer
void w2i(FILE *f, int2 a){
	fwrite(&a, sizeof(int2), 1, f);
}

// write a 4 byte unsigned integer
void w4i(FILE *f, int4 a){
	fwrite(&a, sizeof(int4), 1, f);
}



typedef struct NoiseSettings{
	
	int2 wave_format;
	
	int2 bits_per_sample;
	int4 sample_rate;
	
	int2 channels;

	uint16_t max_volume;
	
	float duration;
	long order;
	float leak_factor;

	char *output_path;
	
} NoiseSettings;

NoiseSettings create_default_settings(){
	NoiseSettings settings = {
		.wave_format = 0x0001, // WAVE_FORMAT_PCM
		
		.bits_per_sample = 16,
		.sample_rate = 44100,
		
		.channels = 2, // stereo

		.max_volume = 1<<14,

		.duration = 10.f,
		.order = 1, // brown noise
		.leak_factor = 0.99f,

		.output_path = NULL,
	};

	return settings;
}



void print_usage_dialog() {
	puts("Usage: noisegen [OPTION]... FILE");
}

void print_help_dialog() { // terminates
	
	print_usage_dialog();
	puts("Generates audio noise using leaky integration and stores the result in WAV format.");
	puts("Example: noisegen --duration 30.0 --order 5 --leak 0.98 output.wav");

#define OPTION_FORMATTING "  %-24s %s\n"
	
	puts("\nOptions:");
	printf(OPTION_FORMATTING, "-h, --help", "Display this help dialog and exit");

	printf("\n"OPTION_FORMATTING, "-t, --duration SECONDS", "The duration of the output in seconds (float between 0 and 20,000, default: 10.0)");

	printf("\n"OPTION_FORMATTING, "-n, --order N", "The order of integration of the noise (non-negative integer, default: 1)");
	printf(OPTION_FORMATTING, "", "E.g. white noise = 0, brown noise = 1");

	printf("\n"OPTION_FORMATTING, "-l, --leak FACTOR", "The leak factor of the leaky integrator (float between 0.0 and 1.0, default: 0.99)");

	exit(0);
	
}

void print_help_suggestion_dialog() { // terminates
	puts("Try 'noisegen --help' for more information.");
	exit(1);
}

NoiseSettings parse_options(int argc, char *argv[]) {
	NoiseSettings settings = create_default_settings();

	if(argc < 2) {
		print_usage_dialog();
		print_help_suggestion_dialog();
	}

#define NEXT_ARG() if(i+1 >= argc) { printf("Invalid Arguments: %s must be followed with a value", argv[i]); exit(1); } i++

	for(int i = 1; i < argc; i++) {
		if( strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0 ) {
			print_help_dialog();
		}

		if( strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--duration") == 0 ) {
			NEXT_ARG();
			char *endptr; float duration = strtof(argv[i], &endptr);
			if(*endptr != '\0') {
				printf("Invalid Argument '%s' for %s\n", argv[i], argv[i-1]);
				print_help_suggestion_dialog();
			}
			if(duration < 0 || duration > 20000) {
				printf("Invalid Argument '%s' for %s, must be between 0 and 20,000\n", argv[i], argv[i-1]);
				print_help_suggestion_dialog();
			}
			settings.duration = duration;
			continue;
		}
		
		if( strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "--order") == 0 ) {
			NEXT_ARG();
			char *endptr; long order = strtol(argv[i], &endptr, 10);
			if(*endptr != '\0') {
				printf("Invalid Argument '%s' for %s\n", argv[i], argv[i-1]);
				print_help_suggestion_dialog();
			}
			if(order < 0) {
				printf("Invalid Argument '%s' for %s, must not be negative\n", argv[i], argv[i-1]);
				print_help_suggestion_dialog();
			}
			settings.order = order;
			continue;
		}
		
		if( strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "--leak") == 0 ) {
			NEXT_ARG();
			char *endptr; float leak_factor = strtof(argv[i], &endptr);
			if(*endptr != '\0') {
				printf("Invalid Argument '%s' for %s\n", argv[i], argv[i-1]);
				print_help_suggestion_dialog();
			}
			if(leak_factor < 0.f || leak_factor > 1.f) {
				printf("Invalid Argument '%s' for %s, must be between 0.0 and 1.0\n", argv[i], argv[i-1]);
				print_help_suggestion_dialog();
			}
			settings.leak_factor = leak_factor;
			continue;
		}

		if(argv[i][0] == '-') {
			printf("Invalid Argument '%s'\n", argv[i]);
			print_help_suggestion_dialog();
		}

		if(i != argc-1) {
			printf("Invalid Argument '%s', output file must be specified as the final argument\n", argv[i]);
			print_help_suggestion_dialog();
		}

		settings.output_path = argv[i];
	}

#undef NEXT_ARG

	if(settings.output_path == NULL) {
		puts("Invalid Arguments: no output file specified");
		print_help_suggestion_dialog();
	}
	
	return settings;
}



void write_header(FILE *f, NoiseSettings *settings){
	
	int2 block_align = settings->channels * (settings->bits_per_sample / 8);
	int4 byte_rate = settings->sample_rate * block_align;
	
	/*
	  RIFF header
	*/
	
	w4c(f, "RIFF");
	w4i(f, 0); // placeholder    

	w4c(f, "WAVE");

	
	/*
	  fmt chunk
	*/
	
	w4c(f, "fmt ");
	w4i(f, 16);
	
	w2i(f, settings->wave_format);
	
	w2i(f, settings->channels);
	
	w4i(f, settings->sample_rate);
	
	w4i(f, byte_rate);
	w2i(f, block_align);
	
	w2i(f, settings->bits_per_sample);
	
}

void finalise_header(FILE *f){
	/*
	  finalise cksize in RIFF header
	*/
	
	fseek(f, 0, SEEK_END);
	long file_size = ftell(f);
	
	fseek(f, 4, SEEK_SET);
	w4i(f, file_size - 8);
}



void flush_samples(FILE *f, float *sample_buffer, size_t flush_size, NoiseSettings *settings) {
	
	float max_value = 0.f;
	for(int i = 0; i < flush_size; i++) { // find max value for normalisation
		
		if( fabsf( sample_buffer[i] ) > max_value ) {
			max_value = fabsf( sample_buffer[i] );
		}
		
	}
	
	static float max_value_prev = NAN; 
	if( isnan(max_value_prev) ) {
		max_value_prev = max_value; // no interpolation for first flush
	}
	
	for(int i = 0; i < flush_size; i++) { // write normalised samples

		float lerp_amount = (float)i / (float)flush_size; // interpolate between previous normalisation and current to avoid artefacts
		float normalise_to = max_value*lerp_amount + max_value_prev*(1-lerp_amount);

		float sample_normalised = sample_buffer[i] / normalise_to;
		int16_t sample_s16 = sample_normalised * settings->max_volume;

		fwrite(&sample_s16, sizeof(sample_s16), 1, f);
	}

	max_value_prev = max_value;
}

void write_data(FILE *f, NoiseSettings *settings){
	
	w4c(f, "data");

	long cksize_pos = ftell(f);
	w4i(f, 0); // placeholder

	long total_samples = settings->duration * settings->sample_rate * settings->channels;

	int flush_size = settings->sample_rate * settings->channels; // flush 5 second of audio per flush
	float *sample_buffer = malloc(flush_size * sizeof(float)); // stores samples to be flushed to file (and normalised)

	float *values = calloc( (settings->order + 1) * settings->channels, sizeof(float) ); // stores displacements for each order of integration
#define VALUE_AT(order, channel) values[ (order) * settings->channels + (c) ]

	
	int s;
	for(s = 0; s < total_samples; s++) {
	   
		if(s > 0 && s % flush_size == 0) {
			flush_samples(f, sample_buffer, flush_size, settings);
		}
		
		int c = s % settings->channels;
		VALUE_AT(0, c) = ((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f;
		for(int o = 1; o <= settings->order; o++) {

			VALUE_AT(o, c) *= settings->leak_factor;
			VALUE_AT(o, c) += VALUE_AT(o-1, c) * (1 - settings->leak_factor); // absolute value cannot exceed 1

		}

		float sample = VALUE_AT(settings->order, c);
		sample_buffer[s % flush_size] = sample;
			
	}

	if( s % flush_size == 0 ) {
		flush_samples(f, sample_buffer, flush_size, settings);
	}
	else {
		flush_samples(f, sample_buffer, s % flush_size, settings);
	}

#undef VALUE_AT

	free(values);
	free(sample_buffer);
	

	long end_pos = ftell(f);

	fseek(f, cksize_pos, SEEK_SET);
	w4i(f, end_pos - cksize_pos);
	
}



void check_file_exists(char *output_path) {
	FILE *fp = fopen(output_path, "r");
    if(!fp) {
		return;
	}
	fclose(fp);

	printf("File '%s' already exists. Overwrite? [y/N] ", output_path);

	int response = getchar();

	if(response != '\n') { // flush stdin
		int c;
		while( (c = getchar()) != '\n' && c != EOF ){ } 
	}

	if(response == 'y' || response == 'Y') {
		return;
	}

	exit(0);
}

void check_file_extension(char *output_path) {
	char *extension = strrchr(output_path, '.');

	if(extension != NULL) {
		if(strcmp(extension, ".wav") == 0) {
			return;
		}
		if(strcmp(extension, ".wave") == 0) {
			return;
		}
	}

	printf("Invalid output file: '%s'. Please use a valid file extension (.wav or .wave)\n", output_path);
	print_help_suggestion_dialog();
	exit(1);
}



int main(int argc, char *argv[]) {
	srand(time(NULL));

	NoiseSettings settings = parse_options(argc, argv);

	check_file_extension(settings.output_path);
	check_file_exists(settings.output_path);
		
	FILE *f = fopen(settings.output_path, "wb");
	if(!f) {
		printf("Failed to open %s for writing\n", settings.output_path);
		return 1;
	}

	if(settings.order == 0) {
		puts("Warning: white noise (order 0) can sound very harsh and loud, so lowering volume is advised during playback");
	}

	printf("Writing to %s\n", settings.output_path);

	write_header(f, &settings);
	write_data(f, &settings);
	finalise_header(f);

	if(ferror(f)) {
		puts("An error occured during writing, contents likely corrupted");
		return 1;
	}
	
	fclose(f);

	return 0;
}
