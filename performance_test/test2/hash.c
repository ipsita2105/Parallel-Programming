#include<stdio.h>
#include<stdlib.h>
#include<limits.h>

#define SIZE 10

struct entry_s{

	int data;
	int key;
	struct entry_s *next;
};

typedef struct entry_s entry_t;

struct hashtable_s{

	struct entry_s **table;
};

typedef struct hashtable_s hashtable_t;

//create a new hashtable

hashtable_t* ht_create(){
	
	hashtable_t* hashtable = NULL;
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

	return hashtable;
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
void ht_set( hashtable_t* hashtable, int key, int data){

	int bin =0;
	entry_t* newpair = NULL;
	entry_t* next = NULL;
	entry_t* last = NULL;

	bin = ht_hash(key);

	next = hashtable->table[bin];

	while(next != NULL && next->key !=NULL && key != next->key){
		last = next;
		next = next->next;
	}

	//if there is already a pair. replace that?
	
	if(next != NULL && next->key !=NULL && key == next->key){
	
		free(next->data);
		next->data = data;
	
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

}

// get key-value pair from hash table
int ht_get(hashtable_t *hashtable, int key){


	int bin =0;
	entry_t* pair;

	bin = ht_hash(key);

	//find our value
	pair = hashtable->table[bin];

	while(pair != NULL && pair->key != NULL && key != pair->key){
		pair = pair->next;
	}

	//did we actually find it?
	if(pair == NULL || pair->key == NULL || key != pair->key){
		return NULL;
	}
	else{
		return pair->data;
	}
	
}

int main(){

	hashtable_t* hashtable = ht_create();

	ht_set(hashtable, 1, 11);
	ht_set(hashtable, 2, 22);
	ht_set(hashtable, 3, 33);
	ht_set(hashtable, 4, 44);

	printf("%d\n", ht_get(hashtable, 1));
	printf("%d\n", ht_get(hashtable, 2));
	printf("%d\n", ht_get(hashtable, 3));
	printf("%d\n", ht_get(hashtable, 4));

	return 0;

}
















