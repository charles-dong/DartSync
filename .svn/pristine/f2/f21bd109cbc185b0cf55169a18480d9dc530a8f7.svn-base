#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <pwd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <stdio.h>                          
#include <stdlib.h>

#include "../utils/socket_common.h"
#include "../utils/qs.h"
#include "../utils/common.h"
#include "files.h"
#include "../utils/data_structures.h"


// ---------------- Constants

#define WALK_SIZE 1024
#define STORAGE ".SUPER_DUPER_SECRET_STORAGE"
#define SHUS_CONSTANT 10 //arbitrary # to make sure we don't have off-by-one errors

// ---------------- Global Variables
int t_conn = -1;
int port = -1;

file_node_t* myCurrentHead = NULL;
extern pthread_mutex_t file_list_lock; // shared between peer and filewalker for modifying global file list
extern file_node_t* globalFileListHead;
extern pthread_t * dlPeerTableHead;
extern pthread_mutex_t dl_peer_table_lock;
extern char* root_path;
extern int WHO_AM_I; //0 if peer, 1 if peer
extern int num_dl_threads;
extern pthread_mutex_t dl_num_lock;


file_node_t* localFileListHead = NULL;
file_node_t* firstItemInDartSync;

// ---------------- Support Functions
int listdir(char *name, int level,file_node_t* object);
void squeeze_in(file_node_t* object, file_node_t* object_2 );
int has_colon(const char *s);
int get_integer(char* input);
void replace_char(char* s, char c);
void getDartSyncPath_fs(char* string);
long get_size_linked_list(file_node_t* other);
void getChecksumsNotInFirst(long* a, long size_a,  long* b, long size_b,  long* checksumNotInList);
void getFilesNotInFirst(char** a, long size_a, char** b, long size_b, long* fileNotInList);
long get_file_size(char* path);
long get_file_entry_count(char* s);
long get_longest_filename(file_node_t *head);
void get_string_at_index(char* s, long index, char* d);
void removeFromLinkedList(file_node_t** head_p, file_node_t* current);
unsigned short checksum(char* filename);
void partition(char*** list, char* filestring, long size);
void node_copy_notname(file_node_t* a,file_node_t* b);
void node_copy_name(file_node_t* a,file_node_t* b);
void getFilesInBoth(char** a, long size_a, char** b, long size_b, char** commonFiles);
void replace_first_null_with_colon(char* s, char r);
void remove_colon(char** x, long size);
void remove_null(char** x, long size);
void populate_a_using_indices_from_b_indexing_c(char** a, long* b, long size_b, char** c);
void parititionIntoTwo(char** a, long size_a, char** b, long size_b, long* c, long* d);
void getChecksumsNamesInBoth(char** a, long size_a, char** b, long size_b, long* commonFiles, char** commonNames);
void renameRenamedFiles(file_node_t* head_of_renamed);
void modifyModifiedFiles(file_node_t* head_of_mod);


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  set_tracker_con
 *  Description:  Sets globals: tracker connfd and home (DartSync Path)
 * =====================================================================================
 */
void set_tracker_con(int conn){
    t_conn = conn;
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  getDifferences
 *  Description:  Obtain which files are new, deleted, and modified/renamed. Received
 *  string is the string representation of a global file list we've received
 * =====================================================================================
 */
file_collection_t* getDifferences(char* received_file_table, file_node_t* our){
    
 
    /* Detect invalid head of linked list of our files */
    if(!(our && our->name && strlen(our->name))){
        LOG(stderr, "The global file list is null. Adding all new files.\n");
        file_collection_t* collection = (file_collection_t*)malloc(sizeof(file_collection_t));
        collection->deleted_head = NULL;
        collection->modified_head = NULL;
        collection->new_head = deserializeFileList(received_file_table);
        collection->renamed_head = NULL;
        collection->common_files = NULL;
        return collection;
    }

     /* Detect NULL string */
    if(!received_file_table){
        LOG(stderr, "The passed string is null\n");
        return NULL;
    }
    
    //setup
    long i = 0;
    long limit = 0;
    long other_size = get_file_entry_count(received_file_table); //count files in received string
    long our_size = get_size_linked_list(our); //count files in our list

    
    char** other_files = (char**)malloc(sizeof(char*)* other_size);
    char** newAndModNames = (char**)malloc(sizeof(char*)* other_size);
    char** newSoFar = (char**)malloc(sizeof(char*)*other_size);

    long* newAndMod = (long*)malloc(sizeof(long)* other_size);
    long* newFiles = (long*)malloc(sizeof(long)* other_size);
    long* commonFiles = (long*)malloc(sizeof(long) * other_size);
    
    long* modFiles = (long*)malloc(sizeof(long)* our_size);
    long* deletedFiles = (long*)malloc(sizeof(long)* our_size);
    long* renamedFiles = (long*)malloc(sizeof(long)* our_size);
    long* deletedAndRenamed = (long*)malloc(sizeof(long)* our_size);
    char** deletedAndRenamedNames = (char**)malloc(sizeof(char*)* our_size);
    char** our_files = (char**)malloc(sizeof(char*)* our_size);
    char** deletedSoFar = (char**)malloc(sizeof(char*)* our_size);
  
    
    // intialize to all -1
    for(i = 0; i <  other_size; i++){
        newAndMod[i] = -1;
        newFiles[i] = -1;
        commonFiles[i] = -1;
        newAndModNames[i] = NULL;
        newSoFar[i] = NULL;
     }
    for(i = 0; i <  our_size; i++){
        deletedFiles[i] = -1;
        deletedAndRenamed[i] = -1;
        modFiles[i] = -1;
        renamedFiles[i] = -1;
        deletedAndRenamed[i] = -1;
        deletedAndRenamedNames[i] = NULL;
        deletedSoFar[i] = NULL;
    }
    
    //print each line of our serialized global file table into array of strings 'our_files'
    char* s = serializeFileList(our);
    partition(&our_files, s, our_size);
    free(s);
    s = NULL;
    
    //put each line of received file table into array of strings 'other_files'
    char* string = (char*)malloc(((strlen(received_file_table)+SHUS_CONSTANT) * sizeof(char)));
    memset(string, 0, ((strlen(received_file_table)+SHUS_CONSTANT) * sizeof(char)));
    sprintf(string, "%s", received_file_table);
    partition(&other_files, string, other_size);
    memset(string, 0, ((strlen(received_file_table)+SHUS_CONSTANT) * sizeof(char)));
    sprintf(string, "%s", received_file_table);
    
    /* Put indices of files in other_files but not our_files in newAndModIndices
       Put indices of files in our_files but not other_files in deletedAndRenamed */
    getFilesNotInFirst(our_files, our_size, other_files, other_size, newAndMod); //indexes into other_files
    getFilesNotInFirst(other_files, other_size, our_files, our_size, deletedAndRenamed); //indexes into our_files
    
    /* 
        delimits filename and checksum in each file entry will null terminator
        changes a file_name:checksum to file\0checkusm
     
     */
    remove_colon(our_files, our_size);
    remove_colon(other_files, other_size);

    /* fill up newAndModNames with filenames from other_files using indices from newAndMod */
    populate_a_using_indices_from_b_indexing_c(newAndModNames, newAndMod, other_size, other_files);
    /* splits files in newAndMod into newFiles and modFiles */
    parititionIntoTwo(our_files, our_size, newAndModNames, other_size, modFiles, newFiles);
	
	/* newFiles has other_size */
	/* modFiles has our_size */
    
    for(i = 0; i < other_size && newFiles[i]>= 0; i++){
        newFiles[i] = bs_string(other_size, other_files, newAndModNames[newFiles[i]]);
    }
	
	/* deleted has our_size */
	/* renamed has other_size */
    

    populate_a_using_indices_from_b_indexing_c(deletedAndRenamedNames,  deletedAndRenamed, our_size, our_files);
    parititionIntoTwo(other_files, other_size, deletedAndRenamedNames, our_size, renamedFiles, deletedFiles);
    
    for(i = 0; i < our_size && deletedFiles[i]>=0; i++){
        deletedFiles[i] = bs_string(our_size, our_files, deletedAndRenamedNames[deletedFiles[i]]);
    }
    
    for(i = 0; i < other_size && renamedFiles[i] >= 0; i++) /* ignore , vestigial */
            renamedFiles[i] = -1;


    /* 
     restores file entries to original form
     changes file_name\0checksum to filename:checksum
     */
    remove_null(our_files, our_size);
    remove_null(other_files, other_size);


    /* setup */
    file_collection_t* collection = (file_collection_t*)malloc(sizeof(file_collection_t));
    collection->renamed_head = NULL;
    
    /* >-----------BEGIN NEW FILES --------------------------< */
    
    /* we obtain common files in the received file list and our list */
    /* make a new list out of common files and remove them from received list */
    file_node_t*  new_received_list = deserializeFileList(string);
    file_node_t* current = new_received_list;
    char* buffer = NULL;
    file_node_t* temp = NULL;
    file_node_t* update_peer_head = NULL;
    file_node_t* current_peer = NULL;
    while (current ) {
        
        long x = 0;
        long p = 1;
        x = current->checksum;
        while (x > 0) { //get # of digits of checksum into p
            x /= 10;
            p++;
        }
        buffer =(char*) malloc(sizeof(char) * ((p)+strlen(current->name)+SHUS_CONSTANT));
        memset(buffer, 0, sizeof(char) * ((p)+strlen(current->name)+SHUS_CONSTANT));
        sprintf(buffer, "%s:%" PRIu64 , current->name, current->checksum);
        
        if(bs_string(our_size, our_files, buffer) >= 0){
            temp = current;
            current = current->next;
            removeFromLinkedList(&new_received_list, temp);
            
            if(!update_peer_head){
                current_peer = update_peer_head = temp;
            } else {
                temp->previous = current_peer;
                current_peer->next = temp;
                current_peer = temp;
            }
        } else {
            current = current->next;
        }
        free(buffer);
    }
    
    collection->common_files = update_peer_head;
    
    
    /*
     High level description:
     using newFile indices, we obtain the
     corresponding checksum from other_files and iterate the
     received global filelist till we find a matching object
     
     We then copy the matching object into a new linked list of new files
     (not a memcopy, but actual duplication of object
     ----------------------------------------------------------*/
    file_node_t* head_new = NULL;
    file_node_t* new_current = NULL;
    current = NULL; // this is a file_node_t
    
    // put # of new files in 'limit'
    for(limit = 0; limit < other_size && newFiles[limit]>=0;limit++);
    
    if(limit){
        head_new = (file_node_t*) calloc(sizeof(file_node_t), sizeof(char));
        head_new->previous = NULL;
    }
    new_current = head_new;
    
    for(i = 0; i < limit; i++){ 
        // otherfiles array entries have strings of the form fileName:checksum
        long index_into_otherfiles = newFiles[i];
        char* s = other_files[index_into_otherfiles];
        replace_char(s, ':'); // puts null terminator between fileName and checksum; other_files char* now just fileName
        long checksum =  get_integer(strlen(s)+s+1);
        
        // look through new_file_tracker_list, which includes modified files, for the new file from above
        // and copy it into new_current file node
        current = new_received_list;
        while (current && current->checksum != checksum )
            current = current->next;
        
        if(!current){
            printf("shit 1");
            fflush(stdout);
            continue;
        }
        
        node_copy_name(new_current, current);
        new_current->next = NULL;
        
        if(current){
            removeFromLinkedList(&new_received_list, current);
            free_linked_list(current);
        }
        
        if(i < limit-1){
            new_current->next = (file_node_t*) calloc(sizeof(file_node_t), sizeof(char));
            new_current->next->previous = new_current;
        } else{
            new_current->next = NULL;
        }
        
        new_current = new_current->next;
    }
    if(head_new && !head_new->name){
        free(head_new);
        head_new = NULL;
    }


    collection->new_head = head_new;

    /* >-----------END NEW  FILES --------------------------< */


    
    /* >----------- BEGIN MODIFIED FILES --------------------------< */
    
    {
        current = new_received_list;
        file_node_t* head_mod = NULL;
        file_node_t* mod_current = NULL;
        current = NULL; // this is a file_node_t
        
        // put # of new files in 'limit'
        for(limit = 0; limit < our_size && modFiles[limit]>=0;limit++);
        
        if(limit){
            head_mod = (file_node_t*) calloc(sizeof(file_node_t), sizeof(char));
            head_mod->previous = NULL;
        }
        mod_current = head_mod;
        
        for(i = 0; i < limit; i++){
            // otherfiles array entries have strings of the form fileName:checksum
            long index_into_ourfiles = modFiles[i];
            char* s = our_files[index_into_ourfiles];
            replace_char(s, ':'); // puts null terminator between fileName and checksum; other_files char* now just fileName
            //long checksum =  get_integer(strlen(s)+s+1);
            
            // look through new_file_tracker_list now only has modified files, for the new file from above
            // and copy it into new_current file node
            current = new_received_list;
            while (current && strcmp(current->name, s)) {
                current = current->next;
            }
            
            if(!current){
                printf("shit 2");
                fflush(stdout);
                continue;
            }
            
            node_copy_name(mod_current, current);
            mod_current->next = NULL;
            
            if(current){
                removeFromLinkedList(&new_received_list, current);
                free_linked_list(current);
            }
            
            if(i < limit-1){
                mod_current->next = (file_node_t*) calloc(sizeof(file_node_t), sizeof(char));
                mod_current->next->previous = mod_current;
            } else{
                mod_current->next = NULL;
            }
            
            mod_current = mod_current->next;
        }
        
        collection->modified_head = head_mod;
}
    /* >----------- END MODIFIED FILES --------------------------< */
    
 
    
    /* >-----------BEGIN DELETED FILES --------------------------< */

    file_node_t* head_deleted = NULL;
    
    
    // put # of deleted files in limit
    for(limit = 0; limit < our_size && deletedFiles[limit]>=0; limit++);
    
    
    if(limit){
        head_deleted = (file_node_t*) calloc(sizeof(file_node_t), sizeof(char));
        head_deleted->previous = NULL;
    }
    new_current = head_deleted;
    
    // copy files to be deleted from our_files to collection->head_deleted
    for(i = 0; i < limit; i++){
        current = our; //our global file list head
        
        long index_to_ourfiles = deletedFiles[i];
        char* s = our_files[index_to_ourfiles];
        replace_char(s, ':'); // puts null terminator between fileName and checksum; our_files char* now just fileName
        
        while (current && strcmp(current->name,s)) //find matching file_node
            current = current->next;
        if(!current){
            printf("drake shit\n");
            fflush(stdout);
            continue;
        }
        
        node_copy_name(new_current, current);
        
        if(i < limit-1){
            new_current->next = (file_node_t*) calloc(sizeof(file_node_t), sizeof(char));
            new_current->next->previous = new_current;
        } else{
            new_current->next = NULL;
        }
        new_current = new_current->next;
    }
    
    if(head_deleted && !head_deleted->name){
        free(head_deleted);
        head_deleted = NULL;
    }

    collection->deleted_head = head_deleted;
    /* >-----------END DELETED FILES --------------------------< */
    
    
 
    
    //cleanup
    free(string);
    string = NULL;
    free(s);
    s = NULL;
    
    free(newAndMod);
    free(newFiles);
    free(commonFiles);
    free(modFiles);
    free(deletedFiles);
    free(renamedFiles);
    free(deletedAndRenamed);
    free(newAndModNames);
  
    for(i = 0; i < our_size; i++){
        free(our_files[i]);
    }
    for(i = 0; i < other_size; i++){
        free(other_files[i]);
    }
    free(deletedAndRenamedNames);
    free(deletedSoFar);
    free(newSoFar);
    free(our_files);
    free(other_files);
    free_linked_list(new_received_list);
    return collection;
}


void populate_a_using_indices_from_b_indexing_c(char** a, long* b, long size_b, char** c){
    for(long i = 0; i< size_b && b[i] >= 0; i++){
        a[i] = c[b[i]];
    }
}



/*
 * ===  FUNCTION  ======================================================================
 *         Name: node_copy
 *  Description:  copy over contents of b to a
 * =====================================================================================
 */

void node_copy_name(file_node_t* a,file_node_t* b){
    
    a->next = NULL;
    a->previous = NULL;
    a->name_pointer = NULL;
    a->newpeerip = NULL;
    
    
    a->numPeers = b->numPeers;
    a->size = b->size;
    a->type = b->type;
    a->version_number = b->version_number;
    a->checksum = b->checksum;
    
    long s = strlen(b->name)+SHUS_CONSTANT;
    a->name = (char*)malloc(s * sizeof(char));
    memset(a->name, 0, s);
    sprintf(a->name,"%s", b->name);
    
    if(a->numPeers > 0){
        a->newpeerip = (char**)malloc(sizeof(char*)* a->numPeers);
    }
    
    for(long i = 0; i < a->numPeers; i++){
        s = strlen(b->newpeerip[i])+SHUS_CONSTANT;
        a->newpeerip[i] = (char*)malloc(sizeof(char) * s);
        memset(a->newpeerip[i], 0, s);
        
        sprintf(a->newpeerip[i], "%s", b->newpeerip[i]);
    }
}

/*
 * ===  FUNCTION  ======================================================================
 *         Name: node_copy
 *  Description:  copy over contents of b to a
 * =====================================================================================
 */
void node_copy_notname(file_node_t* a,file_node_t* b){
    
    a->next = b->next;
    a->previous = b->previous;
    a->name = NULL;
    a->newpeerip = NULL;
  
    a->numPeers = b->numPeers;
    a->size = b->size;
    a->type = b->type;
    a->version_number = b->version_number;
    a->checksum = b->checksum;
    
    a->name_pointer = (file_name_t*)malloc(sizeof(file_name_t));
    a->name_pointer->previous   = b->name_pointer->previous;
    a->name_pointer->next = b->name_pointer->next;
    sprintf(a->name_pointer->name,"%s", b->name_pointer->name);


}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  partition
 *  Description:  Print substring of each line of string into malloc'ed array of (char*) 'list'
 *                as fileName:checksum
 * =====================================================================================
 */
void partition(char*** list, char* string, long size){
    
    //setup
    long count = 0;
    char* token = NULL;
    
    // print each non-blank line of passed string into malloc'ed array of (char *) called list
    for(token = strtok(string, "\n"); token; token = strtok(NULL, "\n") ) {
        if (has_colon(token)){ // ensure we skip blank lines
            long s = strlen(token);
            (*list)[count] = (char*)malloc((sizeof(char) * s)+SHUS_CONSTANT);
            memset((*list)[count], 0, (sizeof(char) * s)+SHUS_CONSTANT);
            sprintf((*list)[count], "%s", token);
            count++;
        }
    }
    
    //change every line from 'file_name:checksum:newpeerip:numpeers:type:size:version' to 'fileName:checksum'
    long i = 0;
    for(i = 0; i < count; i++){
        token = strtok((*list)[i], ":"); //filename
        token = strtok(NULL, ":"); // checksum
        sprintf((*list)[i]+strlen((*list)[i]), ":%s", token);
    }   
}
/*
 * ===  FUNCTION  ======================================================================
 *         Name:  serializeFileList
 *  Description:  Turns file_node_t linked list into string array
 * =====================================================================================
 */
char* serializeFileList(file_node_t *head){
    if (!(head && head->name) || (head && head->name && !strlen(head->name))) {
        //printf("SerializeFileList received empty\n");
        return NULL;
    }
    //file_name:checksum:newpeerip:numpeers:type:size:version
    long longest_name = get_longest_filename(head);
    if(longest_name <= 0){
        printf("\nFailed to obtain file with longest name");
        return NULL;
    }
    long size = get_size_linked_list(head);
    if(size <= 0){
        printf("\nFailed to obtain linked list of size > 0");
        return NULL;
    }
    long size_of_string = longest_name + 1000000;
    
    char* serial = (char*)malloc(sizeof(char) * size_of_string*size);
    memset(serial, 0, sizeof(char) * size_of_string*size);
    char* start = serial;
    file_node_t* current = head;
    char** sort_array = (char**)malloc(sizeof(char*) * size);
    long counter = 0;
    while (current) {
      
        char* peers = (char*)malloc(sizeof(char)*20*current->numPeers);
        memset(peers, 0, sizeof(char)*20*current->numPeers);
        
        if(current->numPeers > 0){
            sprintf(peers, "%s", current->newpeerip[0]);
        } else {
            sprintf(peers, "%s", "Strange");
        }
        
  
        for (long i = 1; WHO_AM_I && i < current->numPeers; i++) {
            sprintf(peers+strlen(peers), ",%s", current->newpeerip[i]);
            //sprintf(peers, "%s,%s", peers, current->newpeerip[i]);
        }
        sort_array[counter] = (char*)malloc(sizeof(char)* size_of_string);
        memset(sort_array[counter], 0, sizeof(char)* size_of_string);
        sprintf(sort_array[counter], "%s:%"PRIu64":%s:%d:%d:%u:%d\n", current->name, current->checksum,peers, current->numPeers,current->type,current->size, current->version_number);
        counter++;
        
        current = current->next;
        
        free(peers);
    }
    
    qs_string(sort_array, 0, size-1);
    
    for(long i = 0; i < size; i++){
        sprintf(serial, "%s", sort_array[i]);
        serial+= strlen(serial);
        free(sort_array[i]);
    }
    
    free(sort_array);
    
    
    
    
    return start;
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  deserializeFileList
 *  Description:  Turns string array into file_node_t linked list and returns head
 * =====================================================================================
 */
file_node_t* deserializeFileList(char* serializedFileList){
    if (!serializedFileList) {
        printf("DeserializeFileList received null\n");
        return NULL;
    }
    long count = get_file_entry_count(serializedFileList);
    if(count < 0){
        printf("Failed to get count\n");
        return NULL;
    }
    long s = strlen(serializedFileList)+SHUS_CONSTANT;
    char* received_string = (char*)malloc((sizeof(char)* s));
    memset(received_string, 0, (sizeof(char)* s));
    sprintf(received_string, "%s", serializedFileList);
    //             1        2         3       4     5    6
    //file_name:checksum:newpeerip:numpeers:type:size:version
    char* token = strtok(received_string, "\n");
    char* dummy = (char*)malloc(s);
    memset(dummy, 0, s);
    file_node_t* current =  NULL;
    file_node_t* previous = NULL;
    file_node_t* head = NULL;
    
    while (token) {
        
        if(!current) {
            current = (file_node_t*) calloc(sizeof(file_node_t), sizeof(char));
            current->next = NULL;
            current->previous = previous;
            if(previous) previous->next = current;
            else head = current;
            current->name = NULL;
        }
        
        replace_char(token, ':');
        memset(dummy, 0, s);
        get_string_at_index(token, 0, dummy);
        current->name = (char*)malloc((strlen(dummy)* sizeof(char))+SHUS_CONSTANT);
        memset(current->name, 0, (strlen(dummy)* sizeof(char))+SHUS_CONSTANT);
        sprintf(current->name, "%s", dummy);
        memset(dummy, 0, s);
        get_string_at_index(token, 1, dummy); //checksum at point 1
        current->checksum =get_integer(dummy);
        memset(dummy, 0, s);
        get_string_at_index(token, 3, dummy); //numpeers at point 3
        current->numPeers = get_integer(dummy);
        memset(dummy, 0, s);
        get_string_at_index(token, 2, dummy);
        current->newpeerip  = (char**)malloc(sizeof(char*)* current->numPeers);
        replace_char(dummy, ',');
        for(long i = 0; i < current->numPeers; i++){
            current->newpeerip[i] = (char*)malloc(sizeof(char)*20);
            memset(current->newpeerip[i], 0, sizeof(char)*20);
            get_string_at_index(dummy, i, current->newpeerip[i]);
        }
        get_string_at_index(token, 4, dummy);
        current->type = get_integer(dummy);
        memset(dummy, 0, s);
        get_string_at_index(token, 5, dummy);
        current->size = get_integer(dummy);
        memset(dummy, 0, s);
        get_string_at_index(token, 6, dummy);
        current->version_number = get_integer(dummy);
        token = strtok(NULL, "\n");
        previous = current;
        current = NULL;
    }
    free(dummy);
    free(received_string);
    return head;
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  write_config
 *  Description:  Write a string to the config file in passed directory
 * =====================================================================================
 */
void write_config(char* string){
    if(!string) return;
    
    long size = strlen(root_path) + 1 + 2000 + 1; //dir:delimiter:STORAGE:delimiter
    char* s = (char*)calloc(size, sizeof(char));
    
    char delimiter = '/';
    sprintf(s, "%s%c%s%cconfig", root_path, delimiter, STORAGE, delimiter);
    
    FILE* config = fopen(s, "w");
    if (config) {
        fprintf(config, "%s", string);
        fclose(config);
    } else{
        printf("Failed to load file to write config to\n");
        fflush(stdout);
    }
}

/*
 * ===  FUNCTION  ======================================================================
 *         Name:  updateExistingListWithDifferences
 *  Description:  Updates existing global file list with differences from
 *                file_collection_t struct
 * =====================================================================================
 */
void updateExistingListWithDifferences(file_collection_t *differences){
    if (!differences) {
        printf("updateExistingListWithDifferences received NULL value of differences arg\n");
        return;
    }
    
    if(globalFileListHead && !globalFileListHead->name){
        printf("Seems global file list improperly freed\n Investigate. Reinitializing");
        globalFileListHead = NULL;
    }
    
    
    deleteFileNodesFromList(differences->deleted_head);
    addFileNodesToList(differences->new_head);
    //renameRenamedFiles(differences->renamed_head);
    modifyModifiedFiles(differences->modified_head);
    
    
    
    
    char* s = serializeFileList(globalFileListHead);

    // cleanup
    free_linked_list(globalFileListHead);
    globalFileListHead = deserializeFileList(s);
    free(s);
    return;
}


void modifyModifiedFiles(file_node_t* head_of_mod){
    if(!head_of_mod)
        return;
    
    if(!globalFileListHead)
        return;
    
    file_node_t* current_our = globalFileListHead;
    file_node_t* current_mod = head_of_mod;
    
    
    while (current_mod) {
        while (strcmp(current_our->name, current_mod->name)) {
            current_our = current_our->next;
        }
        
        if(current_mod->version_number < current_our->version_number){
            //ignored since out of date
        } else if(current_mod->version_number >= current_our->version_number){
            //this is a file update
            
            current_our->version_number++;
            
            free(current_our->name);
            for(long i = 0; i < current_our->numPeers;i++)
                free(current_our->newpeerip[i]);
            free(current_our->newpeerip);
        
            current_our->checksum = current_mod->checksum;
            long s = strlen(current_mod->name)+SHUS_CONSTANT;
            current_our->name = (char*)malloc(s * sizeof(char));
            memset(current_our->name, 0, s);
            sprintf(current_our->name,"%s", current_mod->name);
            
            if(current_mod->numPeers > 0){
                current_our->newpeerip = (char**)malloc(sizeof(char*)* current_mod->numPeers);
            }
            
            for(long i = 0; i < current_mod->numPeers; i++){
                s = strlen(current_mod->newpeerip[i])+SHUS_CONSTANT;
                current_our->newpeerip[i] = (char*)malloc(sizeof(char) * s);
                memset(current_our->newpeerip[i], 0, sizeof(char) * s);
                sprintf(current_our->newpeerip[i], "%s", current_mod->newpeerip[i]);
            }
            
            current_our->numPeers = current_mod->numPeers;
            current_our->size = current_mod->size;
            current_our->type = current_mod->type;
    
        }
//         else if(current_mod->version_number > current_our->version_number ){
//            /* I need to think about this */
//        }
       
        current_mod = current_mod->next;
    }
}

/*
 this function removes files that have lower version numbers
 from the list
*/
void do_version_comparisons(file_node_t* head_of_mod){
    if(!head_of_mod)
        return;
    
    if(!globalFileListHead)
        return;
    
    file_node_t* current_our = globalFileListHead;
    file_node_t* current_mod = head_of_mod;

    
    while (current_mod) {
        while (strcmp(current_our->name, current_mod->name)) {
            current_our = current_our->next;
        }
        if(current_mod->version_number < current_our->version_number){
            //received file is out of date, ignore it
            // so remove it from linked list
            

            file_node_t* n = current_mod->next;
            
            
            removeFromLinkedList(&head_of_mod, current_mod);
            free(current_mod);
            current_mod = NULL;
            current_mod = n;
        
        } else if(current_mod->version_number == current_our->version_number){
            current_mod = current_mod->next;
            //this is a file update
        } else if(current_mod->version_number > current_our->version_number ){
            //this is a file update
            current_mod = current_mod->next;
        }
        
    }
    
    char* s = serializeFileList(head_of_mod);
    write_config(s);
    free(s);
    
    
}

void renameRenamedFiles(file_node_t* head_of_renamed){
    if(!head_of_renamed)
        return;
    
    if(!globalFileListHead)
        return;
    
    file_node_t* current_our = globalFileListHead;
    file_node_t* current_update = head_of_renamed;
    
    
    while (current_update) {
        
        while (strcmp(current_our->name, current_update->name)) {
            current_our = current_our->next;
        }
        
        long new_size = strlen(current_update->name);
        char* new_name = new_size+1+current_update->name;
        
        free(current_our->name);
        current_our->name = (char*)malloc(sizeof(char)* (new_size+SHUS_CONSTANT));
        memset(current_our->name, 0, sizeof(char)*(new_size+SHUS_CONSTANT));
       
        sprintf(current_our->name, "%s", new_name);
        
        current_update = current_update->next;
    }
    
    
}


/*
 * ===  FUNCTION  ======================================================================
 *         Name:  updatePeer_ips
 *  Description:  Updates existing global file list with new ip addresses
 * =====================================================================================
 */
void updatePeer_ips(file_node_t* head_of_list, file_collection_t* differences){
    
    file_node_t* head_of_updates = differences->common_files;
    file_node_t* ours = head_of_list;
    file_node_t* update = head_of_updates;
    
    /* iterate and update nodes */
    while (update) {
        while (ours && strcmp(ours->name, update->name))
            ours = ours->next;
        
        if(!ours){
            printf("The impossible just happened\n");
            break;
        }
        long peers = update->numPeers;
        
        /* sort our newpeerip table */
        qs_string(ours->newpeerip, 0, ours->numPeers-1);
        
        /* for every ip in update node
         check if we have it
         if we don't have it, add it to our list
         */
        for (long i = 0; i < peers; i++) {
            /* hey remind me to check for the success and failure of realloc lol */
            if (bs_string(peers, ours->newpeerip, update->newpeerip[i]) < 0){
                
                
                ours->newpeerip = (char**)realloc(ours->newpeerip, sizeof(char*)*(ours->numPeers+1));
                long a = SHUS_CONSTANT + strlen(update->newpeerip[i]);
                ours->newpeerip[ours->numPeers] = (char*)malloc(a * sizeof(char));
                memset(ours->newpeerip[ours->numPeers], 0, a * sizeof(char));
                sprintf(ours->newpeerip[ours->numPeers], "%s",update->newpeerip[i]);
                ours->numPeers++;
            }
        }
        update = update->next;
    }
}


/*
 * ===  FUNCTION  ======================================================================
 *         Name:  removeIPfromFileNode
 *  Description:  Removes a given ip from node's filetable and decrements the peer count
 * =====================================================================================*/


void removeIPfromFileNode(file_node_t* node, char* ip){
    
    if(!node||!ip)
        return;

    
    long num = node->numPeers;
    qs_string(node->newpeerip, 0, num-1);
    long pos = bs_string(num, node->newpeerip, ip);
    
    if(pos < 0){
        printf("The ip %s could not be found in current node", ip);
        return;
    }
    
    
    node->numPeers--;
    
    
    
    char** newPeerList = (char**)malloc(sizeof(char*)*node->numPeers);
    
    long count  = 0;
    
    for(long i = 0; i < num; i++){
        
        if(!strcmp(node->newpeerip[i], ip)){
            free(node->newpeerip[i]);
            node->newpeerip[i] = NULL;
            continue;
        }
        
        newPeerList[count++] = node->newpeerip[i];
    }
    
    free(node->newpeerip);
    node->newpeerip =NULL;
    node->newpeerip = newPeerList;
    
}


// -------------------------------------------  SUPPORT FUNCTIONS





/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  deleteFileNodesFromList
 *  Description:  frees nodes from existing file list with the same names as filesToDelete
 * =====================================================================================
 */
void deleteFileNodesFromList(file_node_t *filesToDelete){
    if (!(filesToDelete && filesToDelete->name) || !(globalFileListHead && (globalFileListHead)->name)) return;
    file_node_t* to_delete = filesToDelete;
    file_node_t* current = globalFileListHead;
    while (to_delete) {
        while (current && strcmp(current->name, to_delete->name)) 
            current = current->next;
        if(!(current && current->name)){
            printf("\nuh oh\n");
            to_delete = to_delete->next;
            current = globalFileListHead;
            break;
        }
        file_node_t* next = current->next;
        file_node_t* previous = current->previous;
        
        
        if (previous)
            previous->next = next;
        else
            globalFileListHead = next;
        if (next)
            next->previous = previous;
        
    
        
        current->next = NULL;
        current->previous = NULL;
        free(current);
        current = next;
        
        to_delete = to_delete->next;
    }
}

/* Find character c in string s and replace it will null */
void replace_char(char* s, char c){
    while (*s != '\0') {
        if(*s == c) *s = '\0';
        s++;
    }
}

/* Print string s from index onwards into d */
void get_string_at_index(char* s, long index, char* d){
    long count = 0;
    for (long i = 0; i < index; ) {
        if(*s == '\0') i++;
        s++;
        count++;
    }
    sprintf(d, "%s", s);
    
    /* hmm replace this with s-=count lol */
    while (count >= 0) {count--;s--;}
}

/* Free linked list of file nodes */
void free_linked_list(file_node_t* head){
    
    file_node_t* temp;
    file_node_t* p;
    file_node_t* n;
    
    while (head) {
        
        
        p = head->previous;
        n = head->next;
        
        if(p) head->next = NULL;
        if(n) n->previous = NULL;
        
        head->next = NULL;
        head->previous = NULL;
    
        temp = head;
       
        if (temp->name){
            free(temp->name);
            temp->name = NULL;
        }
        long i = 0;
        while (i < temp->numPeers && temp->newpeerip) {
            if(temp->newpeerip[i]){
                free(temp->newpeerip[i]);
                temp->newpeerip[i]= NULL;
            }
            i++;
        }
        if(temp->newpeerip){
            free(temp->newpeerip);
            temp->newpeerip = NULL;
        }
        temp = NULL;
        
        head = n;
    }
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  addFileNodesToList
 *  Description:  adds linked list of file_collection_t->new_head to end of existing file list
 *                and re-alphabetizes
 * =====================================================================================
 */
void addFileNodesToList(file_node_t *filesToAdd){
    if (!(filesToAdd && filesToAdd->name)) return;
    
    if(!globalFileListHead){
        globalFileListHead = filesToAdd;
        return;
    }
    
    //add ips
    
    file_node_t* current = globalFileListHead;
    while (current->next) current = current->next;
    current->next = filesToAdd;
    filesToAdd->previous = current;
    char* string = serializeFileList(globalFileListHead);
    free_linked_list(globalFileListHead);
    globalFileListHead = NULL;
    globalFileListHead= deserializeFileList(string);
    free(string);
}

/* Remove file node from file list */
void removeFromLinkedList(file_node_t** head_p, file_node_t* current){
    if(!current){
        return;
    }
    file_node_t* previous = current->previous;
    file_node_t* next = current->next;
    if (previous) previous->next = next;
    else {
        *head_p = next;
    }
    if(next) next->previous = previous;
    current->previous = NULL;
    current->next = NULL;
}

/* Return file size */
long get_file_size(char* path){
    u_int64_t sz;
    FILE* file = fopen(path, "rb");
    if(!file){
        printf("The file doesn't exist\n");
        return -1;
    }
    fseek(file, 0L, SEEK_END);
    sz = ftell(file);
    fclose(file);
    return sz;
}

/* Return length of longest file name */
long get_longest_filename(file_node_t *head){
    file_node_t* current = head;
    long longest = -1;
    while (current && current->name) {
        long l = strlen(current->name);
        if (l > longest) longest = l;
        current  = current->next;
    }
    return longest;
}

/* Binary searches list a for the filenames in list b, and puts the index of anything in a but not b
in fileNotInList */
void getFilesNotInFirst(char** a, long size_a, char** b, long size_b, long* fileNotInList){

    long count = 0;
    long bs_index = 0;
    long i;
    for(i = 0; i < size_b && b[i] ; i++){
        if((bs_index = bs_string(size_a, a, b[i])) < 0){
            fileNotInList[count] = i;
            count++;
        }
    }
}

/* gets files in both list a and b */
void getFilesInBoth(char** a, long size_a, char** b, long size_b, char** commonFiles){
    
    long count = 0;
    long bs_index = 0;
    long i;
    for(i = 0; i < size_b && b[i] ; i++){
        if((bs_index = bs_string(size_a, a, b[i])) >= 0){
            commonFiles[count] = b[i];
            count++;
        }
    }
    
    
}

/* gets checksums in both list a and b */
void getChecksumsNamesInBoth(char** a, long size_a, char** b, long size_b, long* commonFiles, char** commonNames){
    
    long count = 0;
    long bs_index = 0;
    long i;
    long size;
    
    
    long* advance_record = (long*)malloc(sizeof(long)*size_a);
    for(i = 0; i < size_a && a[i]; i++){
        long l = strlen(a[i])+1;
        advance_record[i] = l;
        a[i]+=l;
    }
    
    
    for(i = 0; i < size_b && b[i] ; i++){
        
        char* s = strlen(b[i]) + b[i]+1;
        
        if((bs_index = bs_string(size_a, a, s)) >= 0){
            commonFiles[count] = get_integer(s);
            
            char* oldName = b[i];
            char* newName = a[bs_index] -(advance_record[bs_index]);
            
            size = (strlen(oldName) + strlen(newName)+ SHUS_CONSTANT);
            size *= sizeof(char);
            
            commonNames[count] = (char*)malloc(size);
            memset(commonNames[count], 0, size);
            
            sprintf(commonNames[count], "%s%c%s", oldName, '\0', newName);
            
            count++;
        }
    }
    
    for(i = 0; i < size_a && a[i]; i++){
        a[i]-= advance_record[i];
    }
    
    free(advance_record);
    
}

/* Paritions b into c,d where  c are filenames it has in common with a and d filenames it doesn't */
void parititionIntoTwo(char** a, long size_a, char** b, long size_b, long* c, long* d){
    
    long count = 0;
    long count2 = 0;
    long bs_index = 0;
    long i;
    
    for(i = 0; i < size_b && b[i]; i++){
        
    
        if((bs_index = bs_string(size_a, a, b[i])) < 0){
            d[count] = i;
            count++;
        } else {
            c[count2] = bs_index;
            count2++;
        }
    }
}



/* Binary searches list a for the checksums in list b */
void getChecksumsNotInFirst(long* a, long size_a,  long* b, long size_b,  long* checksumNotInList){
    long count = 0;
    for(long i = 0; i < size_b; i++){
        if(bs_number(size_a, a, b[i]) < 0){
            checksumNotInList[count] = b[i];
            count++;
        }
    }
}

/* Puts object2 immediately after object1 */
void squeeze_in(file_node_t* object, file_node_t* object_2) {
    
    file_node_t* t_obj;
    
    if(object->next){
        t_obj = object->next;
        object->next = object_2;
        object_2->previous = object;
        object_2->next = t_obj;
        t_obj->previous = object_2;
    }
    else {
        object->next = object_2;
        object_2->previous = object;
        object_2->next = NULL;
    }
}


/* Return number of elements in list */
long get_size_linked_list(file_node_t* list){
    long size = 0;
    if(!(list && list->name)){
        return -1;
    }
    while(list){
        list = list->next;
        size++;
    }
    return size;
}

/* Returns 1 if string is empty */
int is_empty(const char *s) {
    while (*s != '\0') {
        if (!isspace(*s))
            return 0;
        s++;
    }
    return 1;
}

/* Return the number of files in serialized file table */
long get_file_entry_count(char* s){
    int found_colon = 0;
    long size = 0;
    for(int i = 0; s[i] != '\0'; i++){
        if(s[i] == ':'){
            found_colon++;
        }
        else if(s[i] == '\n'){
            if (found_colon > 0) {
                size++;
                found_colon = 0;
            }
        }
    }
    return size;
}

/* Returns 1 if string contains colon */
int has_colon(const char *s) {
    while (*s != '\0') {
        if (*s == ':') return 1;
        s++;
    }
    return 0;
}

/* Return parsed integer from string */
int get_integer(char* input){
    char** dummy = NULL;
    return (int) strtol(input, dummy, 10);
}


/* Return path to DartSync directory */
void getDartSyncPath_fs(char* string){
    struct passwd* pwd = getpwuid(getuid());
    sprintf(string, "%s/DartSync", pwd->pw_dir);
}

/* Return head of global file list */
file_node_t* getList() {
    return myCurrentHead;
}



void remove_colon(char** x, long size){
    
    for(long i = 0; i < size; i++){
        replace_char(x[i], ':');
    }
    
}


void remove_null(char** x, long size){
    
    for(long i = 0; i < size; i++){
        
        replace_first_null_with_colon(x[i], ':');
    }
    
}


/* Find character c in string s and replace it will null */
void replace_first_null_with_colon(char* s, char r){
    while (*s != '\0'){
        s++;
    }
    *s = r;
}


