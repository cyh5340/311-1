////////////////////////////////////////////////////////////////////////////////
//
//  File           : raid_cache.c
//  Description    : This is the implementation of the cache for the TAGLINE
//                   driver.
//
//  Author         : ????
//  Last Modified  : ????
//

// Includes
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

// Project includes
#include <cmpsc311_log.h>
#include <cmpsc311_util.h>
#include <raid_cache.h>

//creat a structure for the cashe
struct ct{
uint64_t disk;
uint64_t blockid;
char* buff;
int s;//for the sussesion
};
//define it
struct ct *cache;
//
// TAGLINE Cache interface

////////////////////////////////////////////////////////////////////////////////
//
// Function     : init_raid_cache
// Description  : Initialize the cache and note maximum blocks
//
// Inputs       : max_items - the maximum number of items your cache can hold
// Outputs      : 0 if successful, -1 if failure

int init_raid_cache(uint32_t max_items) {
	cache=calloc(max_items, sizeof(struct ct));
	// Return successfully
	return(0);
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : close_raid_cache
// Description  : Clear all of the contents of the cache, cleanup
//
// Inputs       : none
// Outputs      : o if successful, -1 if failure

int close_raid_cache(void) {
	free(cache);
	// Return successfully
	return(0);
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : put_raid_cache
// Description  : Put an object into the block cache
//
// Inputs       : dsk - this is the disk number of the block to cache
//                blk - this is the block number of the block to cache
//                buf - the buffer to insert into the cache
// Outputs      : 0 if successful, -1 if failure

int put_raid_cache(RAIDDiskID dsk, RAIDBlockID blk, void *buf)  {
	//first, get the current size of the cache
	int n=0, i=0, temp=NULL;
	//n is for the index of the cache
	//i is for the index for the access order 
	//temp is to make sure if the cashe contains what we need
	for(n=0; cache[n].s==n; n++){
	//n will be the current size of the struct
	}
	
	//if the cach is not full yet
	if (cache[n].s!=n){
		//first, check if this block has been accessed before
		for (i=0; i<n; i++){
			if (cache[n].disk==dsk&&cache[n].blockid==blk){
				temp=n;
			}
		}//temp= NULL: if has never been accessed. temp=n: n is what we want
		//if it has never been accessed before			
		if (temp==NULL){
			cache[n].disk=dsk;
			cache[n].blockid=blk;
			cache[n].s=n;
			cache[n].buff=buf;
		}
		//if it has been accessed before
		else{
			struct ct *c=calloc(1, sizeof(struct ct));//just hold one for swipt
			c[0]= cache[temp];
			for (temp<n; 1==1; temp++){
				cache[temp]=cache[temp+1];
				cache[temp].s=temp;
				//move everything in the index, so that the latest accessed will stay at the greatest number of index
			}
			//after moving everything, eventually place the last accessed at the end.			
			cache[n]=c[0];
		}
	}
	//if the cache is already full
	else{
		//first, check if it is inside
		for (i=0; i<n; i++){
			if (cache[n].disk==dsk&&cache[n].blockid==blk){
				temp=n;
			}
		}//temp= NULL: it has never been accessed. temp=n: n is what we want
		//if it has never been accessed before
		//since the cache was already sorted by the order of previous use, the lowest index contains the earliest accessed index, which is 0
		if (temp==NULL){
			cache[0].disk=dsk;
			cache[0].blockid=blk;
			cache[0].s=n;
			cache[0].buff=buf;
		}
		//if it has been accessed before, just change the order of the struct
		else{
			struct ct *c=calloc(1, sizeof(struct ct));//just hold one for swipt
			c[0]= cache[temp];
			for (temp<n; 1==1; temp++){
				cache[temp]=cache[temp+1];
				cache[temp].s=temp;
				//move everything in the index, so that the latest accessed will stay at the greatest number of index
			}
			//after moving everything, eventually place the last accessed at the end.			
			cache[n]=c[0];
		}
	}
	// Return successfully
	return(0);
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : get_raid_cache
// Description  : Get an object from the cache (and return it)
//
// Inputs       : dsk - this is the disk number of the block to find
//                blk - this is the block number of the block to find
// Outputs      : pointer to cached object or NULL if not found

void * get_raid_cache(RAIDDiskID dsk, RAIDBlockID blk) {
	int n=0, j=0, k=0;
	struct ct *c=calloc(1, sizeof(struct ct));
	char a=NULL;
	for (n=0; cache[n].s==n; n++){
		//n will the current size of the struct 
		if (cache[n].disk==dsk&&cache[n].blockid==blk){
		a=cache[n].buff;
		j=n;	
		//then reorder the struct
		}
	}
	if (a!=NULL){
		c[0]=cache[j];
		for(j<n; 1==1; j++){
			cache[j]=cache[j+1];
			cache[j].s=j;
		}
	}
	return(a);
}

