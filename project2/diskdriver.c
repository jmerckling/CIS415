//Jordan Merckling, jmerckli
//CIS 415 Project 2
//This is my own work except I designed psuedo code with JD M
//I also utilized the bounded buffer functions that you provided

#include "diskdriver.h"
#include "BoundedBuffer.h"
#include "sectordescriptorcreator.h"
#include "freesectordescriptorstore_full.h"
#include "sectordescriptor.h"

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

#define SIZE 100
#define VSIZE 30

//Structure for voucher
typedef struct voucher 
{
	int status; // 1 if done
	SectorDescriptor* sd;
	pthread_mutex_t mute;
	pthread_cond_t cond;
	int read_write; // 0 if read 1 for write;
	int completed; // says if the code is done, 1 if done
} Voucher;

//Global Variables
FreeSectorDescriptorStore *fsds_in;
DiskDevice *disk;
pthread_t tRead;
pthread_t tWrite;
BoundedBuffer *writeBB;
BoundedBuffer *readBB;
BoundedBuffer *vouchBB;
Voucher vouchers[VSIZE];

/*
	Function to write onto a sector
	makes a voucher for writing
	locks 
	sets status, sets complete to 1
	signals and unlocks
  */
void *write() 
{
	while (1) 
	{
		Voucher *writev = blockingReadBB(writeBB);
		pthread_mutex_lock(&(writev->mute));
		writev->status = write_sector(disk, (writev->sd));
		writev->completed = 1;
		pthread_cond_signal(&(writev->cond));
		pthread_mutex_unlock(&(writev->mute));
		//blocking_put_sd(fsds_in, writev->sd);
	}
}
/*
	Function to read a sector
	makes a voucher for reading
	locks 
	sets status, sets complete to 1
	signals and unlocks
  */
void *read() 
{
	while (1) 
	{		
		Voucher *readv = blockingReadBB(readBB);
		pthread_mutex_lock(&(readv->mute));
		readv->status = read_sector(disk, (readv->sd));
		readv->completed = 1;
		pthread_cond_signal(&(readv->cond));
		pthread_mutex_unlock(&(readv->mute));

	}
}

/*
  Initializes the data, called first.
 
  Arguments:
    dd: the DiskDevice that you must drive
    mem_start, mem_length: some memory for SectorDescriptors
    fsds_ptr: you hand back a FreeSectorDescriptorStore constructed
              from the memory provided in the two previous arguments
 */
void init_disk_driver(DiskDevice *dd, void *mem_start, unsigned long mem_length, FreeSectorDescriptorStore **fsds) 
{
	disk = dd;
	fsds_in = create_fsds();
	*fsds = fsds_in;
	create_free_sector_descriptors(*fsds, mem_start, mem_length);
	vouchBB = createBB(VSIZE * sizeof(Voucher *)); 

	int i;
	for(i = 0; i < VSIZE; i++)
	{
		pthread_mutex_init(&(vouchers[i].mute), NULL);
		pthread_cond_init(&(vouchers[i].cond), NULL);
		vouchers[i].completed = 0; //not done
		blockingWriteBB(vouchBB, &vouchers[i]);
	}
	readBB = createBB(SIZE);
	writeBB = createBB(SIZE);

	if (pthread_create(&tWrite, NULL, write, NULL)) 
	{
		fprintf(stderr, "Error: failed to create writing thread\n");
		exit(1);
	}
   
	if (pthread_create(&tRead, NULL, read, NULL)) 
	{
		fprintf(stderr, "Error: failed to create reading thread\n");
		exit(1);
	}
}
/*
	Functions to write a sector to disk
	nonblocking returns a 1 if it successfully queues up the write,
	0 if it is not successfull.
	The blocking write returns once there is space in the buffer
	both calls when successful retun a voucher to see if the write 
	was successful or not.
*/
void blocking_write_sector(SectorDescriptor *sd, Voucher **v) 
{
	Voucher *writev = blockingReadBB(vouchBB);
	writev->read_write = 1;
	writev->completed = 0;
	writev->sd = sd;
	*v = writev;
	blockingWriteBB(writeBB, *v);
}

int nonblocking_write_sector(SectorDescriptor *sd, Voucher **v) 
{
	Voucher *writev = blockingReadBB(vouchBB);
	writev->read_write = 1;
	writev->completed = 0;
	writev->sd = sd;
	//*v = writev;
	int test = nonblockingWriteBB(writeBB, writev);
	if(test == 1)
	{
		*v = writev;
		return 1;
	}
	nonblockingWriteBB(vouchBB, writev);
	return 0;
}
/*
	Functions to read a sector to disk
	nonblocking returns a 1 if it successfully queues up the read,
	0 if it is not successfull.
	The blocking read returns once there is space in the buffer
	both calls when successful retun a voucher to see if the read 
	was successful or not.
*/
void blocking_read_sector(SectorDescriptor *sd, Voucher **v) 
{
	Voucher *readv = blockingReadBB(vouchBB);
	readv->read_write = 0;
	readv->completed = 0;
	readv->sd = sd;
	*v = readv;

	blockingWriteBB(readBB, *v);
}

int nonblocking_read_sector(SectorDescriptor *sd, Voucher **v) 
{
	Voucher *readv = blockingReadBB(vouchBB);
	readv->read_write = 0;
	readv->completed = 0;
	readv->sd = sd;
//	*v = readv;
	int test = nonblockingWriteBB(readBB, readv);
	if(test == 1)
	{
		*v = readv;
		return 1;
	}
	nonblockingWriteBB(vouchBB, readv);
	return 0;
}

/*
	Function that retrieves the status of the read of write
	returns a 1 for successful, 0 otherwise
	the app that is calling is blocked until the current read/write
	has completed
	if a read is successful the SectorDescriptor is retuned in sd
*/
int redeem_voucher(Voucher *v, SectorDescriptor**sd){

	if(v == NULL)
	{
		fprintf(stderr, "null voucher has been redeemed!\n");
		return 0;
	}
	else
	{
		pthread_mutex_lock(&(v->mute));
	
		while(v->completed != 1)
		{
			pthread_cond_wait(&v->cond,&v->mute);
		}
		if(v->status == 1)
		{
			if(v->read_write == 1)
			{
				*sd = v->sd;
			}
		}	

		if((v->read_write) ==0)
		{
			blocking_put_sd(fsds_in, v->sd);
		}

		v->completed = 0;
		blockingWriteBB(vouchBB, v);
		pthread_mutex_unlock(&(v->mute));
		return v->status;
	}
}
