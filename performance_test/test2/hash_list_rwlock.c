#include<stdio.h>
#include<stdlib.h>
#include<limits.h>
#include<pthread.h>
#include<time.h>
#include<unistd.h>

#define SIZE 100
#define NUM_THREADS 2
#define NUM_OPER 400
#define PERCENT_INSERT 30

pthread_rwlock_t rwlocks[SIZE];

struct entry_s{

        int data;
        int key;
        struct entry_s *next;
};

typedef struct entry_s entry_t;

struct hashtable_s{

        struct entry_s **table;
} *hashtable;

typedef struct hashtable_s hashtable_t;


//create a new hashtable
void  ht_create(){
	
	int i;

	if(SIZE < 1) return NULL;

	//Allocate the table
	if((hashtable = malloc(sizeof(hashtable_t))) == NULL){
		return NULL;
	}

	//Allocate pointers to head nodes
	if((hashtable->table = malloc(sizeof(entry_t*)*SIZE)) == NULL){
		return NULL;
	}

	for(i=0; i< SIZE; i++){
		hashtable->table[i] = NULL;
	}

}

//the hash function
int ht_hash(int key){
	
	return key%SIZE;
}

//create key-value pair
entry_t *ht_newpair(int key, int data){

	entry_t *newpair;

	if((newpair = malloc(sizeof(entry_t))) == NULL){
		return NULL;
	}

	if((newpair->data = data) == NULL){
		return NULL;
	}

	if((newpair->key = key) == NULL){
		return NULL;
	}
	
	newpair->next = NULL;

	return newpair;
}

//insert key-value pair into hash table
void ht_set(int key, int data){

	int bin =0;
	entry_t* newpair = NULL;
	entry_t* next = NULL;
	entry_t* last = NULL;

	bin = ht_hash(key);

	pthread_rwlock_wrlock(&rwlocks[bin]); //lock this list
	next = hashtable->table[bin];

	while(next != NULL && next->key !=NULL && key != next->key){
		last = next;
		next = next->next;
	}

	//if there is already a pair. do nothing
	
	if(next != NULL && next->key !=NULL && key == next->key){

		//printf("here\n");	
		//free(next->data);
		//next->data = data;
		pthread_rwlock_unlock(&rwlocks[bin]);
		return;
	
	//if not found time to grow a pair
	} 
	
	  else {
	
		newpair = ht_newpair(key, data);

		// we are at start of linked list of this bin
		if(next == hashtable->table[bin]){
			newpair->next = next;
			hashtable->table[bin] = newpair;
		
		}

		// we are end of the linked list in this bin

		else if(next == NULL){
			last->next = newpair;
		}

		// we are in the middle of the list
		else{
			newpair->next = next;
			last->next = newpair;
		}
	
	}

	pthread_rwlock_unlock(&rwlocks[bin]);

}

// get key-value pair from hash table
int ht_get(int key){


	int bin =0;
	entry_t* pair;

	bin = ht_hash(key);

	pthread_rwlock_rdlock(&rwlocks[bin]); //get lock
	
	//find our value
	pair = hashtable->table[bin];

	while(pair != NULL && pair->key != NULL && key != pair->key){
		pair = pair->next;
	}

	//did we actually find it?
	if(pair == NULL || pair->key == NULL || key != pair->key){

		pthread_rwlock_unlock(&rwlocks[bin]);
		return -1;
	}
	else{

		pthread_rwlock_unlock(&rwlocks[bin]);
		return pair->data;
	}
	
}


void* thread_insert(void* tnum){

	int my_rank = (int)tnum;
	long long int my_num_opr = NUM_OPER/NUM_THREADS; //some multiple of 100
	long long int num_loops = my_num_opr/100;

	int r;

	srand(time(NULL));

//	pthread_mutex_lock(&table_lock);

	for(long long int i=0; i<num_loops; i++){

		//do percent inserts
		for(int j=0; j<PERCENT_INSERT; j++){
			r = rand()%500 + 1;
			printf("inserted %d\n", r);
			ht_set(r, r*11);
		}	

		for(int j=0; j<100-PERCENT_INSERT; j++){
		        r = rand()%500 + 1;
			printf("search %d\n", r);
			ht_get(r);
		}
	}

//	pthread_mutex_unlock(&table_lock);
}


int main(){

	ht_create();

	//lets first keep it populated	
	for(int v=1; v<=100; v++){
		ht_set(v, v*11);
	}

	for(int i=0; i<SIZE; i++){
		
		pthread_rwlock_init(&rwlocks[i], NULL);
	}

	clock_t begin = clock();

	pthread_t work_threads[NUM_THREADS];
	for(int t=0; t<NUM_THREADS; t++){
		
		pthread_create(&work_threads[t], NULL, thread_insert, (void *)t);
	}

	for(int t=0; t<NUM_THREADS; t++){
		pthread_join(work_threads[t], NULL);
	}

	clock_t end = clock();
	printf("TIME TAKEN = %f s\n",(double)(end-begin)/CLOCKS_PER_SEC);
	return 0;

}
