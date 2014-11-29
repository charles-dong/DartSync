/*
 * =====================================================================================
 *
 *       Filename:  socket_common.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  05/10/2014 20:57:48
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Shuyang Fang (Shu), sfang@cs.dartmouth.edu
 *   Organization:  Dartmouth College - Department of Computer Science
 *
 * =====================================================================================
 */

#ifndef  socket_common_INC
#define  socket_common_INC

// ---------------- Prerequisites e.g., Requires "math.h"
#include <inttypes.h>

// ---------------- Constants
#define MAX_CONN  256

// ---------------- Structures/Types

// ---------------- Public Variables


// ---------------- Prototypes/Macros

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  send_struct
 *  Description:  Sends a struct of TYPE to the given sockfd. Returns 0 on 
 *  success, -1 on failure.
 * =====================================================================================
 */
int send_struct(int sockfd, void *unknown, int TYPE);

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  receive_struct
 *  Description:  Receives a struct of TYPE from the given sockfd. Returns 
 *  0 on success, -1 on failure.
 *
 *  ASSUMES: Unknown has been allocated. If the struct has a pointer field, it is set to NULL.
 * =====================================================================================
 */
int receive_struct(int sockfd, void *unknown, int TYPE);

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  create_listening_socket
 *  Description:  Creates a socket of the given type on the given port. Begins 
 *  listening on it, up to MAX_CONN, return the fd.
 * =====================================================================================
 */
int create_listening_socket(int port, int type);

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  connect_to_server
 *  Description:  Given the ip and port, connects to a TCP server socket and returns 
 *  the client side fd.
 * =====================================================================================
 */
int connect_to_server(int port, char *server_ip); 

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  close_socket
 *  Description:  Shutdown and close the socket. Abortive close if option 
 *  valid.
 * =====================================================================================
 */
void close_socket(int sockfd, int abort);

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  hostname_to_ip
 *  Description:  Finds the ip linked to the given hostname. Places it into 
 *  the passed in ip buffer.
 * =====================================================================================
 */
int hostname_to_ip(char *hostname, char *ip); 

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  get_my_ip
 *  Description:  Finds the ip linked to the calling host. Places it into the 
 *  passed in ip buffer.
 * =====================================================================================
 */
int get_my_ip(char *ip);

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  find_port
 *  Description:  Finds and returns the port number associated with the given 
 *  sockfd.
 * =====================================================================================
 */
int find_port(int sockfd); 

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  ntoh64
 *  Description:  Converts network order to host order for 64 bit.
 * =====================================================================================
 */
uint64_t ntoh64(const uint64_t *input);

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  hton64
 *  Description:  Converts host order to network order for 64 bits.
 * =====================================================================================
 */
uint64_t hton64(const uint64_t *input);


#endif   /* ----- #ifndef socket_common_INC  ----- */
