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

#define SIZE 10

FreeSectorDescriptorStore *fsds;
DiskDevice *disk;
BoundedBuffer *incoming;
BoundedBuffer *read_buf;
pthread_mutex_t mute;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;


//defining the structure of the voucher
typedef struct Voucher {
	int status;
	SectorDescriptor* sd;
} Vouchers;


//defining the write function.
//main usage is to write onto the sector.

void *write() {

	Vouchers *vou;
	int status;

	while (1) {
		vou = (Vouchers *) blockingReadBB(incoming);				
		status = write_sector(disk, (vou)->sd);
		//fprintf(stderr,"Status: %d\n",status);
		pthread_mutex_lock(&mute);
		(vou)->status = status;
		pthread_cond_broadcast(&cond);
		pthread_mutex_unlock(&mute);
		blocking_put_sd(fsds, (vou)->sd);
	}
}


// defining the write function
// main usage is to read for a certain sector
void *read() {

	Vouchers *vou;
	int status;

	while (1) {
		vou = (Vouchers *) blockingReadBB(read_buf);
		status = read_sector(disk, vou->sd);

		pthread_mutex_lock(&mute); 
		vou->status = status;
		pthread_cond_broadcast(&cond);
		pthread_mutex_unlock(&mute);
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
void init_disk_driver(DiskDevice *dd, void *mem_start, unsigned long mem_length, FreeSectorDescriptorStore **fsds_ptr) {

	disk = dd;	
	*fsds_ptr = create_fsds();
	create_free_sector_descriptors(*fsds_ptr, mem_start, mem_length);
	fsds = *fsds_ptr;
    
	incoming = createBB(SIZE);
	read_buf = createBB(SIZE);

    pthread_t workThread;
	if (pthread_create(&workThread, NULL, write, NULL)) {
		printf("Error: failed to create writing thread\n");
		exit(1);
	} else {
        printf ("Write Thread Created");
    }

    
    pthread_t readThread;
	if (pthread_create(&readThread, NULL, read, NULL)) {
		printf("Error: failed to create reading thread\n");
		exit(1);
	}else {
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
void blocking_write_sector(SectorDescriptor *sd, Voucher **v) {

	Vouchers *vou = (Vouchers *)malloc(sizeof(Vouchers));
	vou->status = -1;
	vou->sd = sd;

	*v = (Voucher *) vou;

	blockingWriteBB(incoming, vou);

}

int nonblocking_write_sector(SectorDescriptor *sd, Voucher **v) {

	Vouchers *vou = (Vouchers *)malloc(sizeof(Vouchers));
	vou->status = -1;
	vou->sd = sd;

	*v = (Voucher *) vou;

	return nonblockingWriteBB(incoming, vou);

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
void blocking_read_sector(SectorDescriptor *sd, Voucher **v) {

	Vouchers *vou = (Vouchers *)malloc(sizeof(Vouchers));
	vou->status = -1;
	vou->sd = sd;

	*v = (Voucher *) vou;

	blockingWriteBB(read_buf, vou);

}
int nonblocking_read_sector(SectorDescriptor *sd, Voucher **v) {

	Vouchers *vou = (Vouchers *)malloc(sizeof(Vouchers));
	vou->status = -1;
	vou->sd = sd;

	*v = (Voucher *) vou;

	return nonblockingWriteBB(read_buf, vou);

}

/*
 * the following call is used to retrieve the status of the read or write
 * the return value is 1 if successful, 0 if not
 * the calling application is blocked until the read/write has completed
 * if a successful read, the associated SectorDescriptor is returned in sd
 */
int redeem_voucher(Voucher *v, SectorDescriptor **sd) {

	Vouchers *vou = (Vouchers *) v;
	int status;

	pthread_mutex_lock(&mute); 
	while (vou->status == -1) {
		pthread_cond_wait(&cond, &mute);
	}
	*sd = vou->sd;
	status = vou->status;
	free(vou);
    //free(v);
	pthread_mutex_unlock(&mute);
    pthread_mutex_destroy(&mute);
    pthread_cond_destroy(&cond);
	
	return status;
}
