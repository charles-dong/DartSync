/*
  pthread_mutex_unlock(&dl_peer_table_lock);
 * =====================================================================================
 *
 *       Filename:  peer.c
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <signal.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <pwd.h>
#include <errno.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <time.h>

#include "./peer.h"
#include "../utils/data_structures.h"
#include "../utils/socket_common.h"
#include "../utils/common.h"
#include "../filemonitor/lukefilewalker.h"


//TODO: put globals for root_path, root_path_len, backup_path, and backup_path_len in .h

/*
Description:
1) Connects to tracker
2) Parse information returned by the tracker
3) Start file monitor thread
4) Start P2P listening thread
5) Start alive thread with given KEEP_ALIVE_TIMEOUT_PERIOD
6) Create appropriate P2P Download threads
*/

int main(int argc, char** argv) {

  root_path = calloc(WALK_SIZE, sizeof(char));
  backup_path = calloc(WALK_SIZE, sizeof(char));

  root_path_len = getDartSyncPath(root_path);
  bu_path_len   = getDartSyncBackUpPath(backup_path);

  if (mk_path(root_path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)) {
    LOG(stderr, "Unable to create root_path!");
    exit(-1);
  }

  if (mk_path(backup_path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)) {
    LOG(stderr, "Unable to create backup_path!");
    exit(-1);
  }

  // init some globals
  globalFileListHead   = NULL;
  dlPeerTableHead      = NULL;
  ulPeerTableHead      = NULL;

  pthread_mutex_lock(&dl_num_lock);
  num_dl_threads       = 0;
  pthread_mutex_unlock(&dl_num_lock);

  // Get user input for host and password
  char input1[20];
  memset(input1, 0, 20);
  printf("Please enter tracker address: ");
  scanf("%s", input1);
  //sprintf(input1, "localhost");

  char *ip = (char *)calloc(sizeof(char), 16);
  hostname_to_ip(input1, ip);
  tracker_conn = connect_to_server(HANDSHAKE_PORT, ip);
  if (tracker_conn < 0) {
    printf("Error connecting to tracker, exiting.\n");
    exit(0);
  }

  set_tracker_con(tracker_conn);/* pass tracker conn to file_walker*/

  ptp_peer_t login;
  memset((char *) &login, 0, sizeof(ptp_peer_t));
  login.type = AUTHENTICATION;
  char input2[20];
  printf("Please enter authentication password: ");

  memset(input2, 0, 20);
  scanf("%s", input2);
  login.file_table = input2;

  // Attempt to log in
  int max_tries = 5;
  while (max_tries > 0) {
    if (send_struct(tracker_conn, &login, PTP_PEER_T) < 0) {
      printf("Error sending login, exiting\n");
      exit(1);
    }

    ptp_tracker_t login_response;
    memset((char *) &login_response, 0, sizeof(ptp_tracker_t));
    if (receive_struct(tracker_conn, &login_response, PTP_TRACKER_T) < 0) {
      printf("Error receiving login response, exiting\n");
      exit(1);
    }

    free(login_response.file_table);
    login_response.file_table = NULL;

    if (login_response.interval == PASSWORD_CORRECT) {
      break;
    }
    if (login_response.interval == PASSWORD_INCORRECT) {
      printf("Incorrect password.\nTry again: ");
      memset(input2, 0, 20);
      scanf("%s", input2);
      login.file_table = input2;
      max_tries--;
      continue;
    }
    if (login_response.interval == PASSWORD_NULL) {
      login.type = CHANGE_PASSWORD;
    }
    if (login_response.interval == PASSWORD_CHANGE_ACK) {
      break;
    }
  }

  if (max_tries == 0) {
    printf("Max retries exceeded, exiting\n");
    exit(1);
  }

  // Send registration packet
  ptp_peer_t reg;
  memset(&reg, 0, sizeof(ptp_peer_t));
  reg.type = REGISTER;
  get_my_ip(reg.peer_ip);
  reg.port = find_port(tracker_conn);

  if (send_struct(tracker_conn, &reg, PTP_PEER_T) < 0) {
    printf("Error sending register, exiting\n");
    exit(1);
  }

  printf("\nFormat file_name:checksum:newpeerip:numpeers:type:size:version\n");

  // Get registration response including global file table
  ptp_tracker_t* reg_response= (ptp_tracker_t*)malloc(sizeof(ptp_tracker_t));

  memset(reg_response, 0, sizeof(ptp_tracker_t));
  if (receive_struct(tracker_conn, reg_response, PTP_TRACKER_T) < 0) {
    printf("Error receiving register response, exiting\n");
    exit(1);
  }

  if(reg_response->file_table)
    printf("Received initial file table %s\n", reg_response->file_table);
  else
    printf("Received empty file table from tracker\n");

  // register for SIGINT
  signal(SIGINT, cleanup);

  // Get heartbeat interval
  heartbeat_interval = (int) reg_response->interval;

  //Crawl folder to make local file table
  crawlAndUpdate();

  // If tracker already has files, download those and recrawl local folder
  if (reg_response->file_table && strlen(reg_response->file_table) > 0) {

    printf("\nTracker already has files. Applying updates");
    file_collection_t *collection = getDifferences(reg_response->file_table, globalFileListHead);
    if (collection->common_files)
      do_version_comparisons_commmon(&collection->common_files);
    if (collection->deleted_head)
      do_version_comparisons_deleted(&collection->deleted_head);
    if (collection->modified_head)
      do_version_comparisons_modified(&collection->modified_head);

    applyDifferences(collection);

    free_linked_list(collection->common_files);
    free_linked_list(collection->deleted_head);
    free_linked_list(collection->modified_head);
    free_linked_list(collection->new_head);
    free_linked_list(collection->renamed_head);
    free(collection);
    sleep(5);
    if(pthread_mutex_lock(&dl_num_lock)){
      perror("\tpthread_mutex_lock\n");
      exit(1);
    }


    while(num_dl_threads){
      pthread_mutex_unlock(&dl_num_lock);
      sleep(1);
      if(pthread_mutex_lock(&dl_num_lock)){
        perror("\tpthread_mutex_lock\n");
        exit(1);
      }
    }


    /* crawl FS, get file list and update globalFileListHead*/

    crawlAndUpdate();
    pthread_mutex_unlock(&dl_num_lock);
  }

  //cleanup
  if(reg_response->file_table)
    free(reg_response->file_table);
  if(reg_response)
    free(reg_response);

  // Send local file table
  ptp_peer_t* fileupdate = (ptp_peer_t*)malloc(sizeof(ptp_peer_t));
  memset(fileupdate, 0, sizeof(ptp_peer_t));
  fileupdate->type = FILE_UPDATE;
  get_my_ip(fileupdate->peer_ip);
  fileupdate->port = find_port(tracker_conn);
  fileupdate->file_table = serializeFileList(globalFileListHead);

  printf("[%s/%d]: Sent out initial file table!\n%s", __FILE__, __LINE__, fileupdate->file_table);
  send_struct(tracker_conn, fileupdate, PTP_PEER_T);


  if(fileupdate->file_table)
    free(fileupdate->file_table);

  pthread_create(&filemonitor_thread, NULL, file_monitor_thread, NULL);
  pthread_create(&P2P_listener_thread, NULL, P2P_listening_thread, NULL);
  pthread_create(&heartbeat_thread, NULL, alive_thread, NULL);
	time(&starttime);
	pthread_create(&logger_thread, NULL, logging_thread, NULL);
  waitTracker();

}

/*
Description: Receive information from the tracker and update file list
*/
void waitTracker() {

  //setup
  ptp_tracker_t* tracker_info = (ptp_tracker_t*)malloc(sizeof(ptp_tracker_t));
  memset(tracker_info, 0, sizeof(ptp_tracker_t));
  file_collection_t *collection = NULL;

  while (receive_struct(tracker_conn, tracker_info, PTP_TRACKER_T) > 0) {

    printf("\n[%s/%d] WAITTRACKER: Received file table\n%s\n", __FILE__, __LINE__, 
        tracker_info->file_table);

    if (pthread_mutex_lock(&file_list_lock)) {
      perror("\tpthread_mutex_lock\n");
      return;
    }

    if (!globalFileListHead) {
      collection = (file_collection_t*)calloc(sizeof(file_collection_t), sizeof(char));
      collection->new_head = deserializeFileList(tracker_info->file_table);
      collection->modified_head = NULL;
      collection->deleted_head = NULL;
      collection->common_files = NULL;
      collection->renamed_head = NULL;
    }
    else {
      collection = getDifferences(tracker_info->file_table, globalFileListHead);
    }

    if (collection) {

      if(collection->common_files)
        do_version_comparisons_commmon(&collection->common_files);
      if(collection->deleted_head)
        do_version_comparisons_deleted(&collection->deleted_head);
      if(collection->modified_head)
        do_version_comparisons_modified(&collection->modified_head);


      char* s = serializeFileList(globalFileListHead);

      if(!WHO_AM_I) //if not tracker write file
        write_config(s);

      if(s) free(s);

      applyDifferences(collection);



      free_linked_list(collection->deleted_head);
      free(collection);
      collection = NULL;
    }

    memset(tracker_info, 0, sizeof(ptp_tracker_t));
    pthread_mutex_unlock(&file_list_lock);

  }

  if(tracker_info){
    if(tracker_info->file_table)
      free(tracker_info->file_table);
    free(tracker_info);
  }
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  master_delete_thread
 *  Description:  Carries out deletion of the file in the passed in arg.
 * =====================================================================================
 */
void *master_delete_thread(void *arg)
{
  file_node_t *current = (file_node_t *) arg;
  char file_root_path[WALK_SIZE];
  char file_bu_path[WALK_SIZE];

  memset(file_root_path, 0, WALK_SIZE);
  memset(file_bu_path, 0, WALK_SIZE);

  strcpy(file_root_path, root_path);
  strcpy(file_bu_path, backup_path);

  memset(file_root_path + root_path_len, 0, WALK_SIZE - root_path_len);
  strcat(file_root_path, current->name);
  memset(file_bu_path + bu_path_len, 0, WALK_SIZE - bu_path_len);
  strcat(file_bu_path, current->name);

  // Verify that we are not currently downloading this file
  pthread_mutex_lock(&dl_peer_table_lock);
  while (in_peer_peer_t_list(&dlPeerTableHead, current->name, NULL, 0)) {
    pthread_mutex_unlock(&dl_peer_table_lock);
    sleep(CHECK_PEER_TABLE_INTERVAL);
    pthread_mutex_lock(&dl_peer_table_lock);
  }

  printf("\n[%s/%d] DELETING: Deleting %s!\n", __FILE__, __LINE__, file_root_path);

  // backup files will just be named by their checksum value within
  // a directory that is the actual file name
  if (current->type == FILE_T) {
    strcat(file_bu_path, "/");
    sprintf(file_bu_path + bu_path_len + strlen(current->name) + 1, "%llu", current->checksum);
  }

  if (mk_path(file_bu_path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH))
    printf("\n[%s/%d] DELETING: Unable to create backup_path %s!\n", __FILE__, __LINE__, file_bu_path);
  else
    printf("\n[%s/%d] DELETING: Created backup_path %s!\n", __FILE__, __LINE__, file_bu_path);

  if (current->type == FILE_T) {
    if (copy_file(file_bu_path, file_root_path)) 
      printf("\n[%s/%d] DELETING: Unable to backup file %s!\n", __FILE__, __LINE__, file_bu_path);
    else
      printf("\n[%s/%d] DELETING: Created backup file %s!\n", __FILE__, __LINE__, file_bu_path);
  }

  if (remove(file_root_path)) 
    printf("\n[%s/%d] DELETING: Unable to delete file %s!\n", __FILE__, __LINE__, file_root_path);
  else
    printf("\n[%s/%d] DELETING: Deleted file %s!\n", __FILE__, __LINE__, file_root_path);

  pthread_mutex_unlock(&dl_peer_table_lock);
  free_file_node_t(current);

  pthread_exit(NULL);
}		/* -----  end of function master_delete_thread  ----- */

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  master_new_dl_thread
 *  Description:  Carries out download of new file with parameters in passed 
 *  in arg.
 * =====================================================================================
 */
void *master_new_dl_thread(void *arg)
{
  file_node_t *current                = (file_node_t *) arg;
  FILE *fp                            = NULL;
  uint64_t start_pos                  = 0;
  uint64_t length                     = 0;
  uint32_t idx                        = 0;
  uint32_t jdx                        = 0;
  uint32_t num_live_peers             = 0;
  int peer_sockfds[current->numPeers];
  char file_root_path[WALK_SIZE];
  char my_ip[WALK_SIZE];

  memset(my_ip, 0, WALK_SIZE);
  memset(file_root_path, 0, WALK_SIZE);

  strcpy(file_root_path, root_path);
  get_my_ip(my_ip);

  memset(file_root_path + root_path_len, 0, WALK_SIZE - root_path_len);
  strcat(file_root_path, current->name);

  printf("\n[%s/%d] NEW_DOWNLOAD: Beginning a new download for %s!\n", __FILE__, 
      __LINE__, current->name);

  if (current->type == DIR_T)
    strcat(file_root_path, "/");

  if (mk_path(file_root_path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)) {
    printf("\n[%s/%d] NEW_DOWNLOAD: Unable to create new file path %s!\n", __FILE__, 
        __LINE__, file_root_path);
    free_file_node_t(current);

    pthread_mutex_lock(&dl_num_lock);
    num_dl_threads--;
    pthread_mutex_unlock(&dl_num_lock);

    pthread_exit(NULL);
  }

  printf("\n[%s/%d] NEW_DOWNLOAD: Created new file path %s!\n", __FILE__, __LINE__, 
      file_root_path);

  // in the case we're dealing with a new file
  if (current->type == FILE_T) {

    // make sure we're not in the ip list
    for (idx = 0; idx < current->numPeers; idx++) {
      if (!strcmp(current->newpeerip[idx], my_ip)) {
        printf("\n[%s/%d] NEW_DOWNLOAD: Peer already has the most recent revision of this "
            "file %s!\n", __FILE__, __LINE__, file_root_path);
        free_file_node_t(current);

        pthread_mutex_lock(&dl_num_lock);
        num_dl_threads--;
        pthread_mutex_unlock(&dl_num_lock);

        pthread_exit(NULL);
      }
    }

    // create the file for writing, truncate its size to the size
    // of the most recent revision
    if (!(fp = fopen(file_root_path, "w"))) {
      printf("\n[%s/%d] NEW_DOWNLOAD: Unable to create %s for writing!\n", __FILE__, __LINE__, 
          file_root_path);
      free_file_node_t(current);

      pthread_mutex_lock(&dl_num_lock);
      num_dl_threads--;
      pthread_mutex_unlock(&dl_num_lock);

      pthread_exit(NULL);
    }

    printf("\n[%s/%d] NEW_DOWNLOAD: Created %s for writing!\n", __FILE__, __LINE__, 
        file_root_path);

    fclose(fp);

    // If the size is nonzero
    if (current->size) {

      // truncate
      truncate(file_root_path, current->size);

      printf("\n[%s/%d] NEW_DOWNLOAD: Truncated %s to %u!\n", __FILE__, __LINE__, file_root_path, 
          current->size);

      /*-----------------------------------------------------------------------------
       *  Determine the number of alive peers.
       *-----------------------------------------------------------------------------*/
      for (idx = 0; idx < current->numPeers; idx++) {
        peer_sockfds[idx] = -1;
        if ((peer_sockfds[idx] = connect_to_server(P2P_PORT, current->newpeerip[idx])) < 0) {
          printf("\n[%s/%d] NEW_DOWNLOAD: Peer with ip %s is offline!\n", __FILE__, __LINE__, 
              current->newpeerip[idx]);
          peer_sockfds[idx] = -1;
        }
        else{

          pthread_mutex_lock(&dl_peer_table_lock);
          if (in_peer_peer_t_list(&dlPeerTableHead, current->name, current->newpeerip[idx], current->checksum)) {

            printf("\n[%s/%d] NEW_DOWNLOAD: Peer with ip %s is online, but we are either dling %s from "
                "it already or we are dling a different version of %s.\n", __FILE__, __LINE__, current->newpeerip[idx], 
                current->name, current->name);

            pthread_mutex_unlock(&dl_peer_table_lock);
            close(peer_sockfds[idx]);
            peer_sockfds[idx] = -1;
            continue;
          }
          pthread_mutex_unlock(&dl_peer_table_lock);

          printf("\n[%s/%d] NEW_DOWNLOAD: Peer with ip %s is online!\n", __FILE__, __LINE__, 
              current->newpeerip[idx]);
          num_live_peers++;
        }
      }

      if (!num_live_peers) {
        printf("\n[%s/%d] NEW_DOWNLOAD: None of the peers who are supposed to have %s's "
            "most recent revision are alive!\n", __FILE__, __LINE__, 
            file_root_path);

        remove(file_root_path);
        pthread_mutex_lock(&dl_num_lock);
        num_dl_threads--;
        pthread_mutex_unlock(&dl_num_lock);

        free_file_node_t(current);
        pthread_exit(NULL);

      }

      pthread_t dl_threads[num_live_peers];

      // calculate the piece length, ceiling division
      length = 1 + ((current->size - 1) / num_live_peers);

      printf("\n[%s/%d] NEW_DOWNLOAD: The number of alive peers with the most recent rev "
          "of %s is %d. The file size is %u, so the piece length is %llu.\n", __FILE__, __LINE__, 
          current->name, num_live_peers, current->size, length);

      for (idx = 0; idx < current->numPeers; idx++) {

        if (peer_sockfds[idx] == -1)
          continue;

        // if we're dealing with the last peer, it's possible that we may be
        // requesting too much data, therefore only request the remaining
        // data
        if ((start_pos + length) > current->size)
          length = current->size - start_pos;

        // TODO: check that this is the checksum we want

        // create the download request and start the dl threads
        dl_req_t *new = new_dl_req_t(start_pos, 65536,
            length, current->name, current->newpeerip[idx]);

        dl_arg_t *info = calloc(1, sizeof(dl_arg_t));
        memset((char *) info, 0, sizeof(dl_arg_t));
        info->new_checksum = current->checksum;
        info->dl_req       = new;
        info->port         = peer_sockfds[idx];

        printf("\n[%s/%d] NEW_DOWNLOAD: Starting a download for piece from file %s "
            "with start pos %llu and length %llu from peer ip %s. The total file size is "
            "%u.\n", __FILE__, __LINE__, new->file_name, new->start_pos, new->length, new->peer_ip, 
            current->size);

        pthread_mutex_lock(&dl_num_lock);
        num_dl_threads++;
        pthread_mutex_unlock(&dl_num_lock);

        pthread_create(&dl_threads[jdx], NULL, P2P_download_thread, (void *) info);
        jdx++;
        start_pos += length;
      }

      //for (jdx = 0; jdx < num_live_peers; jdx++) 
      //(void) pthread_join(dl_threads[jdx], NULL);
    }
  }

  //printf("\n[%s/%d] NEW_DOWNLOAD: Finished downloading %s.\n", __FILE__, __LINE__, file_root_path);

  // TODO: WE CAN CHECK THE CHECKSUM HERE, IF NOT EQUAL TO CURRENT->CHECKSUM, WE DELETE THIS FILE BECAUSE WE DON'T
  // HAVE THE PREVIOUS REVISION

  pthread_mutex_lock(&dl_num_lock);
  num_dl_threads--;
  pthread_mutex_unlock(&dl_num_lock);

  free_file_node_t(current);
  pthread_exit(NULL);
}		/* -----  end of function master_new_dl_thread  ----- */

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  master_mod_dl_thread
 *  Description:  Carries out modification of file with paramters passed in.
 * =====================================================================================
 */
void *master_mod_dl_thread(void *arg)
{
  file_node_t *current                = (file_node_t *) arg;
  uint64_t start_pos                  = 0;
  uint64_t length                     = 0;
  uint32_t idx                        = 0;
  uint32_t jdx                        = 0;
  uint32_t num_live_peers             = 0;
  int peer_sockfds[current->numPeers];
  char file_root_path[WALK_SIZE];
  char file_bu_path[WALK_SIZE];
  char my_ip[WALK_SIZE];

  memset(my_ip, 0, WALK_SIZE);
  memset(file_root_path, 0, WALK_SIZE);
  memset(file_bu_path, 0, WALK_SIZE);

  strcpy(file_root_path, root_path);
  strcpy(file_bu_path, backup_path);
  get_my_ip(my_ip);

  memset(file_root_path + root_path_len, 0, WALK_SIZE - root_path_len);
  strcat(file_root_path, current->name);

  memset(file_bu_path + bu_path_len, 0, WALK_SIZE - bu_path_len);
  strcat(file_bu_path, current->name); 

  printf("\n[%s/%d] MOD_DOWNLOAD: Downloading deltas for %s.\n", __FILE__, 
      __LINE__, file_root_path);

  // in the case we're dealing with a modified file
  if (current->type == FILE_T) {

    // make sure we're not in the ip list
    for (idx = 0; idx < current->numPeers; idx++) {
      if (!strcmp(current->newpeerip[idx], my_ip)) {
        printf("\n[%s/%d] MOD_DOWNLOAD: Peer already has the most recent revision of this "
            "file %s!\n", __FILE__, __LINE__, file_root_path);
        free_file_node_t(current);

        pthread_mutex_lock(&dl_num_lock);
        num_dl_threads--;
        pthread_mutex_unlock(&dl_num_lock);

        pthread_exit(NULL);
      }
    }

    // again, we need to create the backup path using the chksum
    strcat(file_bu_path, "/");
    sprintf(file_bu_path + bu_path_len + strlen(current->name) + 1, "%llu",
        get_local_file_chksum(globalFileListHead, current->name));

    // create the backup path
    if (mk_path(file_bu_path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)) {
      printf("\n[%s/%d] MOD_DOWNLOAD: Unable to create backup_path %s.\n", __FILE__, __LINE__, 
          file_bu_path);
      free_file_node_t(current);

      pthread_mutex_lock(&dl_num_lock);
      num_dl_threads--;
      pthread_mutex_unlock(&dl_num_lock);

      pthread_exit(NULL);
    }

    printf("\n[%s/%d] MOD_DOWNLOAD: Created backup_path %s.\n", __FILE__, __LINE__, 
        file_bu_path);

    // copy the file over
    if (copy_file(file_bu_path, file_root_path)) {
      printf("\n[%s/%d] MOD_DOWNLOAD: Unable to create backup version %s.\n", __FILE__, __LINE__, 
          file_bu_path);
      free_file_node_t(current);

      pthread_mutex_lock(&dl_num_lock);
      num_dl_threads--;
      pthread_mutex_unlock(&dl_num_lock);

      pthread_exit(NULL);
    }

    printf("\n[%s/%d] MOD_DOWNLOAD: Created backup version %s.\n", __FILE__, __LINE__, 
        file_bu_path);

    if (current->size) {
      truncate(file_root_path, current->size);

      printf("\n[%s/%d] MOD_DOWNLOAD: Truncated %s to %u.\n", __FILE__, __LINE__, file_root_path, 
          current->size);

      /*-----------------------------------------------------------------------------
       *  Find out which peers are actually alive.
       *-----------------------------------------------------------------------------*/
      for (idx = 0; idx < current->numPeers; idx++) {

        peer_sockfds[idx] = -1;
        if ((peer_sockfds[idx] = connect_to_server(P2P_PORT, current->newpeerip[idx])) < 0) {
          printf("\n[%s/%d] MOD_DOWNLOAD: Peer with ip %s is offline!\n", __FILE__, __LINE__, 
              current->newpeerip[idx]);
          peer_sockfds[idx] = -1;
        }
        else {

          pthread_mutex_lock(&dl_peer_table_lock);
          if (in_peer_peer_t_list(&dlPeerTableHead, current->name, current->newpeerip[idx], current->checksum)) {

            printf("\n[%s/%d] MOD_DOWNLOAD: Peer with ip %s is online, but we are either dling %s from "
                "it already or we are dling a different version of %s.\n", __FILE__, __LINE__, current->newpeerip[idx], 
                current->name, current->name);

            pthread_mutex_unlock(&dl_peer_table_lock);
            close(peer_sockfds[idx]);
            peer_sockfds[idx] = -1;
            continue;
          }
          pthread_mutex_unlock(&dl_peer_table_lock);

          printf("\n[%s/%d] MOD_DOWNLOAD: Peer with ip %s is online!\n", __FILE__, __LINE__, 
              current->newpeerip[idx]);
          num_live_peers++;
        }
      }

      // make sure there are live peers
      if (!num_live_peers) {
        printf("\n[%s/%d] MOD_DOWNLOAD: None of the peers who are supposed to have %s's "
            "most recent revision are alive or are valid download targets! "
            "Restoring my own backup %s!\n", __FILE__, __LINE__, file_root_path, file_bu_path);

        remove(file_root_path);
        copy_file(file_root_path, file_bu_path);
        pthread_mutex_lock(&dl_num_lock);
        num_dl_threads--;
        pthread_mutex_unlock(&dl_num_lock);

        free_file_node_t(current);
        pthread_exit(NULL);

      }

      pthread_t dl_threads[num_live_peers];

      // calculate the piece length, ceiling division
      length = 1 + ((current->size - 1) / num_live_peers);

      printf("\n[%s/%d] MOD_DOWNLOAD: The number of alive peers with the most recent rev "
          "of %s is %d. The file size is %u, so the piece length is %llu.\n", __FILE__, __LINE__, 
          current->name, num_live_peers, current->size, length);

      for (idx = 0; idx < current->numPeers; idx++) {

        if (peer_sockfds[idx] == -1)
          continue;

        // if we're dealing with the last peer, it's possible that we may be
        // requesting too much data, therefore only request the remaining
        // data
        if ((start_pos + length) > current->size)
          length = current->size - start_pos;

        // TODO: check that this is the checksum we want

        // create the download request and start the dl threads
        dl_req_t *new = new_dl_req_t(start_pos, get_local_file_chksum(globalFileListHead, current->name), 
            length, current->name, current->newpeerip[idx]);

        dl_arg_t *info = calloc(1, sizeof(dl_arg_t));
        memset((char *) info, 0, sizeof(dl_arg_t));
        info->new_checksum = current->checksum;
        info->dl_req       = new;
        info->port         = peer_sockfds[idx];

        printf("\n[%s/%d] MOD_DOWNLOAD: Starting a download for piece from file %s "
            "with start pos %llu and length %llu from peer ip %s. The total file size is "
            "%u.\n", __FILE__, __LINE__, new->file_name, new->start_pos, new->length, new->peer_ip, 
            current->size);

        pthread_mutex_lock(&dl_num_lock);
        num_dl_threads++;
        pthread_mutex_unlock(&dl_num_lock);

        pthread_create(&dl_threads[jdx], NULL, P2P_download_thread, (void *) info);
        jdx++;
        start_pos += length;
      }

      //for (jdx = 0; jdx < num_live_peers; jdx++) 
      //(void) pthread_join(dl_threads[jdx], NULL);
    }
  }

  //printf("\n[%s/%d] MOD_DOWNLOAD: Finished downloading %s deltas.\n", __FILE__, __LINE__, file_root_path);
  // TODO: WE CAN CHECK THE CHECKSUM HERE, IF NOT EQUAL TO CURRENT->CHECKSUM, 
  // WE GRAB OUR RECENT REVISION

  pthread_mutex_lock(&dl_num_lock);
  num_dl_threads--;
  pthread_mutex_unlock(&dl_num_lock);

  free_file_node_t(current);
  pthread_exit(NULL);
}		/* -----  end of function master_mod_dl_thread  ----- */

void applyDifferences(file_collection_t *collection) {
  // remove all the deleted files/directories, do so by moving to the end
  // of the deleted link list and delete backwards
  file_node_t *current = collection->deleted_head;
  int num_deletes      = 0;
  int idx              = 0;
  pthread_t delete_threads[1000];

  if (current) {
    pthread_mutex_lock(&dl_num_lock);
    while (current) {

      // Execute all the deletions atomically.
      printf("\n[%s/%d]: Deletion to make!\n", __FILE__, __LINE__);

      file_node_t *copy = create_file_node_t(current->name, NULL, 0, current->type, 0, 0, current->checksum);
      pthread_create(&delete_threads[idx], NULL, master_delete_thread, (void *) copy);
      idx++;

      current = current->next;
      num_deletes++;
    }

    for (idx = 0; idx < num_deletes; idx++) 
      (void) pthread_join(delete_threads[idx], NULL);

    pthread_mutex_unlock(&dl_num_lock);
  }

  // get the new files
  for (current = collection->new_head; current != NULL; current = current->next) {

    pthread_mutex_lock(&dl_num_lock);
    num_dl_threads++;
    pthread_mutex_unlock(&dl_num_lock);

    file_node_t *copy = create_file_node_t(current->name, current->newpeerip, current->numPeers, current->type, 
        current->size, current->version_number, current->checksum);
    pthread_t new_thread;
    pthread_create(&new_thread, NULL, master_new_dl_thread, (void *) copy);
  }

  // get the modified files
  for (current = collection->modified_head; current != NULL; current = current->next) {

    pthread_mutex_lock(&dl_num_lock);
    num_dl_threads++;
    pthread_mutex_unlock(&dl_num_lock);

    file_node_t *copy = create_file_node_t(current->name, current->newpeerip, current->numPeers, current->type, 
        current->size, current->version_number, current->checksum);
    pthread_t mod_thread;
    pthread_create(&mod_thread, NULL, master_mod_dl_thread, (void *) copy);
  }

}

/*
Description:
1) Monitor a local file directory
2) Send out updated file table to the tracker if any changes occur
*/

void *file_monitor_thread(void *arg) {

  // TODO: monitor files

  printf("file monitor initialized\n");

  set_tracker_con(tracker_conn);
  monitor_files();

  pthread_exit(NULL);
}

/*
Description:
1) Listen on the P2P port
2) Receive download requests from other peers
3) Create appropriate P2P Upload threads
*/
void *P2P_listening_thread(void *arg) {
  struct sockaddr_in client_addr;
  socklen_t client_size = sizeof(client_addr);
  int listen_fd;
  int conn_fd;

  if ((listen_fd = create_listening_socket(P2P_PORT, SOCK_STREAM)) == -1) {
    LOG(stderr, "Unable to create P2P listening socket.");
    pthread_exit(NULL);
  }

  fprintf(stdout, "P2P_listening_thread listening on %d.\n", P2P_PORT);

  for ( ; ; ) {
    memset((char *) &client_addr, 0, client_size);

    if ((conn_fd = accept(listen_fd, (struct sockaddr *) &client_addr, &client_size)) < 0)
      continue;

    fprintf(stdout, "Received a download request from peer with ip %s on port %d\n.",
        inet_ntoa(client_addr.sin_addr), conn_fd);

    int *pass_fd = (int *) malloc(sizeof(int));
    *pass_fd     = conn_fd;
    pthread_t P2P_upload;
    pthread_create(&P2P_upload, NULL, P2P_upload_thread, (void*) pass_fd);
  }

  pthread_exit(NULL);
}

/*
Description: this thread sends a heartbeat message to the tracker every KEEP_ALIVE_TIMEOUT_PERIOD.
*/
void *alive_thread(void *timeout_arg) {
  ptp_peer_t heartbeat;
  memset((char *) &heartbeat, 0, sizeof(ptp_peer_t));
  heartbeat.type = KEEP_ALIVE;
  get_my_ip(heartbeat.peer_ip);
  heartbeat.port = find_port(tracker_conn);

  while (1) {
    if (send_struct(tracker_conn, &heartbeat, PTP_PEER_T) == 0)
      printf("Sent heartbeat\n");
    sleep(heartbeat_interval);
  }

  pthread_exit(NULL);
}

/*
Description: this thread downloads data from a remote peer
Parameters: peer information, file segment being downloaded
*/
void *P2P_download_thread(void *download_arg) {
  dl_arg_t *arg            = (dl_arg_t *) download_arg;
  dl_req_t *req            = (dl_req_t *) arg->dl_req;
  ul_resp_t *resp          = new_ul_resp_t(0, 0, NULL);
  ul_resp_t *current       = NULL;
  FILE *fp                 = NULL;
  int conn_fd              = arg->port;
  char file_root_path[WALK_SIZE];
  memset(file_root_path, 0, WALK_SIZE);

  printf("\n[%s/%d] DOWNLOADING: Dling file %s's delta for piece starting at %llu with length %llu "
      "from peer with ip %s.",  __FILE__, __LINE__, req->file_name, req->start_pos, req->length, req->peer_ip);

  if (req->req_ver == 65536) 
    printf(" I don't have this file, so this is a NEW file download. "
        "The peer has revision %llu.\n", arg->new_checksum);
  else
    printf(" I have rev %llu, so this is a MODIFY file download. "
        "The peer has revision %llu.\n", req->req_ver, arg->new_checksum);

  // send the file request
  if (send_struct(conn_fd, req, DL_REQUEST) < 0) {
    LOG(stderr, "ERROR: Unable to send download request to peer!");

    pthread_mutex_lock(&dl_peer_table_lock);
    remove_peer_peer_t(&dlPeerTableHead, req->file_name, req->peer_ip);
    pthread_mutex_unlock(&dl_peer_table_lock);

    pthread_mutex_lock(&dl_num_lock);
    num_dl_threads--;
    pthread_mutex_unlock(&dl_num_lock);

    close_socket(conn_fd, TRUE);
    free_dl_req_t(req);
    free(arg);
    pthread_exit(NULL);
  }

  printf("\n[%s/%d] DOWNLOADING: Sent request for %s's delta for piece starting at %llu with length %llu "
      "from peer with ip %s sent.\n", __FILE__, __LINE__, req->file_name, req->start_pos, req->length, req->peer_ip);

  // receive the diff linked list
  if (receive_struct(conn_fd, resp, UL_RESPONSE) < 0) {
    LOG(stderr, "ERROR: Unable to receive upload response from peer!");

    pthread_mutex_lock(&dl_peer_table_lock);
    remove_peer_peer_t(&dlPeerTableHead, req->file_name, req->peer_ip);
    pthread_mutex_unlock(&dl_peer_table_lock);

    pthread_mutex_lock(&dl_num_lock);
    num_dl_threads--;
    pthread_mutex_unlock(&dl_num_lock);

    close_socket(conn_fd, TRUE);
    free_dl_req_t(req);
    free_ul_resp_t(resp);
    free(arg);
    pthread_exit(NULL);
  }

  close_socket(conn_fd, TRUE);

  strcpy(file_root_path, root_path);
  strcat(file_root_path, req->file_name);

  // attempt to open the file in update mode
  if ((fp = fopen(file_root_path, "rb+")) == NULL) {
    LOG(stderr, "ERROR: Unable to open the file to be updated!");
    pthread_mutex_lock(&dl_peer_table_lock);
    remove_peer_peer_t(&dlPeerTableHead, req->file_name, req->peer_ip);
    pthread_mutex_unlock(&dl_peer_table_lock);

    pthread_mutex_lock(&dl_num_lock);
    num_dl_threads--;
    pthread_mutex_unlock(&dl_num_lock);

    free_dl_req_t(req);
    free_ul_resp_t(resp);
    free(arg);
    pthread_exit(NULL);
  }

  // move through the diff linked list, write each diff to the correct
  // position in the file
  current = resp;
  while (current) {
    if (!current->length) {
      current = current->next;
      continue;
    }

    printf("\n[%s/%d] DOWNLOADING: Received a delta piece for file %s, rev %llu, from peer with ip %s."
        "This delta piece starts at %llu and has length %llu.\n", __FILE__, __LINE__, req->file_name, 
        arg->new_checksum, req->peer_ip, resp->start_pos, resp->length);

    if (fseek(fp, current->start_pos, SEEK_SET)) {
      LOG(stderr, "ERROR: Failure while applying diff!");
      fclose(fp);
      pthread_mutex_lock(&dl_peer_table_lock);
      remove_peer_peer_t(&dlPeerTableHead, req->file_name, req->peer_ip);
      pthread_mutex_unlock(&dl_peer_table_lock);

      pthread_mutex_lock(&dl_num_lock);
      num_dl_threads--;
      pthread_mutex_unlock(&dl_num_lock);

      free_dl_req_t(req);
      free_ul_resp_t(resp);
      free(arg);
      pthread_exit(NULL);
    }

    if (fwrite(current->diff, sizeof(char), current->length, fp) != current->length) {
      LOG(stderr, "ERROR: Failure while applying diff!");
      fclose(fp);
      pthread_mutex_lock(&dl_peer_table_lock);
      remove_peer_peer_t(&dlPeerTableHead, req->file_name, req->peer_ip);
      pthread_mutex_unlock(&dl_peer_table_lock);

      pthread_mutex_lock(&dl_num_lock);
      num_dl_threads--;
      pthread_mutex_unlock(&dl_num_lock);

      free_dl_req_t(req);
      free_ul_resp_t(resp);

      free(arg);
      pthread_exit(NULL);
    }

    printf("\n[%s/%d] DOWNLOADING: Applied this diff.\n", __FILE__, __LINE__);

    current = current->next;
  }

  // cleanup
  fclose(fp);
  pthread_mutex_lock(&dl_peer_table_lock);
  remove_peer_peer_t(&dlPeerTableHead, req->file_name, req->peer_ip);
  pthread_mutex_unlock(&dl_peer_table_lock);

  pthread_mutex_lock(&dl_num_lock);
  num_dl_threads--;
  pthread_mutex_unlock(&dl_num_lock);

  free_dl_req_t(req);
  free_ul_resp_t(resp);
  free(arg);

  pthread_exit(NULL);
}

/*
Description: this thread uploads data to a remote peer
Parameters: peer information, file segment to send
*/
void *P2P_upload_thread(void *upload_arg) {
  dl_req_t *req            = new_dl_req_t(0, 0, 0, NULL, NULL);
  ul_resp_t *head          = NULL;
  FILE *curr               = NULL;
  FILE *old                = NULL;
  char *curr_buff          = NULL;
  char *old_buff           = NULL;
  char *buff               = NULL;
  struct stat st;
  uint64_t old_size        = 0;
  uint64_t idx             = 0;
  uint64_t diff_pos_bgn    = 0;
  uint64_t diff_pos_end    = 0;
  int conn_fd              = *((int *) upload_arg);
  char file_bu_path[WALK_SIZE];
  char file_root_path[WALK_SIZE];

  memset(file_bu_path, 0, WALK_SIZE);
  memset(file_root_path, 0, WALK_SIZE);

  free(upload_arg);

  // receive the download request
  if (receive_struct(conn_fd, req, DL_REQUEST) < 0) {
    LOG(stderr, "ERROR: Unable to receive download request.");
    free_dl_req_t(req);
    pthread_exit(NULL);
  }

  if (req->req_ver == 65536) 
    printf("\n[%s/%d] UPLOADING: Download request is for most recent rev of file %s "
        "starting at %llu with length %llu.\n", __FILE__, __LINE__, req->file_name, 
        req->start_pos, req->length);
  else
    printf("\n[%s/%d] UPLOADING: Download request is for file %s's delta between the most recent revision "
        " and rev %llu starting at %llu with length %llu.\n", 
        __FILE__, __LINE__, req->file_name, req->req_ver, req->start_pos, req->length);

  strcpy(file_root_path, root_path);
  strcat(file_root_path, req->file_name);


  // attempt to open the most recent version of the file
  if (!(curr = fopen(file_root_path, "rb"))) {
    LOG(stderr, "ERROR: Unable to open the file to be updated!");
    free_dl_req_t(req);
    pthread_exit(NULL);
  }

  // get to the start position
  if (fseek(curr, req->start_pos, SEEK_SET)) {
    LOG(stderr, "ERROR: Failure while creating diff!");
    fclose(curr);
    free_dl_req_t(req);
    pthread_exit(NULL);
  }
  if (!req->length) {
    free_dl_req_t(req);
    free(curr_buff);
    pthread_exit(NULL);
  }
  curr_buff = calloc(req->length, sizeof(char));
  MALLOC_CHECK(stderr, curr_buff);

  // write in length characters into the buffer
  if (fread(curr_buff, sizeof(char), req->length, curr) != req->length) {
    LOG(stderr, "ERROR: Failure while creating diff!");
    fclose(curr);
    free_dl_req_t(req);
    free(curr_buff);
    pthread_exit(NULL);
  }

  fclose(curr);

  // attempt to open the revision that the downloader has
  strcpy(file_bu_path, backup_path);
  strcat(file_bu_path, req->file_name);
  strcat(file_bu_path, "/");
  sprintf(file_bu_path + bu_path_len + strlen(req->file_name) + 1, "%llu",
      req->req_ver);

  // in this case, we don't have the requester's revision in our backup, so 
  // we can't do a diff, so we'll send everything from the start to the end
  if (req->req_ver == 65536 || (old = fopen(file_bu_path, "rb")) == NULL) {
    head = new_ul_resp_t(req->start_pos, req->length, curr_buff);

    printf("\n[%s/%d] UPLOADING: We don't have rev %llu in our backup. Sending the entire piece "
        "starting at %llu with length %llu.\n", __FILE__, __LINE__, req->req_ver, head->start_pos, 
        head->length);
  }
  // in this case, we do have the requester's revision in our backup
  else {
    printf("\n[%s/%d] UPLOADING: We have rev %llu in our backup.\n", 
        __FILE__, __LINE__, req->req_ver);

    // calculate the size of the old version
    fstat(fileno(old), &st);
    old_size = st.st_size;

    // if the size of the old version is greater than or equal
    // to the req start position, we can actually check for a diff
    if (old_size > req->start_pos) {

      // get to the start position in the old file
      if (fseek(old, req->start_pos, SEEK_SET)) {
        LOG(stderr, "ERROR: Failure while creating diff!");
        free_dl_req_t(req);
        free(curr_buff);
        fclose(old);
        pthread_exit(NULL);
      }
      if (!old_buff) {
        free_dl_req_t(req);
        free(curr_buff);
        free(old_buff);
        fclose(old);
        pthread_exit(NULL);
      }
      old_buff = calloc(req->length, sizeof(char));
      MALLOC_CHECK(stderr, old_buff);

      // write in up to length characters into the buffer
      if (fread(old_buff, sizeof(char), req->length, old) <= 0) {
        LOG(stderr, "ERROR: Failure while creating diff!");
        free_dl_req_t(req);
        free(curr_buff);
        free(old_buff);
        fclose(old);
        pthread_exit(NULL);
      }

      fclose(old);

      // create a dummy head buffer
      head = new_ul_resp_t(req->start_pos, 0, NULL);

      // go through the old version buffer and the current version buffer
      idx = 0;
      while (idx < req->length) {
        if (old_buff[idx] == curr_buff[idx]) {
          idx++;
          continue;
        }

        // create upload request nodes for consecutive diffs
        // find the beginning and end position of the consecutive diff
        diff_pos_bgn = idx;
        diff_pos_end = idx;
        while (++idx < req->length && old_buff[idx] != curr_buff[idx])
          diff_pos_end = idx;

        buff = calloc(diff_pos_end - diff_pos_bgn + 1, sizeof(char));
        MALLOC_CHECK(stderr, buff);
        memcpy(buff, curr_buff + diff_pos_bgn, diff_pos_end - diff_pos_bgn + 1);

        // create a new node with that diff information
        add_ul_resp_t(head, new_ul_resp_t(req->start_pos + diff_pos_bgn,
              diff_pos_end - diff_pos_bgn + 1, buff));

        printf("\n[%s/%d] UPLOADING: Delta for file %s between most recent rev and rev %llu "
            "exists! Starting pos: %llu, length: %llu.\n", __FILE__, __LINE__, 
            req->file_name, req->req_ver, req->start_pos + diff_pos_bgn, diff_pos_end - diff_pos_bgn + 1);

        free(buff);
        buff = NULL;
      }
    }
    else {
      fclose(old);
      head = new_ul_resp_t(req->start_pos, req->length, curr_buff);
      printf("\n[%s/%d] UPLOADING: We have rev %llu in our backup; however, the requester is "
          "asking for a delta starting at a position greater than the length of this previous revision. "
          "This happens because the most recent revision has a greater length than its predecessor. "
          "Sending the entire piece starting at %llu with length %llu.\n", 
          __FILE__, __LINE__, req->req_ver, head->start_pos, head->length);
    }
  }

  // send the file diff data
  if (send_struct(conn_fd, head, UL_RESPONSE) < 0) {
    LOG(stderr, "ERROR: Unable to send upload response to peer!");
    free_dl_req_t(req);
    free_ul_resp_t(head);
    free(curr_buff);
    free(old_buff);
    pthread_exit(NULL);
  }

  printf("\n[%s/%d] UPLOADING: File %s's delta between the most recent revision "
      " and rev %llu starting at %llu and ending at %llu has been sent.\n", 
      __FILE__, __LINE__, req->file_name, req->req_ver, req->start_pos, req->start_pos + req->length);

  // cleanup
  free_dl_req_t(req);
  free_ul_resp_t(head);
  free(curr_buff);
  free(old_buff);
  pthread_exit(NULL);
}

/*
 Description: logs information to a file in the home folder
 Parameters: none
 */
void *logging_thread(void *arg) {
	printf("init");
	while (1) {
		pthread_mutex_lock(&stats_lock);
		struct passwd *pwd = getpwuid(getuid());
		char filename[WALK_SIZE];
		sprintf(filename, "%s/.DartSync/.statslog", pwd->pw_dir);
		time(&updatetime);
		char buffer[200];
		memset(buffer, 0, 200);
		sprintf(buffer, "%lu\n%lu\n%lu\n%lu\n%lu\n%lu\n%d\n%d", duncompress, tuncompress, dcompress, tcompress, dencrypt, tencrypt, (int)starttime, (int)updatetime);
		duncompress = 0;
		dcompress = 0;
		dencrypt = 0;
		FILE *f = fopen(filename, "w");
		fwrite(buffer, sizeof(char), sizeof(buffer), f);
		fclose(f);
		pthread_mutex_unlock(&stats_lock);
		printf("Logged status to %s\n", filename);
		sleep(5);
	}
	pthread_exit(NULL);
}

/*
Description: frees memory and closes connections when SIGINT signal occurs
*/
void cleanup(int arg) {
  ptp_peer_t *quit_struct = calloc(1, sizeof(ptp_peer_t));
  get_my_ip(quit_struct->peer_ip);
  quit_struct->type = CLOSE;

  if (send_struct(tracker_conn, quit_struct, PTP_PEER_T) < 0) {
    LOG(stderr, "Couldn't send quit message to tracker!");
    close(tracker_conn);
    close(P2P_conn);
    free(quit_struct);
    free_linked_list(globalFileListHead);
    free_dl_table(dlPeerTableHead);
    free_ul_table(ulPeerTableHead);
    exit(EXIT_SUCCESS);
  }

  close(tracker_conn);
  close(P2P_conn);
  free(quit_struct);

  printf("\n[%s/%d]: Quitting peer. Freeing memory and closing connections.\n", __FILE__, __LINE__);

  if (globalFileListHead)
    free_linked_list(globalFileListHead);
  if (dlPeerTableHead)
    free_dl_table(dlPeerTableHead);

  exit(EXIT_SUCCESS);
}

/*
 * ===  FUNCTION  ======================================================================
 *         Name:  getDartSyncPath
 *  Description:  Gets the absolute pathname for DartSync. /Users/UID/DartSync
 *  Returns strlen.
 * =====================================================================================
 */
unsigned long getDartSyncPath(char *string)
{
  struct passwd *pwd = getpwuid(getuid());
  sprintf(string, "%s/DartSync/", pwd->pw_dir);

  return strlen(string);
}		/* -----  end of function getDartSyncPath  ----- */

/*
 * ===  FUNCTION  ======================================================================
 *         Name:  getDartSyncBackUpPath
 *  Description:  Gets the absolute pathname for DartSync Backup.
 *  /Users/UID/.DartSync Returns strlen.
 * =====================================================================================
 */
unsigned long getDartSyncBackUpPath(char *string)
{
  struct passwd *pwd = getpwuid(getuid());
  sprintf(string, "%s/.DartSync/", pwd->pw_dir);

  return strlen(string);
}		/* -----  end of function getDartSyncBackUpPath  ----- */

void free_dl_table(peer_side_peer_t *head) {
  peer_side_peer_t *current = head;
  while (current) {
    peer_side_peer_t *previous = current;
    current = current->next;
    free(previous->file_name);
    previous->file_name = NULL;
    free(previous);
    previous = NULL;
  }
}

void free_ul_table(peer_side_peer_t *head) {
  peer_side_peer_t *current = head;
  while (current) {
    peer_side_peer_t *previous = current;
    current = current->next;
    free(previous->file_name);
    previous->file_name = NULL;
    free(previous);
    previous = NULL;
  }
}

// ---------------- Helper Functions

// Parse received information from tracker

// Package download request into struct to send to peer

// Parse received download request struct from peer
