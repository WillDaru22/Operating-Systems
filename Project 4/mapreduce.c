//mapreduce.c by WillDaru22
// Pretty sure this has issues but uploading this to chronicle my attempt at it anyways from a few years ago.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "mapreduce.h"

//Data Structures and Global Vars
int argnum;
int numberParts;
int mapnum;
int whichMapper = 0;
Mapper mapfunc;
Combiner combinefunc;
Reducer reducefunc;

struct partitionStruct {  //structure for each partition
  long partition;
  int curr;  //number of current keyValuePairs
  int max;  //max number of pairs
  int next;
  struct keyValuePair *pairs;
};

struct keyValuePair {
  int curr;  //current number of elements in values
  int max;  //max size of values
  int keyGot;  //shows whether we have got this key for get_red_key
  int visited;  //how many elements in values we have visited
  char **values;
  char *key;
};

struct valueNode {  //node to hold values
  char* val;
  int visited;
  struct valueNode *next;
};

struct keyNode {  //node to hold keys
  char* key;
  struct valueNode *values;  //list of values for that key
  struct keyNode *next;
};

struct partitionStruct* partitionArray;  //array to hold partitions and keys

//array of linked list of linked list
struct keyNode *combinerStore;

/*  Inserts nodes into the linked list.  Returns 1 on success.  Anything else on fail.
 *
 */
int insert(struct keyNode *head, char *key, char *value) {  //insert node to linked list
  struct keyNode *ptr = head;
  while(strcmp(ptr->key,key) != 0) {  //checks if key matches
    if(ptr->next != NULL) {  //if key doesnt match, if next element exists go to that
      ptr = ptr->next;
    }
    else {  //if key isnt found, make new node with key
      struct keyNode *newKey;
      if((newKey = malloc(sizeof(struct keyNode))) == NULL) {
	return -1;	      
      }
      newKey->key = key;
      newKey->next = NULL;
      //make new node with value for storing the value
      struct valueNode *newValue;
      if((newValue = malloc(sizeof(struct valueNode))) == NULL) {
        return -1;
      }
      newValue->val = value;
      newValue->visited = 0;
      newValue->next = NULL;
      newKey->values = newValue;  //set start of values list to be the new value
      ptr->next = newKey;
      return 1;
    }
  }  //if key matches, execute
  struct valueNode *newNode;
  newNode = malloc(sizeof(struct valueNode));
  int errorCheck = (newNode != NULL);
  if(errorCheck) {
    newNode->val = value;
    newNode->visited = 0;
    newNode->next = NULL;
    if(ptr->values == NULL) {  //checks if values list has started, if not insert newNode
      ptr->values = newNode;
      return 1;
    }
    struct valueNode *valptr = ptr->values;  //pointer to head of values list if it exists
    while(valptr->next != NULL) {  //find tail of values list
      valptr = valptr->next;
    }
    valptr->next = newNode;
  }
  return errorCheck;
}

void MR_EmitToReducer(char *key, char *value) {
    long part = MR_DefaultHashPartition(key,numberParts); //generate hash to determine partition
    for(int i = 0; i < partitionArray[part].curr; i++) {  //searches for key in partition
      if(partitionArray[part].pairs[i].key == key) {  //if key is found
        partitionArray[part].pairs[i].values[partitionArray[part].pairs[i].curr] = value;  //insert value
	partitionArray[part].pairs[i].curr++;
	if(partitionArray[part].pairs[i].curr >= partitionArray[part].pairs[i].max) {  //check if we need to resize array
	  partitionArray[part].pairs[i].max+=10;
	  char **temp = realloc(partitionArray[part].pairs[i].values, sizeof(char**)*partitionArray[part].pairs[i].max);
	  if(temp != NULL) {  //check that realloc didnt fail and set our new sized array
	    partitionArray[part].pairs[i].values = temp;
	    return;  //value is inserted and array resized so we're done
	  }
	  else {
	    //throw error since realloc failed
	  }
	}
      }
    }
    //if key isnt found, insert it and value
    partitionArray[part].pairs[partitionArray[part].curr].key = key;
    if((partitionArray[part].pairs[partitionArray[part].curr].values = malloc(sizeof(char*)*10)) == NULL) {
      //throw error since malloc failed
    }
    partitionArray[part].pairs[partitionArray[part].curr].values[0] = value;  //insert value to first element of new array
    partitionArray[part].pairs[partitionArray[part].curr].curr++;
    partitionArray[part].curr++;
    if(partitionArray[part].curr >= partitionArray[part].max) {  //resizes pairs array if needed
      partitionArray[part].max+=10;
      struct keyValuePair *tmp;
      tmp = realloc(partitionArray[part].pairs,sizeof(struct keyValuePair)*partitionArray[part].max);
      if(tmp != NULL) {
        partitionArray[part].pairs = tmp;
      }
      else {
        //throw error since realloc failed
      }
      //after array resize is done, loop through new part of the array and set curr and max
      for(int k = partitionArray[part].curr; k < partitionArray[part].max; k++) {
        partitionArray[part].pairs[k].curr = 0;
	partitionArray[part].pairs[k].max = 10;
	partitionArray[part].pairs[k].visited = 0;
      }
    }
    //array resized if needed, new key and value pair inserted.  We're finished so return.
    return;   
}

void MR_EmitToCombiner(char *key, char *value) {
    if(key == NULL) {
    }
    if(insert(&combinerStore[whichMapper],key,value) != 1) {  //insert node with data value into the linked list, check for error
      //throw error
    }
}

char* get_key(int mapper_n, int index) {  //gets the key of the specified mapper at the specified index
  struct keyNode *point = &combinerStore[mapper_n];
  for(int i = 0; i < index; i++) {  //go to index of key we want
    point=point->next;
  }
  return point->key;
}

char* get_red_key(int partition) {
  //struct keyValuePair *ptr = partitionArray[partition]->pairs;
  for(int i = 0; i < partitionArray[partition].curr; i++) {  //goes through all key value pairs
    if(partitionArray[partition].pairs[i].keyGot == 0) {
      partitionArray[partition].pairs[i].keyGot = 1;
      return partitionArray[partition].pairs[i].key;
    }
  }
  return NULL;  //no more keys left
}

int get_list_size(int index) {  //gets the size of the linked list at the specified index
  int size = 0;
  struct keyNode *cur = &combinerStore[index];
  while(cur != NULL) {
    size++;
    cur = cur->next;
  }
  return size;
}

unsigned long MR_DefaultHashPartition(char *key, int num_partitions) {
    unsigned long hash = 5381;
    int c;
    while ((c = *key++) != '\0')
        hash = hash * 33 + c;
    return hash % num_partitions;
}

char* get_next_reducer(char *key, int partition_number) {  //used by reducer
  //if partition_number is out of bounds, add a check or give an error
  for(int i = 0; i < partitionArray[partition_number].curr; i++) {
    if(partitionArray[partition_number].pairs[i].key == key) {  //check if key matches
      if(&partitionArray[partition_number].pairs[i].values[partitionArray[partition_number].pairs[i].visited] != NULL) {  //checks if element we are at is null
        partitionArray[partition_number].pairs[i].visited++;  //advance element we're at
        return partitionArray[partition_number].pairs[i].values[partitionArray[partition_number].pairs[i].visited-1];  //return element we checked
      }
      else {  //if no more elements in values
        return NULL;
      }
    } 
  }
  //if no matching key is found
  return NULL;
}

char* get_next_combiner(char *key) {  //used by combiner
   //iterate and return a value associated with that key
   struct keyNode *ptr = &combinerStore[whichMapper];
   while(strcmp(ptr->key,key) != 0) {  //if key doesnt match
     if(ptr->next != NULL) {  //move to next node if it exists
       ptr = ptr->next;
     }
     else {  //if next nodes doesnt exist, key not found
       return NULL;
     }
   }
   //key found
   struct valueNode *valueptr = ptr->values;  //iterator for the values list
   while((valueptr->visited) != 0) {  //checks if we have already visited the node
     if(valueptr->next != NULL) {  //if the next node exists, advance the pointer
       *valueptr = *valueptr->next;
     }
     else {  //if there is no next node, return NULL
       return NULL;
     }
   }
   //found an unvisited node
   if(&valueptr->val == NULL) {  //SHOULD NOT HAPPEN, value in node somehow NULL
     //throw error
   }
   valueptr->visited = 1;  //mark as visited
   return valueptr->val;  //return value at the node 
}

ReduceStateGetter get_state() {
  return NULL;
}

//wrapper function for mappers, takes an array of arguments
void* mapperWrapper(void* array) {

  for(int i = 0; i < argnum; i++) {  //runs all of the mappers
    char* localarray = (char*)array;
    mapfunc(&localarray[i]);
    //move through all keys in data structure
    for(int j = 0; j < get_list_size(whichMapper); j++) {
      combinefunc(get_key(whichMapper,j),get_next_combiner);
    }
  }
  return NULL;
}

//wrapper function for reducers
void* reducerWrapper(void* part) {
  int localPartition = *(int*)part;  //partition number we passed in
  char* nextkey;
  while((nextkey = get_red_key(localPartition)) != NULL) {
    reducefunc(nextkey,NULL,get_next_reducer,localPartition);
  }
  return NULL;
}

void MR_Run(int argc, char *argv[], Mapper map, int num_mappers,
		Reducer reduce, int num_reducers,
		Combiner combine, Partitioner partition) {
  argnum = argc;
  mapnum = num_mappers;
  mapfunc = map;
  combinefunc = combine;
  reducefunc = reduce;
  int threadarray[num_mappers];
  if((combinerStore = malloc(sizeof(struct keyNode)*num_mappers)) == NULL) {
    //throw error
  }
  numberParts = num_reducers;
  if((partitionArray = malloc(sizeof(struct partitionStruct)*num_reducers)) == NULL) {
    //throw error
  }
  for(int i = 0; i < num_reducers; i++) {  //initialize contents of partition struct
    partitionArray[i].partition = i;
    partitionArray[i].curr = 0;
    if((partitionArray[i].pairs = malloc(sizeof(struct keyValuePair*)*10)) == NULL) {  //initially 10 spaces for key value pairs
      //throw error
    }
    for(int j = 0; j < 10; j++) {
      partitionArray[i].pairs[j].curr = 0;
      partitionArray[i].pairs[j].max = 10;
      partitionArray[i].pairs[j].visited = 0; 
    }
    partitionArray[i].max = 10;
  }
  pthread_t* threads;
  if((threads = malloc(sizeof(pthread_t)*num_mappers)) == NULL) {
    //throw error
  }
  pthread_t* redThreads;
  if((redThreads = malloc(sizeof(pthread_t)*num_reducers)) == NULL) {
    //throw error
  }
  char **arguments[num_mappers][(argc/num_mappers)+1];
  int l = 1;
  for(int e = 0; e < (argc/num_mappers)+1; e++) {  //populate array of arguments to be passed to threads
    if(l >= argc) {
      break;
    }
    for(int p = 0; p < num_mappers; p++) {
      if(l >= argc) {
        break;
      }
      if(argv[l] == NULL) {
        printf("Shit something broke\n");
	exit(1);
      }
      arguments[p][e] = &argv[l];
      l++;
    }
  }
  //create num_mappers threads to run the mapper functions
  /*threadarray[0] = pthread_create(&threads[0],NULL,mapperWrapper,argv);
  if(threadarray[0]) {
    //throw error
  }*/
  for(int i = 0; i < num_mappers; i++) {
    whichMapper = i;
    threadarray[i] = pthread_create(&threads[i],NULL,mapperWrapper,arguments[i]);
    if(threadarray[i]) {
      //throw error
    }
  }
  /*threadarray[0] = pthread_join(threads[0],NULL);
  if(threadarray[0]) {
    //throw error
  }*/
  for(int n = 0; n < num_mappers; n++) {
    threadarray[n] = pthread_join(threads[n],NULL);
    if(threadarray[n]) {
      //throw error
    }
  }
  //threadarray[0] = pthread_create(&redThreads[0],NULL,reducerWrapper,0);
  for(int j = 0; j < num_reducers; j++) {
    threadarray[j] = pthread_create(&redThreads[j],NULL,reducerWrapper,(void*)&j);
    if(threadarray[j]) {
      //throw error
    }
  }

  //threadarray[0] = pthread_join(redThreads[0],NULL);
  for(int k = 0; k < num_reducers; k++) {
    threadarray[k] = pthread_join(redThreads[k],NULL);
    if(threadarray[k]) {
      //throw error
    }
  }

  //call map function in each thread
  //map function calls emit to combiner to save data to library data structure
  //--//Temporarily put in array then sort before sending to combiner
  //number of partitions places to save the data in
  //
  //each mapper thread calls combine function
  //create num_reducers reducer threads and call reducer function in each
  //
  //launch reducer threads after joining mapper threads
}
