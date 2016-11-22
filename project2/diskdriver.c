#include "diskdriver.h"
#include "sectordescriptorcreator.h"
#include "freesectordescriptorstore_full.h"
#include "BoundedBuffer.h"
#include "sectordescriptor.h"
#include "block.h"
#include "diagnostics.h"
#include "diskdevice_full.h"
#include "diskdevice.h"
#include "fakeapplications.h"
#include "freesectordescriptorstore.h"
#include "generic_queue.h"
#include "sectordescriptor.h"
#include "sectordescriptorcreator.h"
#include "voucher.h"
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

#define SIZE 20

static Voucher writev;
static Voucher readv;


FreeSectorDescriptorStore *fsds;
DiskDevice *disk;
BoundedBuffer *writeBB;
BoundedBuffer *readBB;
//possibly put in struct?
pthread_mutex_t mute;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;


//Structure for voucher
typedef struct voucher 
{
	int status; // 1 if done
	SectorDescriptor* sd;
	pthread_mutex_t mute;
	pthread_cond_t cond;
	int read_write; // 1 if read else 0;
} Voucher;


//defining the write function.
//main usage is to write onto the sector.

void *write() 
{
	while (1) 
	{
		Voucher *writev = (Voucher *) blockingReadBB(writeBB);
		pthread_mutex_lock(&(writev->mute));
		int status = write_sector(disk, (writev)->sd);

		if(status == 0)
		{
			pthread_cond_wait(&(writev->cond), &writev->mute);
            pthread_mutex_unlock(&(writev->mute));
		}

		writev->status = status;
		pthread_cond_broadcast(&(writev->cond));
		pthread_mutex_unlock(&(writev->mute));
		//blocking_put_sd(fsds, writev->sd);
	}
}


// defining the write function
// main usage is to read for a certain sector
void *read() 
{
	while (1) 
	{		
		Voucher *readv = (Voucher *) blockingReadBB(writeBB);
		pthread_mutex_lock(&(readv->mute));
		int status = write_sector(disk, (readv)->sd);

		if(status == 0)
		{
			pthread_cond_wait(&(readv->cond), &readv->mute);
            pthread_mutex_unlock(&(readv->mute));
		}

		readv->status = status;
		pthread_cond_broadcast(&(readv->cond));
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
void init_disk_driver(DiskDevice *dd, void *mem_start, unsigned long mem_length, FreeSectorDescriptorStore **fsds_ptr) 
{
	disk = dd;	
	*fsds_ptr = create_fsds();
	create_free_sector_descriptors(*fsds_ptr, mem_start, mem_length);
	fsds = *fsds_ptr;
    
	writeBB = createBB(SIZE);
	readBB = createBB(SIZE);

    pthread_t workThread;
	if (pthread_create(&workThread, NULL, write, NULL)) 
	{
		printf("Error: failed to create writing thread\n");
		exit(1);
	} else 
	{
        printf ("Write Thread Created");
    }
   
    pthread_t readThread;
	if (pthread_create(&readThread, NULL, read, NULL)) 
	{
		printf("Error: failed to create reading thread\n");
		exit(1);
	}else 
	{
        printf ("Read Thread Created");
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
	writev.read_write = 0;
	writev.status = 0;
	writev.sd = sd;
	pthread_mutex_init(&(writev.mute), NULL);
    pthread_cond_init(&(writev.cond), NULL);

	*v = &writev;
	blockingWriteBB(writeBB, *v);
}

int nonblocking_write_sector(SectorDescriptor *sd, Voucher **v) 
{
	writev.read_write = 0;
	writev.status = 0;
	writev.sd = sd;
	pthread_mutex_init(&(writev.mute), NULL);
    pthread_cond_init(&(writev.cond), NULL);

	*v = &writev;
	return nonblockingWriteBB(writeBB, *v);
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
	readv.read_write = 1;
	readv.status = 0;
	readv.sd = sd;
	pthread_mutex_init(&(readv.mute), NULL);
    pthread_cond_init(&(readv.cond), NULL);

	*v = &readv;
	blockingWriteBB(readBB, *v);
}

int nonblocking_read_sector(SectorDescriptor *sd, Voucher **v) 
{
	readv.read_write = 1;
	readv.status = 0;
	readv.sd = sd;
	pthread_mutex_init(&(readv.mute), NULL);
    pthread_cond_init(&(readv.cond), NULL);

	*v = &readv;
	return nonblockingWriteBB(readBB, *v);
}

/*
 * the following call is used to retrieve the status of the read or write
 * the return value is 1 if successful, 0 if not
 * the calling application is blocked until the read/write has completed
 * if a successful read, the associated SectorDescriptor is returned in sd
 */
/*int redeem_voucher(Voucher *v, SectorDescriptor **sd) 
{
    if (v == NULL)
	{
        printf("DRIVER: null voucher redeemed!\n");
        return 0;
    }
    else
	{
        pthread_mutex_lock(&(v->mute));
        if (v->read_write == 1)
		{
            *sd = v->sd;
        }
        else if (v->sd != NULL)
		{
            blocking_put_sd(fsds, v->sd);
        }
        pthread_cond_destroy(&(v->cond));
        pthread_mutex_unlock(&(v->mute));
        pthread_mutex_destroy(&(v->mute));
        if (v->status == 1)
		{
            return 1;
        }
        else
		{
            return 0;
        }
	}
}
*/
int redeem_voucher(Voucher *v, SectorDescriptor **sd) {

	Vouchers *vou = (Vouchers *) v;
	int status;

	pthread_mutex_lock(&(v->mute)); 
	while (v->status == 0) {
		pthread_cond_wait(&(v->cond), &v->mutex);
	}
	*sd = v->sd;
	status = v->status;
	//free(vou);
    //free(v);
	pthread_mutex_unlock(&(v->mutex));
    pthread_mutex_destroy(&(v->mutex));
    pthread_cond_destroy(&(v->cond));
	
	return status;
}
