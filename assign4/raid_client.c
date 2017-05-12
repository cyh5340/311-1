////////////////////////////////////////////////////////////////////////////////
//
//  File          : raid_client.c
//  Description   : This is the client side of the RAID communication protocol.
//
//  Author        : ????
//  Last Modified : ????
//

// Include Files
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <stdint.h>

// Project Include Files
#include <raid_network.h>
#include <cmpsc311_log.h>
#include <cmpsc311_util.h>

// Global data
unsigned char *raid_network_address = NULL; // Address of CRUD server
unsigned short raid_network_port = 0; // Port of CRUD server

//
// Functions

////////////////////////////////////////////////////////////////////////////////
//
// Function     : client_raid_bus_request
// Description  : This the client operation that sends a request to the RAID
//                server.   It will:
//
//                1) if INIT make a connection to the server
//                2) send any request to the server, returning results
//                3) if CLOSE, will close the connection
//
// Inputs       : op - the request opcode for the command
//                buf - the block to be read/written from (READ/WRITE)
// Outputs      : the response structure encoded as needed

RAIDOpCode client_raid_bus_request(RAIDOpCode op, void *buf) {
	int socket_fd;
	struct sockaddr_in caddr;
	
	caddr.sin_family = AF_INET;
	caddr.sin_port = htons(16453);
	if (inet_aton(RAID_DEFAULT_IP, &caddr.sin_addr)==0){
		return(-1);
	}
	
	socket_fd= socket(PF_INET, SOCK_STREAM, 0);
	if(socket_fd== -1){
		printf("Error\n", strerror(errno));
		return(-1);
	}

	if (connect(socket_fd, (const struct sockaddr *)&caddr, sizeof(caddr))==-1){
		printf("Error\n", strerror(errno));
		return(-1);
	}

	op = htonl(1);	
	if (write(socket_fd, &op, sizeof(op))!=sizeof(op)){
		printf("Error\n", strerror(errno));
		return(-1);
	}
	prntf("Send\n", ntohl(op));

	if (read(socket_fd, &op, sizeof(op))!=sizeof(op)){
		printf("Error\n", strerror(errno));
		return(-1);
	}
	op=ntohl(op);
	prntf("Recieve\n", op);


	close(socket_fd);//close the socket
    return(0);

}

