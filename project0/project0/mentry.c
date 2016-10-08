/*Jordan Merckling, jmerckli, CIS 415 Project 0
This is my own work, with help from office hours and online resources*/

#include "mentry.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

//initial sizes, need to figure out how to do without these (if time allows)
#define LINES 1000
#define NAMESIZE 500
#define POSTALCODE 20

// pointer to the structure
MEntry *me;

//reads the entries from a mailing list an populates the struct
MEntry *me_get(FILE *fd)
{
	int i,j;
	char nameLine[LINES] = {}, addressLine[LINES] = {}, stateZip[LINES] = {}; 
	
	if((me = (MEntry *)malloc(sizeof(MEntry))) != NULL)
	{
		me->surname = malloc(sizeof(char)*NAMESIZE);
		me->zipcode = malloc(sizeof(char)*POSTALCODE);
		me->full_address = malloc(sizeof(char)*LINES);
		
		//grab values from file line by line
		fgets(nameLine, LINES, fd);
		fgets(addressLine, LINES, fd);
		fgets(stateZip, LINES, fd);
		
		//return null when there is nothing left to read from the file
		if(*nameLine == '\0')
		{
			return NULL;
		}
		
		//need to make surname all lower case
		for (i = 0; *(nameLine + i) != ','; i++)
		{
			me->surname[i] = (char)tolower(nameLine[i]);
		}
		//set to null so later it only reads up to it
		me->surname[i] = '\0';
		
		//puts strings into full_address
		strcpy(me->full_address, nameLine);
		strcat(me->full_address, addressLine);
		strcat(me->full_address, stateZip);
		
		me->house_number = (int) strtol(addressLine, (char **)NULL, 10);
		/*if(me->house_number < 0)
		{
			me->house_number = 0;
		}
		*/
		
		for (i = 0, j = 0; stateZip[i] != '\n'; i++)
		{
			if(isalnum(stateZip[i]))
			{
				me->zipcode[j] = stateZip[i];
				j++;
			}
		}
		me->zipcode[j] = '\0';
	}
	return me;
}

//helper function for me_hash to give hash values to individule strings
unsigned long help_hash(char *str)
{
	int prime = 31;
	unsigned hashvalue;
	
	for(hashvalue = 0; *str != '\0'; str++)
	{
		hashvalue = *str + prime * hashvalue;
	}
	return hashvalue;
}

//me_hash computes a hash of the MEntry, mod size
unsigned long me_hash(MEntry *me, unsigned long size)
{
	unsigned hashvalue = 0;
	
	hashvalue = help_hash(me->surname);
	hashvalue += help_hash(me->zipcode);
	hashvalue += me->house_number;
	
	return hashvalue % size;
}

//me_print prints the full_address on fd
void me_print(MEntry *me, FILE *fd)
{
	fprintf(fd, "%s", me->full_address);
}

//me_compare compares two mail entries, returning <0, 0, >0
//if me1 < me2, me1 == me2, me1 > me2
int me_compare(MEntry *me1, MEntry *me2)
{
	//compare all three strings
	if((strcmp(me1->surname, me2->surname) == 0) && (strcmp(me1->zipcode, me2->zipcode) == 0) && (me1->house_number == me2->house_number))
	{
		return 0;
	}
	else
	{
		return -1;
	}
}

//me_destroy frees allocated memory
void me_destroy(MEntry *me)
{
	free(me->surname);
	free(me->zipcode);
	free(me->full_address);
	/*free(nameLine);
	free(addressLine);
	free(stateZip);*/
	free(me);
}

