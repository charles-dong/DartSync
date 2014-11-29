/*
 * =====================================================================================
 *
 *       Filename:  data_structures.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  05/10/2014 23:32:32
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Shuyang Fang (Shu), sfang@cs.dartmouth.edu
 *   Organization:  Dartmouth College - Department of Computer Science
 *
 * =====================================================================================
 */

#ifndef  data_structures_INC
#define  data_structures_INC

// ---------------- Prerequisites e.g., Requires "math.h"
#include <pthread.h>
#include <stdint.h>

// ---------------- Constants
#define WALK_SIZE           1024
#define IP_LEN              16
#define RESERVED_LEN        8
#define PROTOCOL_LEN        16 // Do we need this?
#define FILE_NAME_LEN       100 // Do we need this?
#define PASSWORD_CORRECT    (-1)
#define PASSWORD_INCORRECT  (-2)
#define PASSWORD_NULL       (-3)
#define PASSWORD_CHANGE_ACK (-4) 

// Request types for peer to tracker protocol
#define REGISTER        0
#define KEEP_ALIVE      1
#define FILE_UPDATE     2
#define AUTHENTICATION  3
#define CHANGE_PASSWORD 4
#define CLOSE           5

// data struture types, should be enum, don't care
// ADD ADDITIONAL TYPES HERE
#define PTP_PEER_T    0
#define PTP_TRACKER_T 1
#define DL_REQUEST    2
#define UL_RESPONSE   3

// File node types, directory or file
#define DIR_T  100
#define FILE_T 102

//Ports
#define HANDSHAKE_PORT 4758 //tracker listens on this port for new peers
#define P2P_PORT 5739



// ---------------- Structures/Types

/* Data structure for P2P or P2T communication */

typedef struct segment_peer {
  int32_t type;                         // packet type: REGISTER, KEEP_ALIVE, FILE_UPDATE
  int32_t port;                         // listening port number in p2p
  char reserved[RESERVED_LEN];          // reserved space: 8 bytes
  char peer_ip[IP_LEN];                 // the ip address of the peer sending this packet
  char *file_table;                     // client file table
} ptp_peer_t;

/* Data structure for T2P communication */

typedef struct segment_tracker {
  int64_t interval;             // time interval that the peer should send alive messages periodically
  char *file_table;             // tracker file table
} ptp_tracker_t;

/* Peer-side peer table */

typedef struct _peer_side_peer_t {
  char ip[IP_LEN];                // Remote peer IP address, 16 bytes
  char *file_name;                // Current downloading file name
  struct _peer_side_peer_t *next; // Pointer to the next peer, linked list
  uint64_t checksum;              // checksum of file currently being downloaded
} peer_side_peer_t;

/* Tracker-side peer table */

typedef struct _tracker_side_peer_t {
  struct _tracker_side_peer_t *next;
  char ip[IP_LEN];
  uint64_t last_time_stamp;
  int32_t sockfd;
  int32_t authenticated;
  pthread_t handshakeThread;
} tracker_side_peer_t;

typedef struct file_name {
  char name[WALK_SIZE];         // size of fileName and filePath
  struct file_name* next;       // next directory
  struct file_name* previous;   // prev directory
} file_name_t;

typedef struct file_node{
  char*  name;                  // file name
  char** newpeerip;             // for peer, peer IP address
  uint32_t numPeers;            // number of peers who have the latest version of the file
  uint32_t type;                // file or directory
  uint32_t size;                // size of the file
  uint32_t version_number;      // version control

  file_name_t* name_pointer;
  struct file_node* next;
  struct file_node* previous;
  uint64_t checksum;
} file_node_t;

/* Structure that a download peer will send to the upload peer. */
typedef struct dl_request {
  char *file_name;        // file the dler wants
  uint64_t start_pos;     // beginning to start calculating diff
  uint64_t length;        // length to check
  uint64_t req_ver;       // version the dler has
  char peer_ip[IP_LEN];   // IP of the upload peer
} dl_req_t;

/* Structure passed to download thread. */
typedef struct dl_arg {
  dl_req_t *dl_req;      // the dl_req
  uint64_t new_checksum; // the checksum of the ul
  int32_t port;          // the port of the upload peer
} dl_arg_t;

/* Structure that the upload peer will send to the download peer. */
typedef struct ul_response {
  uint64_t start_pos;       // the beginning pos of the diff
  uint64_t length;          // the length of the diff
  char *diff;               // the diff
  struct ul_response *next; // the next diff sequence
} ul_resp_t;

/* structure that will hold that linkedlists for various categories of altered files */
typedef struct file_collection{
    file_node_t* deleted_head; //deleted files
    file_node_t* new_head; //new files and modified files (incl. renamed/moved)
    file_node_t* modified_head;
    file_node_t* renamed_head;
    file_node_t* common_files;
} file_collection_t;

// ---------------- Public Variables

// ---------------- Prototypes/Macros

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  remove_peer_peer_t
 *  Description:  Removes the peer_side_peer_t with the given parameters.
 * =====================================================================================
 */
void remove_peer_peer_t(peer_side_peer_t **head, char *file_name, char *peer_ip);

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  free_peer_peer_t
 *  Description:  Free a peer_side_peer_t.
 * =====================================================================================
 */
void free_peer_peer_t(peer_side_peer_t *to_free);

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  get_local_file_chksum
 *  Description:  Returns the checksum associated with the file name.
 * =====================================================================================
 */
uint64_t get_local_file_chksum(file_node_t *head, char *file_name);

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  new_peer_peer_t
 *  Description:  Creates a new peer_side_peer_t with the given parameters.
 * =====================================================================================
 */
peer_side_peer_t *new_peer_peer_t(char *file_name, char *peer_ip, uint64_t checksum);

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  in_peer_peer_t_list
 *  Description:  Determines if the passed in file name is currently being 
 *  downloaded from the passed in peer ip. Checks through the peer-side peer 
 *  table and looks for a node that has the same ip and filename, if such 
 *  a node exists, return -1 otherwise return 0.
 * =====================================================================================
 */
int in_peer_peer_t_list(peer_side_peer_t **head, char *file_name, char *peer_ip, uint64_t checksum);

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  new_dl_req_t
 *  Description:  Creates a new dl_req_t with the given parameters.
 * =====================================================================================
 */
dl_req_t *new_dl_req_t(uint64_t start_pos, uint64_t req_ver, uint64_t length, char *file_name, char *ip);

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  new_ul_resp_t
 *  Description:  Creates a new ul_resp_t with the given parameters.
 * =====================================================================================
 */
ul_resp_t *new_ul_resp_t(uint64_t start_pos, uint64_t length, char *diff);

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  add_ul_resp_t
 *  Description:  Adds a ul_resp_t to the linked list of ul_resp_t.
 * =====================================================================================
 */
void add_ul_resp_t(ul_resp_t *head, ul_resp_t *to_add);

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  free_dl_req_t
 *  Description:  Frees a dl_req_t.
 * =====================================================================================
 */
void free_dl_req_t(dl_req_t *to_free);

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  free_ul_resp_t
 *  Description:  Frees a ul_resp_t.
 * =====================================================================================
 */
void free_ul_resp_t(ul_resp_t *to_free);

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  free_file_node_t
 *  Description:  Frees a file_node_t.
 * =====================================================================================
 */
void free_file_node_t(file_node_t *to_free);

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  create_file_node_t
 *  Description:  Creates a file_node_t.
 * =====================================================================================
 */
file_node_t *create_file_node_t(char *name, char **newpeerip, uint32_t numPeers, uint32_t type, uint32_t size, uint32_t version_number, uint64_t checksum);

#endif   /* ----- #ifndef data_structures_INC  ----- */
