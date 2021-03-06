#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <string.h>
#include <time.h>
#include <search.h>
#include <errno.h>

#define HASH_TABLE_SIZE 10000000
#define MAX_WORDS 1000000
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

unsigned int words = 0;

//cfuhash_table_t *shakespeare_hash;

typedef struct {
	int count;
	omp_lock_t *lock;
} word_count_t;

// Hash table of values
struct hsearch_data *hash;

ENTRY *new[MAX_WORDS]; 


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
			//int dir;
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


	// Do word counting in parallel:
	//word_count_t *new; //= malloc(sizeof(word_count_t));
						//omp_lock_t *newlock;// = malloc(sizeof(omp_lock_t));
						//omp_init_lock(newlock);
		
	omp_set_num_threads(NUM_THREADS);
	//#pragma omp parallel shared(hash, new, shakespeare)//num_threads(NUM_THREADS)
	{
		ENTRY e, *ep;
		int id = omp_get_thread_num();
		int i = 0, inword = 0;
		char thisword[MAX_WORD_LENGTH];
		//memset(thisword, 0, sizeof(char)*MAX_WORD_LENGTH);
		thisword[0] = '\0';
		//printf("thisword:%s", thisword);
		//printf("Hello from thread %d\n", id);
		// TODO does this skip the last character?
		for(i = start_pos[id]; i < start_pos[id+1]; i++) {
			//printf("'%c'", shakespeare[i]);
			if(isAlphaNum(shakespeare[i])) {
				inword = 1;
				//printf("About to strncat %s\n", thisword);
				strncat(thisword, &shakespeare[i],1);
				//printf("Just did a strncat %s\n", thisword);
			} else {

				// If we were in a word, we have 
				// reached the end.
				if(inword) {

					//printf("lcase the word '%s'\n", thisword);
					lcase(thisword);
					e.key = thisword;
					//printf("Finding key..%s in hash %p\n", e.key, hash);
					hsearch_r(e, FIND, &ep, hash);
					//printf("done\n");
					if(ep) {
						printf("Old word\n");
						//printf("found\n");
						// If an entry exists for this word, aquire a lock
						// then update the count.
						omp_set_lock(((word_count_t*)ep->data)->lock);
						((word_count_t*)ep->data)->count++;
						omp_unset_lock(((word_count_t*)ep->data)->lock);
					} else {

						//printf("New word\n");
						//printf("Creating new entry...\n");
						
						//e.key = "thy";
						//hsearch_r(e, FIND, &ep, hash);
						//printf("About to enter critical section\n");
						int i;
						//#pragma omp critical
						{
							//printf("Executing critical code\n");
							i = words; 
							words++;
						}
						//printf("Exited critical section. i=%d words=%u\n", i, words);
						new[i] = malloc(sizeof(ENTRY));

						new[i]->data = malloc(sizeof(word_count_t));
						((word_count_t*)new[i]->data)->lock = malloc(sizeof(omp_lock_t));
						omp_init_lock(((word_count_t*)new[i]->data)->lock);

						((word_count_t*)new[i]->data)->count = 1;

						ENTRY *result;// = malloc(sizeof(ENTRY));
						//e.data = new;

						int err;

						//#pragma omp critical
						{
							printf("about to enter item... i: %d/%d, words:%u, new:%p, result:%p, hash:%p\n", i, sizeof(new)/sizeof(ENTRY*), words, new[i], &result, hash);
							err = hsearch_r(*new[i], ENTER, &result, hash);
						}

						
						if(err) {
							printf("Entered hash\n");
							if(err != 1) {
								printf("ERROR ENTERING HASH: %d\n", err);
								if(err == ENOMEM) { 
									printf("ENOMEM\n");
								} else if (err == ESRCH) {
									printf("ESRCH\n");
								}
							}


						} else {
							//printf("Entered hash!\n");
						}

						words++;
						printf("new word, words=%d\n", words);

					//printf("Added thing %p, %s, %p", ep, ep->key, ep->data);
					//exit(0);
					}

					// Clear out the current word.
					//memset(thisword, 0, sizeof(char)*MAX_WORD_LENGTH);
					thisword[0] = '\0';
					//clear_string(thisword);
					inword = 0;
				}

			}

			//printf("Words: %u\n", words);




	}

		// Debug code, do we have anything here?
		ENTRY et, *ept;
		et.key = "thy";
		lcase(et.key);
		hsearch_r(et, FIND, &ept, hash);
		hsearch_r(et, FIND, &ept, hash);
		if(ept) {

			printf("Found here! C:%d, TID:%d\n", ((word_count_t*)ept->data)->count, id);
		} else {
			printf("Not even here, seriously? TID:%d\n", id);
		}
			
		}	


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
						//printf("ERROR IN SEARCH\n");
					}
					if(ep) {
						//printf("Known hash, count:%d", ((word_count_t*)ep->data)->count);
						count = ((word_count_t*)ep->data)->count;
					} else {
						//printf("Unknown hash %p\n", ep);
						//count = cfuhash_get(shakespeare_hash, thisword);
					}

					// TODO uncomment this.
					printf("%s #%d\n", thisword, count);


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
	printf("SSIZE: %d\n", size);
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
		printf("SIZE INPUT:%d\n", size);
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


void clear_string(char* string){
	int i;
	for(i = 0; string[i] != '\0'; i++) {
		string[i] = '\0';
	}
}
