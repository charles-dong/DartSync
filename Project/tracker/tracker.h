/*
 * =====================================================================================
 *
 *       Filename:  tracker.h
 *
 *       Description:  
 * =====================================================================================
 */

#ifndef  tracker_INC
#define  tracker_INC

// ---------------- Prerequisites e.g., Requires "math.h"
#ifdef __MACH__
#include <mach/clock.h>
#include <mach/mach.h>
#endif

#include "../utils/data_structures.h"
#include "../utils/socket_common.h"
#include "../filemonitor/lukefilewalker.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <time.h>
// ---------------- Constants

#define KEEP_ALIVE_TIMEOUT_PERIOD 600 //10 min - after this, peers who haven't sent a KEEP_ALIVE to tracker will be deleted
#define HEARTBEAT_CHECK_PERIOD 60 //1 min
#define PEER_HEARTBEAT_INTERVAL 180 //3 min
// ---------------- Structs and Data Structures

// ---------------- Functions

/* 		
	Description: initializes globals, start heartbeat timer thread, and starts listening
		for connections on HANDSHAKE_PORT

	Return Value: listening sockfd
*/
int tracker_initialize();

/*
	Description: a new handshake thread is created by main() each time a new peer joins. 
		1) send a packet to the peer with the interval and latest global file list
		2) receive messages from the specific peer, responding as needed by using peer-tracker protocol
			CASE REGISTER
				1) send back interval and file table
			CASE KEEP_ALIVE
				1) update last_time_stamp in peer table for this peer
			CASE FILE_UPDATE
				1) call getNewFiles, getModifiedFiles, and getDeletedFiles to get any changes (NULL if no changes)
				2) call addFileNodesToList, deleteFileNodesFromList, and modifyFileNodesInList as needed to update
					global file list
				3) if there were changes, call broadcastGlobalFileList to notify all peers
			CASE AUTHENTICATION
				1) Compare char *file_table with loginPassword global
				2) send segment back to peer with interval = PASSWORD_CORRECT or PASSWORD_INCORRECT
			CASE CHANGE_PASSWORD
				1) Change loginPassword global to seg's char *file_table
				2) send segment back to peer with interval = PASSWORD_CHANGE_ACK and char *file_table = new password
	Parameters: peer list node for this particular peer

*/
void *handshake(void *arg);

/*
	Description: this thread checks to the peer table's last_time_stamps to see if it has been more than
		KEEP_ALIVE_TIMEOUT_PERIOD since any peers have sent a KEEP_ALIVE segment. It does this every
		HEARTBEAT_CHECK_PERIOD. 
		If a peer has timed out, delete its entry in the peer table and send a message to its 
		corresponding handshake thread to exit
*/
void *heartbeatTimer(void *arg);

/* 
	Description: iterates through peer table (global var), broadcasting updated global file list to everyone
	Return Value: 1 if successful, 
*/
int broadcastGlobalFileList();

/* 		
	Description: creates new handshake thread and creates/adds corresponding thread node in global thread list

	Return Value: 1 for success, -1 for failure
*/
int addNewHandshakeThread(int newPeerSockfd, struct sockaddr_in tcpclient_addr);

void tracker_stop(int nothing);

// ---------------- Support Functions

/*
	Description: Prints current global file list, list of peers, and password
*/
void printGlobals();

/*
	Description: Returns current time in seconds
*/
unsigned long current_utc_time();




#endif   /* ----- #ifndef tracker_INC  ----- */
