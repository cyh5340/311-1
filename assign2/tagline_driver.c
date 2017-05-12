///////////////////////////////////////////////////////////////////////////////
//
//  File           : tagline_driver.c
//  Description    : This is the implementation of the driver interface
//                   between the OS and the low-level hardware.
//
//  Author         : ?????
//  Created        : ?????

// Include Files
#include <stdlib.h>
#include <string.h>
#include <cmpsc311_log.h>

// Project Includes
#include "raid_bus.h"
#include "tagline_driver.h"

//
// Functions

//stuct tagdisk{
//	uint8_t disk
//	uint8_t blockid
//}

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
////////////////////////////////////////////////////////////////////////////////
//
// Function     : tagline_driver_init
// Description  : Initialize the driver with a number of maximum lines to process
//
// Inputs       : maxlines - the maximum number of tag lines in the system
// Outputs      : 0 if successful, -1 if failure

int tagline_driver_init(uint32_t maxlines) {
	//This part start the whole thing
	RAIDOpCode request = create_raid_request(RAID_INIT, (RAID_DISKBLOCKS/RAID_TRACK_BLOCKS), RAID_DISKS, 0, 0); 
	RAIDOpCode resp = raid_bus_request(request, NULL);
	int result = extract_raid_response(resp);

	if (result == 1){ //operation failed
		return (-1);
	}
	//This part format the whole thing (putting 0s)
	int i=0;
	for (i=0; i<RAID_DISKS; i++){
	RAIDOpCode request2 = create_raid_request(RAID_FORMAT, 0, i, 0, 0);
	RAIDOpCode resp2 = raid_bus_request(request2, NULL);
	
	int result2 = extract_raid_response(resp2);
		if (result2 == 1){ //operation failed
			return -1;
		}	
	}
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
	if (tag< bnum){	
		tag=tag+bnum;	
		RAIDOpCode request = create_raid_request(RAID_READ, blks, tag, 0, bnum);
		RAIDOpCode resp = raid_bus_request(request, buf);
		int result = extract_raid_response(resp);

		if (result == 1){ //operation failed
			return (-1);
		}
	}
	else{
		return (-1);
	}
	
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

	if (bnum< tag){
		return (-1);
	}
	else{
		tag = tag+bnum;
		create_raid_request(RAID_FORMAT, blks, tag, 0, bnum);	
		RAIDOpCode request = create_raid_request(RAID_WRITE, blks, tag, 0, bnum);
		RAIDOpCode resp = raid_bus_request(request, buf);
		int result = extract_raid_response(resp);

		if (result == 1){ //operation failed
			return (-1);
		}
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
	RAIDOpCode resp = raid_bus_request(request, NULL);
	int result = extract_raid_response(resp);

	if (result == 1){ //operation failed
	return(-1);
	}
	// Return successfully
	logMessage(LOG_INFO_LEVEL, "TAGLINE storage device: closing completed.");
	return(0);
}
