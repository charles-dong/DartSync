/*
 * =====================================================================================
 *
 *       Filename:  tracker.h
 *
 *       Description:  
 * =====================================================================================
 */

#include "tracker.h"
#include <errno.h>
// ---------------- Global Variables
pthread_t * dlPeerTableHead = NULL;
pthread_mutex_t dl_peer_table_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t file_list_lock = PTHREAD_MUTEX_INITIALIZER; //ignore this, here to make filewalker happy
file_node_t *globalFileListHead; //global file list is a linked list of file_node_t
tracker_side_peer_t *peerListHead; //linked list of peers
char *loginPassword;
pthread_mutex_t* password_mutex;
int listeningfd; //main function listens on this socketfd for incoming connections;
pthread_t heartBeatTimerThread;
int WHO_AM_I = 1;
char root_path[WALK_SIZE];
int num_dl_threads;
pthread_mutex_t dl_num_lock = PTHREAD_MUTEX_INITIALIZER;
unsigned long duncompress = 0;
unsigned long tuncompress = 0;
unsigned long dcompress = 0;
unsigned long tcompress = 0;
unsigned long dencrypt = 0;
unsigned long tencrypt = 0;
int numpeers = 0;
time_t starttime;
time_t updatetime;
pthread_mutex_t stats_lock = PTHREAD_MUTEX_INITIALIZER;

/*
	Description: 
		1) Initializes globals, starts heartbeatTimer thread, and starts listening on HANDSHAKE_PORT
		2) When a new peer joins, creates a new handshake thread
		   and corresponding new peer table entry
*/
int main() {
	//initialize globals, start heartbeat timer thread, start listening
	listeningfd = tracker_initialize();
	printf("Listening for connections from peers...\n");

	//register a signal handler which is used to terminate the process
	signal(SIGINT, tracker_stop);

	//setup
	int newPeerConn;
	struct sockaddr_in tcpclient_addr;
	socklen_t tcpclient_addr_len = sizeof(struct sockaddr_in);

	//accept connections from peers on HANDSHAKE_PORT - when a new peer successfully joins, create
		//a new handshake thread and corresponding new peer table entry and handshake thread node
	while ((newPeerConn = accept(listeningfd, (struct sockaddr*) &tcpclient_addr, &tcpclient_addr_len))) {
		if (newPeerConn < 0){
			printf("Error accepting connection from peer!\n");
		} else {
			printf("Successfully connected to IP %s!\n", inet_ntoa(tcpclient_addr.sin_addr));
			if (addNewHandshakeThread(newPeerConn, tcpclient_addr) < 0) {
				printf("Error creating handshake thread. Closing connection to IP %s.\n", inet_ntoa(tcpclient_addr.sin_addr));
				close(newPeerConn);
			}
		}
	}
	printf("accept errno: %s\n", strerror(errno));
	return 0;
}

/* 		
	Description: initializes globals, start heartbeat timer thread, and starts listening
		for connections on HANDSHAKE_PORT

	Return Value: listening sockfd
*/
int tracker_initialize() {
	//initialize globals
	globalFileListHead = NULL;
	peerListHead = NULL;
	loginPassword = NULL;
	password_mutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(password_mutex, NULL);

	//start heartbeat timer thread
    if (pthread_create(&heartBeatTimerThread, NULL, heartbeatTimer, NULL)){
    	printf("Error creating heartbeat timer thread.\n");
    }

    //start listening for peer handshakes
	return create_listening_socket(HANDSHAKE_PORT, SOCK_STREAM);
}

/* 		
	Description: creates new handshake thread and creates/adds corresponding thread node in global thread list

	Return Value: 1 for success, -1 for failure
*/
int addNewHandshakeThread(int newPeerSockfd, struct sockaddr_in tcpclient_addr) {
	tracker_side_peer_t *newPeerNode = (tracker_side_peer_t *)malloc(sizeof(tracker_side_peer_t));
	memset(newPeerNode, 0, sizeof(tracker_side_peer_t));
	newPeerNode->sockfd = newPeerSockfd;
	strcpy(newPeerNode->ip, inet_ntoa(tcpclient_addr.sin_addr));
	newPeerNode->last_time_stamp = current_utc_time();
	newPeerNode->authenticated = 0;

	//create new handshake thread
    if (pthread_create(&newPeerNode->handshakeThread, NULL, handshake, newPeerNode)){
    	free(newPeerNode);
    	return -1;
    } else { //and add thread node to global thread list
    	printf("Successfully created new handshake thread for IP %s!\n", newPeerNode->ip);
    	 //add new node to beginning of thread node list
    	if (peerListHead != NULL)
    		newPeerNode->next = peerListHead;
    	else
    		newPeerNode->next = NULL;
    	peerListHead = newPeerNode;
    	return 1;
    }
}

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
	Parameters: sockfd (TCP connection to the peer this thread communicates with)

*/
void *handshake(void *arg) {
	ptp_peer_t* segmentFromPeer= (ptp_peer_t*)malloc(sizeof(ptp_peer_t));
    
	memset(segmentFromPeer, 0, sizeof(ptp_peer_t));
	tracker_side_peer_t *peerNode = (tracker_side_peer_t *)arg;
	int sockfd = peerNode->sockfd;
	char *segTypeStrings[] = {(char*)"REGISTER", (char*)"KEEP_ALIVE", (char*)"FILE_UPDATE", (char*)"AUTHENTICATION", (char*)"CHANGE_PASSWORD", (char*)"CLOSE"};

	while (receive_struct(sockfd, segmentFromPeer, PTP_PEER_T) >= 0) {
		printf("Received %s from IP %s.\n", segTypeStrings[segmentFromPeer->type], segmentFromPeer->peer_ip);
		switch(segmentFromPeer->type) {
			case REGISTER: {
				//update peerNode IP
				strncpy(peerNode->ip, segmentFromPeer->peer_ip, IP_LEN);
				
				if (!peerNode->authenticated){
					memset(segmentFromPeer, 0, sizeof(ptp_peer_t));
					break;
				}
				
				//Reply with interval and global file table
				ptp_tracker_t replyToRegisterSeg;
				memset(&replyToRegisterSeg, 0, sizeof(ptp_tracker_t));
				replyToRegisterSeg.interval = PEER_HEARTBEAT_INTERVAL;
				if (globalFileListHead) {
					replyToRegisterSeg.file_table = serializeFileList(globalFileListHead);
					if (replyToRegisterSeg.file_table == NULL) {
						printf("Error serializing global file list into char array in response to REGISTER.\n");
					}
				}
				if (send_struct(sockfd, &replyToRegisterSeg, PTP_TRACKER_T) < 0)
					printf("Error sending reply to REGISTER segment on sockfd %d.\n", sockfd);
				free(replyToRegisterSeg.file_table);
				memset(segmentFromPeer, 0, sizeof(ptp_peer_t));
				break;
			}
			case KEEP_ALIVE: {
				//update last_time_stamp for this peer
				peerNode->last_time_stamp = current_utc_time();
				memset(segmentFromPeer, 0, sizeof(ptp_peer_t));
				
				break;
			}
				
			case FILE_UPDATE: {
				
				if (!peerNode->authenticated){
					memset(segmentFromPeer, 0, sizeof(ptp_peer_t));
					break;
				}
				//get changes from file list and update the global file list
				
				
				/*
				 
				 The logic is that when the globalFileListHead is null, there's
				 no point in trying to find differences since it has to file list
				 of its own. So it should appropriate received file list -Saisi
				 
				 */
				
				if(segmentFromPeer->file_table){
					printf("Received filetable %s\n", segmentFromPeer->file_table);
				} else {
					printf("Received  NULL filetable \n");
				}
				
				if(!globalFileListHead){
					globalFileListHead = deserializeFileList(segmentFromPeer->file_table);
					
				} else{
					if(!segmentFromPeer->file_table) {
						printf("\nTracker just received an empty file table");
						free_linked_list(globalFileListHead);
						globalFileListHead = NULL;
						
					} else {
						file_collection_t* differences = getDifferences(segmentFromPeer->file_table, globalFileListHead);
						updateExistingListWithDifferences(differences);
						updatePeer_ips(globalFileListHead, differences);
						
						
						if(differences){
							if(differences->deleted_head){
								free_linked_list(differences->deleted_head);
							}
                            
							
							free(differences);
						}
						
					}
				}
				
				//broadcast to everyone
				if (broadcastGlobalFileList() < 0) {
					printf("Error broadcasting global file list from handshake thread for IP %s.\n", peerNode->ip);
				}
				memset(segmentFromPeer, 0, sizeof(ptp_peer_t));
				break;
			}
				
			case AUTHENTICATION: {
				ptp_tracker_t authenticationSeg;
				memset(&authenticationSeg, 0, sizeof(ptp_tracker_t));
				//compare with global password
				pthread_mutex_lock(password_mutex);
				if (loginPassword == NULL)
					authenticationSeg.interval = PASSWORD_NULL;
				else if (strncmp(loginPassword, segmentFromPeer->file_table, strlen(segmentFromPeer->file_table)) != 0)
					authenticationSeg.interval = PASSWORD_INCORRECT;
				else {
					authenticationSeg.interval = PASSWORD_CORRECT;
					peerNode->authenticated = 1;
				}
				pthread_mutex_unlock(password_mutex);
				
				//send authentication response
				authenticationSeg.file_table = NULL;
				if (send_struct(sockfd, &authenticationSeg, PTP_TRACKER_T) < 0)
					printf("Error sending authentication in response to AUTHENTICATION segment for IP %s.\n", peerNode->ip);
				memset(segmentFromPeer, 0, sizeof(ptp_peer_t));
				break;
			}
				
			case CHANGE_PASSWORD: {
				if (!peerNode->authenticated && loginPassword != NULL){
					memset(segmentFromPeer, 0, sizeof(ptp_peer_t));
					break;
				}
				//change global password
				pthread_mutex_lock(password_mutex);
				if (loginPassword != NULL)
					free(loginPassword);
				loginPassword = (char *)calloc(strlen(segmentFromPeer->file_table) + 1, sizeof(char)); //+1 for null char
				strncpy(loginPassword, segmentFromPeer->file_table, strlen(segmentFromPeer->file_table) + 1);
				
				//send back new password as ACK
				ptp_tracker_t* authentication = (ptp_tracker_t*)malloc(sizeof(ptp_tracker_t));
				memset(authentication, 0, sizeof(ptp_tracker_t));
				authentication->interval = PASSWORD_CHANGE_ACK;
				authentication->file_table = (char *)calloc(strlen(segmentFromPeer->file_table) + 1, sizeof(char));
				strncpy(authentication->file_table, loginPassword, strlen(segmentFromPeer->file_table) + 1);
				peerNode->authenticated = 1;
				pthread_mutex_unlock(password_mutex);
				
				//send ack
				if (send_struct(sockfd, authentication, PTP_TRACKER_T) < 0)
					printf("Error sending ack in response to CHANGE_PASSWORD segment for IP %s.\n", peerNode->ip);
				memset(segmentFromPeer, 0, sizeof(ptp_peer_t));
				free(authentication);
				break;
			}
			case CLOSE: {
				tracker_side_peer_t *currentPeerNode = peerListHead;
				tracker_side_peer_t *prevPeerNode = peerListHead;
				while (currentPeerNode != NULL) {
					
					//if we've passed the KEEP_ALIVE_TIMEOUT_PERIOD, cancel its handshakeThread and remove
					//it from the peer list
					if (currentPeerNode == peerNode) {

						//remove from peer list
						removeIPfromLinkedList(globalFileListHead, currentPeerNode->ip);
						printf("Removing IP %s from peer table due to CLOSE segment.\n", currentPeerNode->ip);
						close(currentPeerNode->sockfd);
						currentPeerNode->sockfd = -1;
						if (currentPeerNode == prevPeerNode) { //we're deleting the first node
							peerListHead = currentPeerNode->next;
							free(currentPeerNode);
						} else {
							prevPeerNode->next = currentPeerNode->next;
							free(currentPeerNode);
						}
						broadcastGlobalFileList();
						pthread_exit(NULL);
					}
					
					//go to next node
					if (currentPeerNode == prevPeerNode) {
						currentPeerNode = currentPeerNode->next;
					}
					else {
						currentPeerNode = currentPeerNode->next;
						prevPeerNode = prevPeerNode->next;
					}
				}
			}
		}
	}
	close(peerNode->sockfd);
	peerNode->sockfd = -1;
	pthread_exit(NULL);
}

/*
	Description: iterates through peer table (global var), broadcasting updated global file list to everyone
	Return Value: 1 if successful, -1 if failure
*/
int broadcastGlobalFileList() {
	int returnVal = 1;

	//create seg to broadcast
	ptp_tracker_t* segToBroadcast = (ptp_tracker_t* )malloc(sizeof(ptp_tracker_t));
    
	memset(segToBroadcast, 0, sizeof(ptp_tracker_t));
	segToBroadcast->interval = PEER_HEARTBEAT_INTERVAL;
	segToBroadcast->file_table = serializeFileList(globalFileListHead);

	//send segToBroadcast to all peers
	tracker_side_peer_t *currentPeerNode = peerListHead;
    
    
	while (currentPeerNode != NULL) {
		if (currentPeerNode->sockfd > 0 && send_struct(currentPeerNode->sockfd, segToBroadcast, PTP_TRACKER_T) < 0){
            fprintf(stderr, "Failed to send broadcast packet");
			printf("broadcast errno: %s\n", strerror(errno));
			returnVal = -1;
        }
		currentPeerNode = currentPeerNode->next;
	}
    
    printf("\n-----------------------------\n");
    printf("Sent global update\n");
    printf("\n%s", segToBroadcast->file_table);
    printf("\n-----------------------------\n");

	free(segToBroadcast->file_table);
	return returnVal;
}




/*
	Description: this thread checks the peer table's last_time_stamps to see if it has been more than
		KEEP_ALIVE_TIMEOUT_PERIOD since any peers have sent a KEEP_ALIVE segment. It does this every
		HEARTBEAT_CHECK_PERIOD. 
		If a peer has timed out, delete its entry in the peer table and send a message to its 
		corresponding handshake thread to exit
*/
void *heartbeatTimer(void *arg) {
	
	while (1){
		unsigned long currentTime = current_utc_time();
		tracker_side_peer_t *currentPeerNode = peerListHead;
		tracker_side_peer_t *prevPeerNode = peerListHead;
		while (currentPeerNode != NULL) {

			//if we've passed the KEEP_ALIVE_TIMEOUT_PERIOD, cancel its handshakeThread and remove
				//it from the peer list
			if (currentTime - currentPeerNode->last_time_stamp > KEEP_ALIVE_TIMEOUT_PERIOD) {
				//cancel handshakeThread
				if (pthread_cancel(currentPeerNode->handshakeThread)){
					printf("Error cancelling handshake thread with IP %s.\n", currentPeerNode->ip);
				}
				//remove from file table and peer list
				close(currentPeerNode->sockfd);
				currentPeerNode->sockfd = -1;
				removeIPfromLinkedList(globalFileListHead, currentPeerNode->ip);
				printf("Removing IP %s from peer table due to hearbeat timeout.\n", currentPeerNode->ip);
				if (currentPeerNode == prevPeerNode) { //we're deleting the first node
					currentPeerNode = currentPeerNode->next;
					peerListHead = currentPeerNode;
					free(prevPeerNode);
					prevPeerNode = currentPeerNode;
					continue;
				} else {
					prevPeerNode->next = currentPeerNode->next;
					free(currentPeerNode);
					currentPeerNode = prevPeerNode;
				}
			}

			//go to next node
			if (currentPeerNode == prevPeerNode) {
				currentPeerNode = currentPeerNode->next;
			}  else {
				currentPeerNode = currentPeerNode->next;
				prevPeerNode = prevPeerNode->next;
			}
		}

		sleep(HEARTBEAT_CHECK_PERIOD);
	}
	pthread_exit(NULL);
}




/*
	Description: close all connections, kill all threads, free everything
*/
void tracker_stop(int nothing) {
	printf("Closing connections.\n");
	close(listeningfd);
	pthread_cancel(heartBeatTimerThread);

	printf("Deleting peer list - closing connections, killing threads, and freeing.\n");
	tracker_side_peer_t *currentNode = peerListHead;
	while (currentNode != NULL) {
		close(currentNode->sockfd);
		if (pthread_cancel(currentNode->handshakeThread)){
			printf("Error cancelling handshake thread with IP %s.\n", currentNode->ip);
		}
		tracker_side_peer_t *tempNode = currentNode;
		currentNode = currentNode->next;
		free(tempNode);
	}
	
	printf("Deleting file list.\n");
	file_node_t *currentFileNode = globalFileListHead;
	while (currentFileNode != NULL) {
		free(currentFileNode->name);
		for (int i = 0; i < currentFileNode->numPeers; i++)
			free(currentFileNode->newpeerip[i]);
		file_node_t *tempFileNode = currentFileNode;
		currentFileNode = currentFileNode->next;
		free(tempFileNode);
	}

	pthread_mutex_destroy(password_mutex);
	free(password_mutex);
	if (loginPassword != NULL)
		free(loginPassword);
	printf("Tracker successfully exited.\n");
	exit(EXIT_SUCCESS);
}

/*
	Description: Prints current global file list, list of peers, and password
*/
void printGlobals() {
	printf("\nGlobal File List:\n");
	file_node_t *currentNode = globalFileListHead;
	while (currentNode != NULL) {
		printf("\t%c %s is at version %u with %d peers\n", currentNode->type, currentNode->name, currentNode->version_number, currentNode->numPeers);
		currentNode = currentNode->next;
	}

	printf("\nCurrent Peer List:\n");
	tracker_side_peer_t *currentPeerNode = peerListHead; //linked list of peers
	while (currentPeerNode != NULL) {
		printf("\tIP %s is on sockfd %d. Last login: %llu\n", currentPeerNode->ip, currentPeerNode->sockfd, currentPeerNode->last_time_stamp);
		currentPeerNode = currentPeerNode->next;
	}

	printf("\nPassword is %s\n", loginPassword);
}


/*
	Description: Returns current time in seconds
*/
unsigned long current_utc_time() {
	struct timespec ts;
	#ifdef __MACH__ // OS X does not have clock_gettime, use clock_get_time
	  clock_serv_t cclock;
	  mach_timespec_t mts;
	  host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
	  clock_get_time(cclock, &mts);
	  mach_port_deallocate(mach_task_self(), cclock);
	  ts.tv_sec = mts.tv_sec;
	  ts.tv_nsec = mts.tv_nsec;
	  //return ts.tv_nsec + ts.tv_sec*NANOSECONDS_PER_SECOND; //return time in nanoseconds
	  return ts.tv_sec; //return time in seconds
	#else
	  clock_gettime(CLOCK_REALTIME, &ts);
	  //return ts.tv_nsec; //return time in nanoseconds
	  return ts.tv_sec; //return time in seconds
	#endif
}

