#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <string.h>
#include <time.h>
#include <cfuhash.h>

#define INITIAL_HASH_SIZE 1000
#define MAX_WORD_LENGTH 40
#define MAX_FILE_LENGTH 100
#define PATH_TO_DATA "../data/"
#define SHAKESPEARE "ShakespeareComplete.txt"
#define INPUTS_COUNT 3
#define INPUTS {"AToTCities.txt", "JttCoftE.txt", "WaP.txt"}

int NUM_THREADS = 4;

char shakespearePath[MAX_FILE_LENGTH];

char inputPaths[INPUTS_COUNT][MAX_FILE_LENGTH];

char input_files[][MAX_FILE_LENGTH] = INPUTS;

char *shakespeare;
int shakespeare_size;
char *inputs[INPUTS_COUNT];

int *start_pos;

cfuhash_table_t *shakespeare_hash;


// prototypes

void readShakespeare(void);
void readInputs(void);
int isAlphaNum(char);
void lcase(char*);

// PARAMS:
// -t #NUM indicates number of threads to run on
// -if #FILE indicates the path to the file to work on
// -sf #FILE indicates the path of the shakespeare filez

int main(int argc, char **argv) {
	printf("Hello world! Args:\n");

	// Assign paths
	strcpy(shakespearePath, PATH_TO_DATA);
	strcat(shakespearePath, SHAKESPEARE);

	{
		int i;
		for(i = 0; i < INPUTS_COUNT; i++) {
			strcpy(inputPaths[i], PATH_TO_DATA);
			strcat(inputPaths[i], input_files[i]);
			printf("INPUT FILE %d:%s\n", i+1,inputPaths[i]);
		}
	}

	/*int i;
	for(i = 0; i < argc; i++) {
		printf("%s\n", argv[i]);
	}*/

	readShakespeare();
	readInputs();

	// Divide up input
	start_pos = malloc(sizeof(int)*(NUM_THREADS + 1));
	start_pos[NUM_THREADS] = shakespeare_size;

	{
		int i;
		start_pos[0] = 0;
		for(i = 1; i < NUM_THREADS; i++) {
			start_pos[i] = start_pos[i-1] + (shakespeare_size/NUM_THREADS);
		}

		for(i = 1; i < NUM_THREADS; i++) {
			int dir;
			// Alternate direction to make sure each block is
			// roughly the same size;
			dir = (i%2)*2 - 1;

			while(isAlphaNum(shakespeare[start_pos[i]])) {
				start_pos[i] += dir;
			}

		}
	}


	// DEBUG: Print the things
	{
		int i;
		for(i = 0; i < NUM_THREADS; i++) {
			printf("start_pos[%d] = %d, Len:%d, char:'%c'\n", i,start_pos[i], start_pos[i+1] - start_pos[i], shakespeare[start_pos[i]]);
		}
	}


	shakespeare_hash = cfuhash_new_with_initial_size(INITIAL_HASH_SIZE);
	// CFU hash functions:
	// cfuhash_put, cfuhash_delete, cfuhash_get, cfuhash_exists, cfuhash_clear, cfuhash_destroy
	// cfuhash_pretty_print.
	
	/*cfuhash_put(hash, "key1", "1");
	cfuhash_put(hash, "key2", "2");
	cfuhash_put(hash, "key3", "3");
	cfuhash_pretty_print(hash, stdout);*/

	// Do word counting in parallel:
	omp_set_num_threads(NUM_THREADS);
	#pragma omp parallel
	{
		int id = omp_get_thread_num();
		int i = 0, inword = 0;
		char thisword[MAX_WORD_LENGTH];
		printf("Hello from thread %d\n", id);
		for(i = start_pos[id]; i < start_pos[id+1]; i++) {
			if(isAlphaNum(shakespeare[i])) {
				inword = 1;
			} else {

				// If we were in a word, we have 
				// reached the end.
				if(inword) {
					int j;
					for(j = 0; j < MAX_WORD_LENGTH && thisword[j] != '\0'; j++) {
						thisword[j] = '\0';
					}

					inword = 0;
				}

			}

			// Clear word
			
		}
	}


}

int isAlphaNum(char c) {
	return ((c >= 65 && c <= 90) || (c >= 97 && c <= 122));
}

void readArgs(int argc, char **argv) {
	int i;
	for(i = 0; i < argc; i++) {
		if(strcmp(argv[i], "-t")) {
			break;
		}
	}
}

// Read shakespeare to buffer
void readShakespeare() {
	FILE *fp;
	fp = fopen(shakespearePath, "rb");
	fseek(fp, 0L, SEEK_END);
	int size = ftell(fp);
	fseek(fp, 0L, SEEK_SET);
	shakespeare = malloc(sizeof(char)*size);
	fread(shakespeare, sizeof(char), size, fp);
	fclose(fp);
	shakespeare_size = size;
}


// Read inputs to buffer
void readInputs() {
	FILE *fp;
	int i;
	for(i = 0; i < INPUTS_COUNT; i++) {
		fp = fopen(inputPaths[i], "rb");
		fseek(fp, 0L, SEEK_END);
		int size = ftell(fp);
		fseek(fp, 0L, SEEK_SET);
		inputs[i] = malloc(sizeof(char)*size);
		fread(inputs[i], sizeof(char), size, fp);
		fclose(fp);
	}
	
}

// Convert an ASCII string to lower case (in place)
void lcase(char* string) {
	int i;
	for(i = 0; string[i] != '\0'; i++) {
		if(string[i] <= 90 && string[i] >= 65) {
			string[i] += 32;
		}
	}
}