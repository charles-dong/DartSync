/*
 * =====================================================================================
 *
 *       Filename:  peer.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  05/10/2014 18:30:25
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author: 
 *   Organization:  Dartmouth College - Department of Computer Science
 *
 * =====================================================================================
 */

#ifndef  peer_INC
#define  peer_INC

// ---------------- Prerequisites e.g., Requires "math.h"
#include "../utils/data_structures.h"

// ---------------- Constants

// interval of time in seconds between checking the peer table dl list to see if
// a dl can proceed, up to modification
#define CHECK_PEER_TABLE_INTERVAL 1


// ---------------- Global Variables

file_node_t *globalFileListHead;   //global file list is a linked list of file_node_t
peer_side_peer_t *dlPeerTableHead; //peer table of peers currently being dl'd from
peer_side_peer_t *ulPeerTableHead; //peer table of peers currently being ul'd to

int tracker_conn; 	//socket file descriptor to tracker connection
int P2P_conn; 		  //socket file descriptor to P2P listener connection

int heartbeat_interval;
int WHO_AM_I = 0;
int num_dl_threads;

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

pthread_t filemonitor_thread;
pthread_t P2P_listener_thread;
pthread_t heartbeat_thread;
pthread_t logger_thread;
pthread_t *download_thread_list_head;
pthread_t *upload_thread_list_head;

pthread_mutex_t ul_peer_table_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t dl_peer_table_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t dl_num_lock        = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t file_list_lock = PTHREAD_MUTEX_INITIALIZER;

char* root_path   = NULL;
char* backup_path = NULL;

unsigned long root_path_len;
unsigned long bu_path_len;


// ---------------- Functions

/*
	Description: 
		1) Connects to tracker
		2) Parse information returned by the tracker
		3) Start file monitor thread
		4) Start P2P listening thread
		5) Start alive thread with given KEEP_ALIVE_TIMEOUT_PERIOD
		6) Create appropriate P2P Download threads
*/

//int main(int argc, char** argv);

/*
	Description: Receive information from the tracker and update file list
*/
void waitTracker();

/*
	Description: 
		1) Monitor a local file directory
		2) Send out updated file table to the tracker if any changes occur
*/
void *file_monitor_thread(void *arg);

/* 		
	Description:
		1) Listen on the P2P port
		2) Receive download requests from other peers
		3) Create appropriate P2P Upload threads
*/
void *P2P_listening_thread(void *arg);

/*
	Description: this thread sends a heartbeat message to the tracker every KEEP_ALIVE_TIMEOUT_PERIOD.
*/
void *alive_thread(void *heartbeat_arg);

/*
	Description: this thread downloads data from a remote peer
	Parameters: peer information, file segment being downloaded
*/
void *P2P_download_thread(void *download_arg);

/*
	Description: this thread uploads data to a remote peer
	Parameters: peer information, file segment to send
*/
void *P2P_upload_thread(void *upload_arg);

/*
	Description: this thread uploads data to a remote peer
	Parameters: peer information, file segment to send
*/
void *P2P_upload_thread(void *upload_arg);

/*
	Description: frees memory and closes connections when SIGINT signal occurs
*/
void cleanup(int arg);

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  getDartSyncPath
 *  Description:  Gets the absolute pathname for DartSync. /Users/UID/DartSync
 *  Returns strlen
 * =====================================================================================
 */
unsigned long getDartSyncPath(char *string);

void applyDifferences(file_collection_t *collection);
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  getDartSyncBackUpPath
 *  Description:  Gets the absolute pathname for DartSync Backup. 
 *  /Users/UID/.DartSync Returns strlen.
 * =====================================================================================
 */
unsigned long getDartSyncBackUpPath(char *string);
void free_dl_table(peer_side_peer_t *head);
void free_ul_table(peer_side_peer_t *head);

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  master_delete_thread
 *  Description:  Carries out deletion of the file in the passed in arg.
 * =====================================================================================
 */
void *master_delete_thread(void *arg);

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  master_new_dl_thread
 *  Description:  Carries out download of new file with parameters in passed 
 *  in arg.
 * =====================================================================================
 */
void *master_new_dl_thread(void *arg);

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  master_mod_dl_thread
 *  Description:  Carries out modification of file with paramters passed in.
 * =====================================================================================
 */
void *master_mod_dl_thread(void *arg);

void *logging_thread(void *arg);

// ---------------- Helper Functions

// Parse received information from tracker

// Package download request into struct to send to peer

// Parse received download request struct from peer

#endif   /* ----- #ifndef peer_INC  ----- */
