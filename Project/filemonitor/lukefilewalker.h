/*
 * =====================================================================================
 *
 *       Filename:  filemonitor.h
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

#ifndef  filemonitor_INC
#define  filemonitor_INC

// ---------------- Prerequisites e.g., Requires "math.h"
#include <stdio.h>                           // fprintf
#include <stdlib.h> // exit
#include "../utils/data_structures.h"
// ---------------- Constants

// ---------------- Public Variables

// ---------------- Functions


/*
 * ===  FUNCTION  ======================================================================
 *         Name:  removeIPfromFileNode
 *  Description:  Removes a given ip from suppplied linked list
 * =====================================================================================*/
 
void removeIPfromLinkedList(file_node_t* node, char* ip);

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  getDifferences
 *  Description:  Obtain which files are new, deleted, and modified/renamed. Received
 *  string is the string representation of a global file list we've received
 * =====================================================================================
 */
file_collection_t* getDifferences(char* received_file_table, file_node_t* our);

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  updateExistingListWithDifferences
 *  Description:  Updates existing global file list with differences from
 *				  file_collection_t struct
 * =====================================================================================
 */
void updateExistingListWithDifferences( file_collection_t *differences);


/*
	Description: helper function to get checksum of file_node_t
	Return: checksum of passed file_node_t, or -1 for failure
	Parameters: node to get checksum of
*/
int checksumOfFile(file_node_t *file);

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  serializeFileList
 *  Description:  Turns file_node_t linked list into string array; returns NULL on failure
 * =====================================================================================
 */
char *serializeFileList(file_node_t *head);

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  deserializeFileList
 *  Description:  Turns string array into file_node_t linked list and returns head on success
 * 				  or NULL on failure
 * =====================================================================================
 */
file_node_t *deserializeFileList(char *serializedFileList);


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  addFileNodesToList
 *  Description:  adds linked list of file_collection_t->new_head to end of existing file list
 *                and re-alphabetizes
 * =====================================================================================
 */
void addFileNodesToList( file_node_t *new_modified_head);


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  deleteFileNodesFromList
 *  Description:  deletes nodes from existing file list with the same names as filesToDelete
 * =====================================================================================
 */
void deleteFileNodesFromList(file_node_t *filesToDelete);

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  start_traversal
 *  Description:  For given path 'directory', recursively obtains file and directory names
 *                and stores it as file_node_t linked list in global variable localFileListHead
 *       Return:  Returns 1 for success and -1 for failure
 * =====================================================================================
 */
int start_traversal();

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  crawlAndUpdate
 *  Description:  Call start_traversal and creates new config file (stores version number)
 * =====================================================================================
 */
int crawlAndUpdate();

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  set_tracker_con
 *  Description:  Sets globals: tracker connfd and home (DartSync Path)
 * =====================================================================================
 */
void set_tracker_con(int conn);

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  monitor_files
 *  Description:  crawls files every second and sends FILE_UPDATE segment to tracker if needed
 * =====================================================================================
 */
int monitor_files();

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  write_config
 *  Description:  Write a string to the config file in passed directory
 * =====================================================================================
 */
void write_config(char* string);


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  free_linked_list
 *  Description:  Frees linked list of file_node_t
 * =====================================================================================
 */
void free_linked_list(file_node_t* head);



/*
 * ===  FUNCTION  ======================================================================
 *         Name:  updatePeer_ips
 *  Description:  update newpeerip field for items in linkedlist
 * =====================================================================================
 */

void updatePeer_ips(file_node_t* head_of_list, file_collection_t* differences);

/*
 * ===  FUNCTION  ======================================================================
 *         Name:  send_fileupdate
 *  Description:  Sends out serialized file list to the tracker
 * =====================================================================================
 */

void send_fileupdate(char* newFiles);


/*
 * ===  FUNCTION  ======================================================================
 *         Name:  do_version_comparisons
 *  Description:  Removes files with lower version numbers than our current from file list
                    Then saves version info to config
 * =====================================================================================
 */

void do_version_comparisons_modified(file_node_t** head_of_mod_p);

/*
 * ===  FUNCTION  ======================================================================
 *         Name:  do_version_comparisons
 *  Description:  Bumps up our version number Then saves version info to confi
  ===================================================================================== */
void do_version_comparisons_commmon(file_node_t** head_of_common_p);


/*
 * ===  FUNCTION  ======================================================================
 *         Name:  do_version_comparisons
 *  Description:  Ignores deletion requests for files that are ahead of tracker version
 ===================================================================================== */
void do_version_comparisons_deleted(file_node_t** head_of_deleted_p);


#endif   /* ----- #ifndef filemonitor_INC  ----- */
