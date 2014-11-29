/*
 * =====================================================================================
 *
 *       Filename:  data_structures.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  05/14/2014 17:53:05
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Shuyang Fang (Shu), sfang@cs.dartmouth.edu
 *   Organization:  Dartmouth College - Department of Computer Science
 *
 * =====================================================================================
 */

#include <arpa/inet.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "data_structures.h"
#include "common.h"

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  remove_peer_peer_t
 *  Description:  Removes the peer_side_peer_t with the given parameters.
 * =====================================================================================
 */
void remove_peer_peer_t(peer_side_peer_t **head, char *file_name, char *peer_ip)
{
  peer_side_peer_t *current  = *head;
  peer_side_peer_t *previous = NULL;
  peer_side_peer_t *next     = NULL;

  if (*head && (*head)->ip && (*head)->file_name && !strcmp((*head)->ip, peer_ip) && !strcmp((*head)->file_name, file_name)) {
    previous  = *head;
    *head     = (*head)->next;
    free_peer_peer_t(previous);
      previous = NULL;
    return;
  }

  while (current->next) {
    next = current->next;

    if (next->ip && next->file_name && !strcmp(next->ip, peer_ip) && !strcmp(next->file_name, file_name)) {
      current->next = next->next;
      free_peer_peer_t(next);
      next = NULL;
      return;
    }

    current = current->next;
  }
}		/* -----  end of function remove_peer_peer_t  ----- */

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  get_local_file_chksum
 *  Description:  Returns the checksum associated with the file name.
 * =====================================================================================
 */
uint64_t get_local_file_chksum(file_node_t *head, char *file_name)
{
  file_node_t *current = head;

  while (current) {
    if (!strcmp(current->name, file_name))
      return current->checksum;

    current = current->next;
  }

  return 0;
}		/* -----  end of function get_local_file_chksum  ----- */

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  new_peer_peer_t
 *  Description:  Creates a new peer_side_peer_t with the given parameters.
 * =====================================================================================
 */
peer_side_peer_t *new_peer_peer_t(char *file_name, char *peer_ip, uint64_t checksum)
{
  peer_side_peer_t *new = calloc(1, sizeof(peer_side_peer_t));
  MALLOC_CHECK(stderr, new);
  new->file_name        = calloc(1, strlen(file_name) + 1);
  MALLOC_CHECK(stderr, new->file_name);
  new->checksum         = checksum;
  new->next             = NULL;
  strncpy(new->file_name, file_name, strlen(file_name));
  strcpy(new->ip, peer_ip);

  return new;
}		/* -----  end of function new_peer_peer_t  ----- */

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  free_peer_peer_t
 *  Description:  Free a peer_side_peer_t.
 * =====================================================================================
 */
void free_peer_peer_t(peer_side_peer_t *to_free)
{
  free(to_free->file_name);
  free(to_free);
  to_free = NULL;
}		/* -----  end of function free_peer_peer_t  ----- */

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  in_peer_peer_t_list
 *  Description:  Determines if the passed in file name is currently being 
 *  downloaded from the passed in peer ip OR if a different version of the 
 *  passed in file name is being downloaded from any peer.
 *
 *  Checks through the peer-side peer table and looks for a node that has the same ip 
 *  and filename OR same filename and different version, if such 
 *  a node exists, return -1 otherwise return 0.
 * =====================================================================================
 */
int in_peer_peer_t_list(peer_side_peer_t **head, char *file_name, char *peer_ip, uint64_t checksum)
{
  peer_side_peer_t *current = *head;

  // if the ip is NULL, we're just checking if the file is being downloaded at 
  // all
  if (!peer_ip) {
    while (current) {
      if (current->file_name && !strcmp(current->file_name, file_name))
        return -1;

      if (!current->next)
        return 0;

      current = current->next;
    }
    return 0;
  }

  // if the head isn't initialized, we do so
  if (!*head) {
    *head = new_peer_peer_t(file_name, peer_ip, checksum);
    return 0;
  }

  // otherwise, we have to move through the list and check if 
  // we are currently downloading the file from the peer

  while (current) {
    if ((current->ip && current->file_name && !strcmp(current->ip, peer_ip) && !strcmp(current->file_name, file_name)) 
        || (current->file_name && !strcmp(current->file_name, file_name) && current->checksum != checksum))
      return -1;

    if (!current->next) {
      current->next = new_peer_peer_t(file_name, peer_ip, checksum);
      return 0;
    }

    current = current->next;
  }
  
  return 0;
}		/* -----  end of function in_peer_peer_t_list  ----- */


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  new_dl_req_t
 *  Description:  Creates a new dl_req_t with the given parameters.
 * =====================================================================================
 */
dl_req_t *new_dl_req_t(uint64_t start_pos, uint64_t req_ver, uint64_t length, char *file_name, char *ip)
{
  dl_req_t *new  = calloc(1, sizeof(dl_req_t));
  MALLOC_CHECK(stderr, new);
  new->start_pos = start_pos;
  new->req_ver   = req_ver;
  new->length    = length;
  new->file_name = NULL;

  if (file_name) {
    new->file_name = calloc(strlen(file_name), sizeof(char) + 1);
    MALLOC_CHECK(stderr, new->file_name);
    memcpy(new->file_name, file_name, strlen(file_name));
  }

  if (ip)
    memcpy(new->peer_ip, ip, INET_ADDRSTRLEN);

  return new;
}		/* -----  end of function new_dl_req_t  ----- */

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  free_dl_req_t
 *  Description:  Frees a dl_req_t.
 * =====================================================================================
 */
void free_dl_req_t(dl_req_t *to_free)
{
  if (!to_free)
    return;

  if (to_free->file_name)
    free(to_free->file_name);
  free(to_free);
  to_free = NULL;
}		/* -----  end of function free_dl_req_t  ----- */

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  new_ul_resp_t
 *  Description:  Creates a new ul_resp_t with the given parameters.
 * =====================================================================================
 */
ul_resp_t *new_ul_resp_t(uint64_t start_pos, uint64_t length, char *diff)
{
  ul_resp_t *new = calloc(1, sizeof(ul_resp_t));
  MALLOC_CHECK(stderr, new);
  new->start_pos = start_pos;
  new->length    = length;
  new->next      = NULL;
  new->diff      = NULL;

  if (length) {
    new->diff    = calloc(length, sizeof(char));
    MALLOC_CHECK(stderr, new->diff);
    memcpy(new->diff, diff, length);
  }

  return new; 
}		/* -----  end of function new_ul_resp_t  ----- */

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  add_ul_resp_t
 *  Description:  Adds a ul_resp_t to the linked list of ul_resp_t.
 * =====================================================================================
 */
void add_ul_resp_t(ul_resp_t *head, ul_resp_t *to_add)
{
  if (!to_add)
    return;

  if (!head) {
    head         = to_add;
    to_add->next = NULL;
    return;
  }

  ul_resp_t *current = head;

  while (current->next)
    current = current->next;

  current->next = to_add;
  to_add->next  = NULL;
}		/* -----  end of function add_ul_resp_t  ----- */

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  free_ul_resp_t
 *  Description:  Frees a ul_resp_t.
 * =====================================================================================
 */
void free_ul_resp_t(ul_resp_t *to_free)
{
  if (!to_free)
    return;

  ul_resp_t *prev    = NULL;
  ul_resp_t *current = to_free;

  while (current) {
    prev = current;
    current = current->next;

    if (prev->diff) 
      free(prev->diff);

    free(prev);
  }
}		/* -----  end of function free_ul_resp_t  ----- */

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  free_file_node_t
 *  Description:  Frees a file_node_t.
 * =====================================================================================
 */
void free_file_node_t(file_node_t *to_free)
{
  uint32_t idx = 0;

  if (to_free->name) {
    free(to_free->name);
    to_free->name = NULL;
  }

  if (to_free->numPeers) {
    for (idx = 0; idx < to_free->numPeers; idx++) {
      free(to_free->newpeerip[idx]);
      to_free->newpeerip[idx] = NULL;
    }

    free(to_free->newpeerip);
    to_free->newpeerip = NULL;
  }

  free(to_free);
  to_free = NULL;
}		/* -----  end of function free_file_node_t  ----- */

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  create_file_node_t
 *  Description:  Creates a file_node_t.
 * =====================================================================================
 */
file_node_t *create_file_node_t(char *name, char **newpeerip, uint32_t numPeers, uint32_t type, uint32_t size, uint32_t version_number, uint64_t checksum)
{
  uint32_t idx = 0;

  file_node_t *new = calloc(1, sizeof(file_node_t));
  MALLOC_CHECK(stderr, new);

  new->name = calloc(strlen(name) + 1, sizeof(char));
  MALLOC_CHECK(stderr, new->name);

  strcpy(new->name, name);

  if (numPeers) {
    new->newpeerip = calloc(numPeers, sizeof(char *));
    MALLOC_CHECK(stderr, new->newpeerip);

    for (idx = 0; idx < numPeers; idx++) {
      new->newpeerip[idx] = calloc(strlen(newpeerip[idx]) + 1, sizeof(char));
      MALLOC_CHECK(stderr, new->newpeerip[idx]);
      strcpy(new->newpeerip[idx], newpeerip[idx]);
    }
  }
  else
    new->newpeerip = NULL;

  new->numPeers       = numPeers;
  new->type           = type;
  new->size           = size;
  new->version_number = version_number;
  new->checksum       = checksum;

  new->name_pointer = NULL;
  new->next         = NULL;
  new->previous     = NULL;

  return new;
}		/* -----  end of function create_file_node_t  ----- */
