/*Jordan Merckling, jmerckli, CIS Project 0
This is my own work, with help from office hours and online resources on c functions*/

#include "mentry.h"
#include "mlist.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#define BUCKETS 20;
extern int errno;

typedef struct mlistnode 
{
	struct mlistnode *next_node;
	MEntry *entry;
	int in_val;
} bucket;

struct mlist 
{
	int size;
	bucket **hash;
};

int errno = 0;
int size = BUCKETS;

//MList pointer
MList **mlpoint;


MList *ml_create(void) 
{
	MList *ml;
	int i;

	//temp pointer
	bucket *temphash;

	//MList creation
	if( (ml = (MList *) malloc(sizeof(MList))) == NULL)
	{
		return ml;
	}

	//set initial size of table
	ml->size = size;
	if((ml->hash = (bucket **) calloc(ml->size, sizeof(bucket *))) != NULL)
	{
		for(i = 0; i < size; i++)
		{
			//set buckets with initial value of 0
			ml->hash[i] = (bucket *) malloc(sizeof(bucket));
			ml->hash[i]->next_node = NULL;
			ml->hash[i]->in_val = 0;
		}
	}
	return ml;
}

/*if any hash bucket reaches size, the table size needs to increase
also hash values have to be redetermined because the mod is now different
steps: double table size, extract old data, rehash old values one at a time,
fill new hash table
*/
void *resize(MList *ml)
{
	if(errno)
		fprintf(stderr, "Update: table resizing has occured\n");
	
	int i,j;
	int cur_bucket;
	
	//double the size of the hash table
	MList *new_ml;
	size = (ml->size) * 2;
	new_ml = ml_create();

	//transfer old bucket values
	bucket *data;
	bucket *new_data;
	bucket *add_data;

	//get new hash values for updated table size
	unsigned long hashvalue;

	for(i = 0; i < ml->size; i++)
	{
		data = ml->hash[i];
		while(data->next_node != NULL)
		{
			new_data = data->next_node;
			
			//set the last value to NULL
			data->next_node = NULL;

			hashvalue = me_hash(data->entry,size);
			add_data = new_ml->hash[hashvalue];

			//put data in the "new" hash table, in the correct buckets
			cur_bucket = 0;
			while(add_data->next_node != NULL)
			{
				add_data = add_data->next_node;
			}
			
			if((new_ml->hash[hashvalue]->in_val == 0))
			{
				new_ml->hash[hashvalue] = data;
				new_ml->hash[hashvalue]->in_val = 1;
			} 
		
			else if( (new_ml->hash[hashvalue]->in_val == 1) )
			{
				new_ml->hash[hashvalue]->next_node = data;
				new_ml->hash[hashvalue]->in_val = 2;
			} 
			else
				add_data->next_node = data;

			//update the data
			data = new_data;
		}	
	}
	//free the old list
	free(ml->hash);
	free(ml);
	
	//return the new list
	return new_ml;
}

//ml_add adds a ne MEntry to the list, return 1 if successful, 0 if fail.
//check for duplicates, duplicates return 1
int ml_add(MList **ml, MEntry *me) 
{
	//bucket_size keeps track of all the buckets in the hash table
	int bucket_size = 0;

	MList *m = *ml;
	unsigned hashvalue = 0;
	int i;
	bucket *cur,*new_bucket;

	//check for duplicate values
	if (ml_lookup(m, me) != NULL)
	{
		return 1;
	}

	//make sure there is room for the next node
	if((new_bucket = (bucket *) malloc(sizeof(bucket))) == NULL)
		{
			return 0;
		}

	new_bucket->next_node = NULL;

	//use me_hash to get the hash value
	hashvalue = me_hash(me, m->size);

	// put current into the proper bucket
	cur = m->hash[hashvalue];

	//loop through to get to spot in bucket
	while(cur->next_node != NULL)
	{
		cur = cur->next_node;
		bucket_size++;
	}

	// set current to next node, and place it in mentry
	cur->next_node = new_bucket;
	cur->entry = me;
	
	//check if any buckets exceed 20 values
	if(bucket_size > m->size)
		{
			*ml = resize(m);
		}
	return 1;	
}

//ml_lookup looks for MEntry in the list, returns matching entry or NULL
MEntry *ml_lookup(MList *ml, MEntry *me) 
{
	unsigned hashvalue = 0;
	bucket *buck_data;

	// get hash value fo me
	hashvalue = me_hash(me, ml->size);

	/*check for the same hash value
	if match is found return a pointer
	return NULL if not found */
	buck_data = ml->hash[hashvalue];

	while(buck_data->next_node!=NULL)
	{
		if(me_compare(buck_data->entry,me)==0)
		{
			return buck_data->entry;
		} 
		else 
		{
			buck_data = buck_data->next_node;
		}
	}
	return NULL;
}

//destroy the mailing list and free allocated data
void ml_destroy(MList *ml) 
{
	int i;
	//pointers for node to remove and the node after
	bucket *remove;
	bucket *next_entry;

	//free all buckets one at a time
	for(i = 0; i < size; i++)
	{
		remove = ml->hash[i];
		while(remove->next_node != NULL)
		{
			next_entry = remove->next_node;
			me_destroy(remove->entry);
			free(remove);
			remove = next_entry;
		}
		free(remove);
	}
	free(ml->hash);
	free(ml);
}