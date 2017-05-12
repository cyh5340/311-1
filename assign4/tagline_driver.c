///////////////////////////////////////////////////////////////////////////////
//
//  File           : tagline_driver.c
//  Description    : This is the implementation of the driver interface
//                   between the OS and the low-level hardware.
//
//  Author         : cyh5340
//  Created        : true

// Include Files
#include <stdlib.h>
#include <string.h>
#include <cmpsc311_log.h>

// Project Includes
#include "raid_bus.h"
#include "tagline_driver.h"
#include "raid_cache.c"
#include "raid_cache.h"
//
// Functions
struct tagdisk{
	uint8_t disk1; //for use
	uint8_t disk2; //for backup
	uint32_t blockid1; //the blockid for disk1
	uint32_t blockid2; //the blockid for disk2
	int tag;
	int bnum; // the block number for the tagline
};//this array will be access for the struct
struct tagdisk *array;

//This fuction do the shifting part that comnines everything altogether into a 64-bit
RAIDOpCode create_raid_request(uint64_t type,uint64_t blocks,uint64_t disks,uint64_t status,uint64_t blockid){
	return(type<<56|blocks<<48|disks<<40|status<<32|blockid);
}

//This function help extract the status
RAIDOpCode extract_raid_response(RAIDOpCode resp){
		resp=resp<<57;
		resp=resp>>63;
	return (uint8_t) resp;
}

///////////////////////////////////////////////////////////////////////////
// Function     : tagline_driver_init
// Description  : Initialize the driver with a number of maximum lines to process
//
// Inputs       : maxlines - the maximum number of tag lines in the system
// Outputs      : 0 if successful, -1 if failure

int tagline_driver_init(uint32_t maxlines) {
	//first, give space to the managing struct
	array= calloc(MAX_TAGLINE_BLOCK_NUMBER*maxlines, sizeof(struct tagdisk));
	//This part start the whole thing
	RAIDOpCode request = create_raid_request(RAID_INIT, (RAID_DISKBLOCKS/RAID_TRACK_BLOCKS), RAID_DISKS, 0, 0); 
	RAIDOpCode resp = client_raid_bus_request(request, NULL);
	int result = extract_raid_response(resp);

	if (result == 1){ //operation failed
		return (-1);
	}
	//This part format the whole thing (putting 0s)
	int i=0;
	for (i=0; i<RAID_DISKS; i++){
	RAIDOpCode request2 = create_raid_request(RAID_FORMAT, 0, i, 0, 0);
	RAIDOpCode resp2 = client_raid_bus_request(request2, NULL);
	
	int result2 = extract_raid_response(resp2);
		if (result2 == 1){ //operation failed
			return -1;
		}	
	}
	// also initialize the cache
	init_raid_cache(TAGLINE_CACHE_SIZE);

	// Return successfully
	logMessage(LOG_INFO_LEVEL, "TAGLINE: initialized storage (maxline=%u)", maxlines);
	return(0);
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : tagline_read
// Description  : Read a number of blocks from the tagline driver
//
// Inputs       : tag - the number of the tagline to read from
//                bnum - the starting block to read from
//                blks - the number of blocks to read
//                bug - memory block to read the blocks into
// Outputs      : 0 if successful, -1 if failure

int tagline_read(TagLineNumber tag, TagLineBlockNumber bnum, uint8_t blks, char *buf) {
	int j=0, a=0;
	//a is the index for tdd
	//j is the index for array
	
	struct tagdisk *tdd= calloc(blks, (sizeof(struct tagdisk)));
	//locate the thing in the struct	
	int b=0;
		//find all the blocks that need to be read
	for (b=0; b<=blks; b++){
		bnum+=b;//find them one by one block
		//if disk1=disk2, then this index has not been written yet
		for (j=0; array[j].disk1!=array[j].disk2; j++){
			if (array[j].tag==tag&&array[j].bnum==bnum){
				tdd[a]=array[j];
				a++;
			}
		}
		
	}	
		
	//now, execute READ if there's something in the context. If there is context, it will be store at 2 seperate disks; therefore, disk1 != disk2
//	if (tdd[0].disk1 != tdd[0].disk2){
//		uint64_t disk= (int) tag%8*2;
//		uint64_t blockid;
//		if (tag<8){
//		blockid= bnum;
//		}
//		else{
//		blockid= ((int)tag/8)*MAX_TAGLINE_BLOCK_NUMBER+bnum;
//		}
	int c=0;
	for (c<blks; array[c].disk1!=array[c].disk2; c++){
	//before accessing the disk, access the cache
		if (get_raid_cache(array[c].disk1, array[c].blockid1)!=NULL){
		//get the buf	
			*buf=get_raid_cache(array[c].disk1, array[c].blockid1);
		}
		else{		
			raid_disk_signal();//check if any disk fail
			RAIDOpCode request = create_raid_request(RAID_READ, 1, tdd[a].disk1, 0, tdd[a].blockid1);
			RAIDOpCode resp = client_raid_bus_request(request, buf[c]);
			put_raid_cache(tdd[a].disk1, tdd[a].blockid1, buf);
			int result = extract_raid_response(resp);
			if (result == 1){ //operation failed
				return (-1);
				}
		}
	}
//	else{//if you're trying to read from out of nowhere
//		return (-1);
//	}
	
	// Return successfully
	logMessage(LOG_INFO_LEVEL, "TAGLINE : read %u blocks from tagline %u, starting block %u.",
			blks, tag, bnum);
	return(0);
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : tagline_write
// Description  : Write a number of blocks from the tagline driver
//
// Inputs       : tag - the number of the tagline to write from
//                bnum - the starting block to write from
//                blks - the number of blocks to write
//                bug - the place to write the blocks into
// Outputs      : 0 if successful, -1 if failure

int tagline_write(TagLineNumber tag, TagLineBlockNumber bnum, uint8_t blks, char *buf) {
	int a=0, b=0, c=0, n=0; 
	//a is the index for tdd
	//b is the index for overwriting
	//c is for clounting the blks
	//n is the size of array
	struct tagdisk *tdd= calloc(blks, sizeof(struct tagdisk));
	//first, check if this block has already been written
	//it will be check for blks times in order to cover all changes
	for (c<blks; bnum+=c; c++){
		int i=0;
		for (i=0; array[i].disk1!=array[i].disk2; i++){
			if (array[i].tag==tag){
				if (array[i].bnum== bnum){
					tdd[a]=array[i];
				}
			}
		}
	}
	//overwriting the already written blocks
	for (b=0; b<a; b++){
		raid_disk_signal();//check if any disk fail
		RAIDOpCode request = create_raid_request(RAID_WRITE, 1, tdd[a].disk1, 0, tdd[a].blockid1);
		RAIDOpCode resp = client_raid_bus_request(request, buf[b]);
		int result = extract_raid_response(resp);
		if (result == 1){ //operation failed
			return (-1);}
	}
	//after overwriting, write new stuff, but first, find out which index of array to write to
	
	for(n=0; array[n].disk1!=array[n].disk2; n++){
	
	}
	n++;//for writing the new stuff
	for (b<blks; b++; bnum+=b){
		array[n].disk1=tdd[b].disk1+1;// always change into a new disk for writing
		if (array[n].disk1>=RAID_DISKS){
			array[n].disk1==0;
		}
		array[n].disk2=array[n].disk1+1;//this is for the backup information
		array[n].tag=tag;
		array[n].bnum=bnum;
		int c=0, temp=0;//this will find which block in the disk to write to, since we dont store the total written blocks in each disk
		for (c=0; c<n; c++){
			if (array[c].disk1==array[n].disk1){
				if (temp< array[c].blockid1){
					temp=array[c].blockid1;//temp= the last block in disk that has been accessed
				}
			}
		}
		array[n].blockid1=temp+1;
		//so a new blockid will now be accessed
		raid_disk_signal();//check if any disk fail
		RAIDOpCode request2 = create_raid_request(RAID_WRITE, 1, array[n].disk1, 0, array[n].blockid1);
		RAIDOpCode resp2 = client_raid_bus_request(request2, buf[b]);
		int result2 = extract_raid_response(resp2);
		if (result2 == 1){ //operation failed
			return (-1);
		}

		//do it again for the backup
		temp=0;
		for (c=0; c<n; c++){
			if (array[c].disk1==array[n].disk1){
				if (temp< array[c].blockid2){
					temp=array[c].blockid2;//temp= the last block in disk that has been accessed
				}
			}
		}
		array[n].blockid2=temp+1;
		//so a new blockid will now be accessed
		raid_disk_signal();//check if any disk fail
		RAIDOpCode request3 = create_raid_request(RAID_WRITE, 1, array[n].disk2, 0, array[n].blockid2);
		RAIDOpCode resp3 = client_raid_bus_request(request3, buf[b]);
		int result3 = extract_raid_response(resp3);
		if (result3 == 1){ //operation failed
			return (-1);
		}

		//call the function
//		int c=tag;
//		uint64_t disk1= (c%8)*2;
//		uint64_t disk2= (c%8)*2+1;
//		uint64_t blockid;
//		if (tag<8){
//		blockid= bnum;
//		}
//		else{
//		blockid= ((int)tag/8)*MAX_TAGLINE_BLOCK_NUMBER+bnum;
//		}
//		RAIDOpCode request = create_raid_request(RAID_WRITE, blks, disk1, 0, blockid);
//		RAIDOpCode resp = client_raid_bus_request(request, buf);
//		int result = extract_raid_response(resp);
//		if (result == 1){ //operation failed
//			return (-1);}
	}

	// Return successfully
	logMessage(LOG_INFO_LEVEL, "TAGLINE : wrote %u blocks to tagline %u, starting block %u.",
			blks, tag, bnum);
	return(0);
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : tagline_close
// Description  : Close the tagline interface
//
// Inputs       : none
// Outputs      : 0 if successful, -1 if failure

int tagline_close(void) {

	RAIDOpCode request = create_raid_request(RAID_CLOSE, 0, 0, 0, 0);
	RAIDOpCode resp = client_raid_bus_request(request, NULL);
	int result = extract_raid_response(resp);
	//also end the cache
	close_raid_cache();
	if (result == 1){ //operation failed
	return(-1);
	}
	// Return successfully
	logMessage(LOG_INFO_LEVEL, "TAGLINE storage device: closing completed.");
	return(0);
}

int raid_disk_signal(void){

	int i=0, disk=NULL, a=0;
	for (i=0; i<RAID_DISKS; i++){
		RAIDOpCode request = create_raid_request(RAID_STATUS, 0, 0, i, 0);		     RAIDOpCode resp = client_raid_bus_request(request, NULL);
		int result = extract_raid_response(resp);
	
		if (result == 1){ //operation failed
		disk=i;
		}		
	}

	//if something did fail, recover.
	if(disk!=NULL){
	//first, reformat everything in that disk to 0
		RAIDOpCode request2 = create_raid_request(RAID_FORMAT, 0, disk, 0, 0);
		RAIDOpCode resp2 = client_raid_bus_request(request2, NULL);

		int result2 = extract_raid_response(resp2);
		if (result2 == 1){ //operation failed
			return(-1);
		}
		struct tagdisk *tdd=calloc(RAID_DISKBLOCKS, sizeof(struct tagdisk));
		int i=0;
		for(i=0; array[i].disk1!=array[i].disk2; i++){
			if (array[i].disk1==disk){
				tdd[a]=array[i];
				a++;
			}
		}
		//then, recover everything in that disk
		int j=0;
		for (j=0; j<RAID_DISKBLOCKS; j++){
			RAIDOpCode request3 = create_raid_request(RAID_READ, 1, tdd[j].disk2, 0, tdd[j].blockid2);
			char b;
			RAIDOpCode resp3= client_raid_bus_request(request3, b);
	
			int result3 = extract_raid_response(resp3);
			if (result3 == 1){ //operation failed
				return(-1);
			}
			RAIDOpCode request4 =create_raid_request(RAID_WRITE, 1, tdd[j].disk1, 0, tdd[j].blockid1);
			RAIDOpCode resp4 = client_raid_bus_request(request4, b);
			int result4 = extract_raid_response(resp4);
			if (result4 == 1){ //operation failed
				return(-1);
			}
		}
		// Return successfully
		logMessage(LOG_INFO_LEVEL, "TAGLINE storage device: closing completed.");
	}
	return(0);
}
