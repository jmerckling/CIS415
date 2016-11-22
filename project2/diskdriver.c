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

//static Voucher writev;
//static Voucher readv;


//Structure for voucher
typedef struct voucher 
{
	int status; // 1 if done
	SectorDescriptor* sd;
	pthread_mutex_t mute;
	pthread_cond_t cond;
	int read_write; // 0 if read 1 for write;
	int completed;
} Voucher;

FreeSectorDescriptorStore *fsds_in;
DiskDevice *disk;
pthread_t tRead;
pthread_t tWrite;
BoundedBuffer *writeBB;
BoundedBuffer *readBB;
BoundedBuffer *vouchBB;
Voucher vouchers[VSIZE];

//Function to write onto a sector
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
	}
}

// defining the write function
// main usage is to read for a certain sector
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
 * called before any other methods to allow you to initialize data
 * structures and to start any internal threads.
 *
 * Arguments:
 *   dd: the DiskDevice that you must drive
 *   mem_start, mem_length: some memory for SectorDescriptors
 *   fsds_ptr: you hand back a FreeSectorDescriptorStore constructed
 *             from the memory provided in the two previous arguments
 
 */
void init_disk_driver(DiskDevice *dd, void *mem_start, unsigned long mem_length, FreeSectorDescriptorStore **fsds) 
{
	disk = dd;
	fsds_in = create_fsds();
	*fsds = fsds_in;
	vouchBB = createBB(VSIZE * sizeof(Voucher *)); 
	create_free_sector_descriptors(*fsds, mem_start, mem_length);

	int i;
	for(i = 0; i < VSIZE; i++)
	{
		pthread_mutex_init(&(vouchers[i].mute), NULL);
		pthread_cond_init(&(vouchers[i].cond), NULL);
		vouchers[i].completed = 0;
		blockingWriteBB(vouchBB, &vouchers[i]);
	}
	readBB = createBB(SIZE);
	writeBB = createBB(SIZE);

	if (pthread_create(&tWrite, NULL, write, NULL)) 
	{
		printf("Error: failed to create writing thread\n");
//		exit(1);
	}
   
	if (pthread_create(&tRead, NULL, read, NULL)) 
	{
		printf("Error: failed to create reading thread\n");
//		exit(1);
	}
}

/*
 * the following calls are used to write a sector to the disk
 * the nonblocking call must return promptly, returning 1 if successful at
 * queueing up the write, 0 if not (in case internal buffers are full)
 * the blocking call will usually return promptly, but there may be
 * a delay while it waits for space in your buffers.
 * neither call should delay until the sector is actually written to the disk
 * for a successful nonblocking call and for the blocking call, a voucher is
 * returned that is required to determine the success/failure of the write
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
	if(test==1)
	{
		*v = writev;
		return 1;
	}
	nonblockingWriteBB(vouchBB, writev);
	return 0;
}

/*
 * the following calls are used to initiate the read of a sector from the disk
 * the nonblocking call must return promptly, returning 1 if successful at
 * queueing up the read, 0 if not (in case internal buffers are full)
 * the blocking callwill usually return promptly, but there may be
 * a delay while it waits for space in your buffers.
 * neither call should delay until the sector is actually read from the disk
 * for successful nonblocking call and for the blocking call, a voucher is
 * returned that is required to collect the sector after the read completes.
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
 * the following call is used to retrieve the status of the read or write
 * the return value is 1 if successful, 0 if not
 * the calling application is blocked until the read/write has completed
 * if a successful read, the associated SectorDescriptor is returned in sd
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
