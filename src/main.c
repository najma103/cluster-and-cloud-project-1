#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <string.h>
#include <time.h>
#include <search.h>
#include <errno.h>

#define HASH_TABLE_SIZE 10000000
#define MAX_WORD_LENGTH 40
#define MAX_FILE_LENGTH 100
#define PATH_TO_DATA "../data/"
#define SHAKESPEARE "ShakespeareComplete.txt"
#define INPUTS_COUNT 3
#define INPUTS {"AToTCities.txt", "JttCoftE.txt", "WaP.txt"}

int NUM_THREADS = 1;

char shakespearePath[MAX_FILE_LENGTH];

char inputPaths[INPUTS_COUNT][MAX_FILE_LENGTH];

char input_files[][MAX_FILE_LENGTH] = INPUTS;

char *shakespeare;
int shakespeare_size;
char *inputs[INPUTS_COUNT];

int *start_pos;

//cfuhash_table_t *shakespeare_hash;

typedef struct {
	int count;
	omp_lock_t *lock;
} word_count_t;

// Hash table of values
struct hsearch_data *hash;


// prototypes

void readShakespeare(void);
void readInputs(void);
int isAlphaNum(char);
void lcase(char*);
//void print_hash(cfuhash_table_t*);
void clear_string(char*);

// PARAMS:
// -t #NUM indicates number of threads to run on
// -if #FILE indicates the path to the file to work on
// -sf #FILE indicates the path of the shakespeare filez

int main(int argc, char **argv) {
	//printf("Hello world! Args:\n");

	// Assign paths
	strcpy(shakespearePath, PATH_TO_DATA);
	strcat(shakespearePath, SHAKESPEARE);

	{
		int i;
		for(i = 0; i < INPUTS_COUNT; i++) {
			strcpy(inputPaths[i], PATH_TO_DATA);
			strcat(inputPaths[i], input_files[i]);
			//printf("INPUT FILE %d:%s\n", i+1,inputPaths[i]);
		}
	}

	/*int i;
	for(i = 0; i < argc; i++) {
		printf("%s\n", argv[i]);
	}*/


	// Read input files to buffer
	readShakespeare();
	readInputs();

	//printf("Creating hashmap..\n");
	hash = malloc(sizeof(struct hsearch_data));
	*hash = (struct hsearch_data){0};
	//memset(hash, 0, sizeof(*hash));
	hcreate_r(HASH_TABLE_SIZE, hash);
	//printf("done %p\n", hash);

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
			//dir = (i%2)*2 - 1;

			while(isAlphaNum(shakespeare[start_pos[i]])) {
				start_pos[i]++;
			}

		}
	}

	//printf("Divided problem\n");

	// Initialise hash table of words


	// DEBUG: Print the things
	/*{
		int i;
		for(i = 0; i < NUM_THREADS; i++) {
			printf("start_pos[%d] = %d, Len:%d, char:'%c'\n", i,start_pos[i], start_pos[i+1] - start_pos[i], shakespeare[start_pos[i]]);
		}
	}*/



	// Do word counting in parallel:
	omp_set_num_threads(NUM_THREADS);
	#pragma omp parallel num_threads(NUM_THREADS)
	{
		int id = omp_get_thread_num();
		int i = 0, inword = 0;
		char thisword[MAX_WORD_LENGTH];
		memset(thisword, 0, sizeof(char)*MAX_WORD_LENGTH);
		//printf("thisword:%s", thisword);
		//printf("Hello from thread %d\n", id);
		// TODO does this skip the last character?
		for(i = start_pos[id]; i < start_pos[id+1]; i++) {
			//printf("'%c'", shakespeare[i]);
			if(isAlphaNum(shakespeare[i])) {
				inword = 1;
				strncat(thisword, &shakespeare[i],1);
			} else {

				// If we were in a word, we have 
				// reached the end.
				if(inword) {
					//printf("lcase the word '%s'\n", thisword);
					lcase(thisword);
					ENTRY e, *ep;
					e.key = thisword;
					//printf("Finding key..%s in hash %p\n", e.key, hash);
					hsearch_r(e, FIND, &ep, hash);
					//printf("done\n");
					if(ep) {
						//printf("found\n");
						// If an entry exists for this word, aquire a lock
						// then update the count.
						//printf("Incrementing word count...%d\n", ((word_count_t*)ep->data)->count);
						omp_set_lock(((word_count_t*)ep->data)->lock);
						((word_count_t*)ep->data)->count++;
						omp_unset_lock(((word_count_t*)ep->data)->lock);
					} else {
						//printf("Creating new entry...\n");
						word_count_t *new = malloc(sizeof(word_count_t));
						omp_lock_t *newlock;
						omp_init_lock(newlock);
						new->count = 0;
						new->lock = newlock;

						e.data = new;

						int err;

						if(err = hsearch_r(e, ENTER, &ep, hash)) {
							printf("ERROR ENTERING HASH\n");
							if(err = ENOMEM) { 
								printf("ENOMEM\n");
							} else if (err = ESRCH) {
								printf("ESRCH\n");
							}
						}
						//printf("Added thing %p, %s, %p", ep, ep->key, ep->data);
						//exit(0);
					}

					// Clear out the current word.
					memset(thisword, 0, sizeof(char)*MAX_WORD_LENGTH);
					//clear_string(thisword);
					inword = 0;
				}

			}

			
		}
	}

	//cfuhash_pretty_print(shakespeare_hash, stdout);
	//print_hash(shakespeare_hash);

	// Iterate through the input file and print out the word counts7
	{
		int i, inword = 0;
		char thisword[MAX_WORD_LENGTH];
		memset(thisword, 0, sizeof(char)*MAX_WORD_LENGTH);
		for(i = 0; inputs[0][i] != '\0'; i++) {
			if(isAlphaNum(inputs[0][i])) {
				inword = 1;
				strncat(thisword, &inputs[0][i],1);
			} else {
				if(inword) {
					int count;
					count = 0;
					lcase(thisword);
					ENTRY e, *ep;
					e.key = thisword;
					//printf("Finding key [%s]", thisword);
					if(hsearch_r(e, FIND, &ep, hash)) {
						printf("ERROR IN SEARCH\n");
					}
					if(ep) {
						printf("Known hash, count:%d", ((word_count_t*)ep->data)->count);
						count = ((word_count_t*)ep->data)->count;
					} else {
						//printf("Unknown hash\n");
						//count = cfuhash_get(shakespeare_hash, thisword);
					}

					// TODO uncomment this.
					//printf("%s #%d\n", thisword, count);
					/*for(j = 0; j < MAX_WORD_LENGTH; j++) {
						printf("%d,", (int)thisword[j]);
					}
					printf("\n");*/

				}
				inword = 0;
				memset(thisword, 0, sizeof(char)*MAX_WORD_LENGTH);
			}
		}
	}

	//print_hash(shakespeare_hash);


}

int isAlphaNum(char c) {
	return ((c >= 65 && c <= 90) || (c >= 97 && c <= 122) || (c >= 48 && c <= 57));
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
	if(!(fp = fopen(shakespearePath, "rb"))){
		printf("Could not open file %s\n", shakespearePath);
	}
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
		if(!(fp = fopen(inputPaths[i], "rb"))){
			printf("Could not open file %s\n", inputPaths[i]);
			exit(0);
		}
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

/*void print_hash(cfuhash_table_t *hash) {
	char **keys;
	size_t key_count;
	size_t *key_sizes;
	keys = (char **)cfuhash_keys_data(hash, &key_count, &key_sizes, 0);
	printf("{\n");
	int i;
	for(i = 0; i < key_count; i++) {
		int *val = (int*)cfuhash_get(shakespeare_hash, keys[i]);
		printf("\t\"%s\" => %d\n", keys[i], *val);
	}
	printf("}\n");
}*/

void clear_string(char* string){
	int i;
	for(i = 0; string[i] != '\0'; i++) {
		string[i] = '\0';
	}
}