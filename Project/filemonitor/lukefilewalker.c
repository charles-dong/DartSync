#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <pwd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include "../utils/socket_common.h"
#include "../utils/qs.h"
#include "../utils/common.h"
#include "lukefilewalker.h"

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
int remove_ip(file_node_t* current, char* ip);


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
    char** other_files2 = (char**)malloc(sizeof(char*)* other_size);
    char** our_files2 =  (char**)malloc(sizeof(char*)* our_size);
    
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
    
	for(i = 0; i <  our_size; i++){
		our_files2[i] = our_files[i];
	}
	
	for(i = 0; i <  other_size; i++){
		other_files2[i] = other_files[i];
	}
	
    
    /* Put indices of files in other_files but not our_files in newAndModIndices
       Put indices of files in our_files but not other_files in deletedAndRenamed */
    getFilesNotInFirst(our_files, our_size, other_files, other_size, newAndMod); //indexes into other_files
    getFilesNotInFirst(other_files, other_size, our_files, our_size, deletedAndRenamed); //indexes into our_files
    
    remove_colon(our_files2, our_size);
    remove_colon(other_files2, other_size);
    
    qs_string(our_files2, 0, our_size-1);
    qs_string(other_files2, 0, other_size-1);
    
    for(long i  = 0; i < our_size && deletedAndRenamed[i]>=0; i++){
        long index_into_our_files = deletedAndRenamed[i];
        char* s = our_files[index_into_our_files];
        deletedAndRenamed[i] = bs_string(our_size, our_files2, s);
    }
    
    for(long i = 0; i < other_size && newAndMod[i]>= 0; i++){
        long index_into_other_files = newAndMod[i];
        char* s = other_files[index_into_other_files];
        newAndMod[i] = bs_string(other_size, other_files2, s);
    }
    
    
    /* 
        delimits filename and checksum in each file entry will null terminator
        changes a file_name:checksum to file\0checkusm
     
     */

    /* fill up newAndModNames with filenames from other_files using indices from newAndMod */
    populate_a_using_indices_from_b_indexing_c(newAndModNames, newAndMod, other_size, other_files2);
    /* splits files in newAndMod into newFiles and modFiles */
    parititionIntoTwo(our_files2, our_size, newAndModNames, other_size, modFiles, newFiles);
    /* modFiles now indexes into our_files2 */
    
	
	/* newFiles has other_size */
	/* modFiles has our_size */

    for(i = 0; i < other_size && newFiles[i]>= 0; i++){
        newFiles[i] = bs_string(other_size, other_files2, newAndModNames[newFiles[i]]);
    }
	
	/* deleted has our_size */
	/* renamed has other_size */
    

    populate_a_using_indices_from_b_indexing_c(deletedAndRenamedNames,  deletedAndRenamed, our_size, our_files2);
    parititionIntoTwo(other_files2, other_size, deletedAndRenamedNames, our_size, renamedFiles, deletedFiles);
    
    for(i = 0; i < our_size && deletedFiles[i]>=0; i++){
        deletedFiles[i] = bs_string(our_size, our_files2, deletedAndRenamedNames[deletedFiles[i]]);
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
    
    
    remove_colon(our_files, our_size);
    remove_colon(other_files, other_size);
 
    
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
        char* s = other_files2[index_into_otherfiles];
        //replace_char(s, ':'); // puts null terminator between fileName and checksum; other_files char* now just fileName
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
            char* s = our_files2[index_into_ourfiles];
            //replace_char(s, ':'); // puts null terminator between fileName and checksum; other_files char* now just fileName
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
        char* s = our_files2[index_to_ourfiles];
        //replace_char(s, ':'); // puts null terminator between fileName and checksum; our_files char* now just fileName
        
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
    free(other_files2);
    free(our_files2);
  
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
        memset(a->newpeerip[i], 0, sizeof(char) *s);
        
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
 *         Name:  start_traversal
 *  Description:  For given path 'directory', recursively obtains file and directory names
 *                and stores it as file_node_t linked list in global variable localFileListHead
 *       Return:  Returns 1 for success and -1 for failure
 * =====================================================================================
 */
int start_traversal(){

    //setup
    char string[BUFSIZ*100];
    memset(string, 0, BUFSIZ*100);
    char delimiter = '/';
    char* full_path = NULL;
    
    // For the path 'directory', recursively obtain files and directories
    // and stores them as file_node_t linked list with 'object' as head

    long c = WALK_SIZE+SHUS_CONSTANT;
    c*= sizeof(char);
    char* d = (char*)malloc(c);
    memset(d, 0, c);
    sprintf(d, "%s", root_path);
    
    
    listdir(d, 0,  firstItemInDartSync);
    file_node_t* object = firstItemInDartSync;
    
    // go through each object and put full path in object->name, fill in other file_node_t attributes
    while (object) {
        int abort = 0;
        file_name_t* n = object->name_pointer;
        file_name_t* p = NULL;
        int i = WALK_SIZE;
        while (n->previous) {
            p = n;
            n = n->previous;
            n->next = p;
            i += strlen(n->name);
        }
        object->name = (char*)malloc((i * sizeof(char))+SHUS_CONSTANT);
        memset(object->name, 0, (i* sizeof(char))+SHUS_CONSTANT);
        while (n) {
            if (strlen(object->name)) {
                sprintf(object->name, "%s/%s", object->name, n->name);
                //sprintf(object->name+strlen(object->name), "/%s", n->name);
            } else  sprintf(object->name, "%s", n->name);
            n = n->next;
            object->size = 0;
            long a = strlen(object->name) + strlen(root_path)+SHUS_CONSTANT;
            if(full_path) free(full_path);
            full_path = (char*)malloc((a * sizeof(char)));
            memset(full_path, 0,  (a*sizeof(char)));
            sprintf(full_path, "%s%c%s", root_path, delimiter, object->name);
            
            long l = get_file_size(full_path);
            if(l < 0) {
                printf("Failed to get size for %s\n", full_path);
                fflush(stdout);
                abort = 1;
                break;
            }
            else 
                object->size = (unsigned int)l;
        }
        
        // if user creates and immediately deletes file, listdir will find it but start_traversal won't
        // if abort was set to 1, start_traversal couldn't find the file and will recrawl the whole file system
        if(abort){
            {
                file_node_t* p = object->previous;
                file_node_t* n = object->next;
            
                if(p) 
                    p->next = n;
                if(n) 
                    n->previous = p;
                
                object->next = NULL;
                object->previous = NULL;
                free(object->name_pointer);
                object->name_pointer = NULL;
                free_linked_list(object);
                object = NULL;
                object = n;
            }
            
            if(full_path) 
                free(full_path);

            return -1;
        } else {
            memset(string, 0, BUFSIZ*100);
            sprintf(string, "%s/%s",root_path, object->name);
            object->checksum = checksum(string);
            object->numPeers = 1;
            object->version_number = 0;
            object->newpeerip = (char**)malloc(sizeof(char*));
            memset(object->newpeerip, 0, sizeof(char*));
            object->newpeerip[0] = (char*)malloc(sizeof(char)*40);
            memset(object->newpeerip[0], 0, sizeof(char)*40);
            get_my_ip(object->newpeerip[0]);
        }

        if(full_path)
            free(full_path);
        full_path = NULL;
        object = object->next;
    }

    //cleanup
    object = firstItemInDartSync;
    while (object) {
        if(object->name_pointer)
            free(object->name_pointer);
        object->name_pointer = NULL;
        object = object->next;
    }
    
    if(full_path)
        free(full_path);
    full_path = NULL;

    if(!(firstItemInDartSync)){
        free_linked_list(object);
        free_linked_list(localFileListHead);
        localFileListHead = NULL;
    } else {
        object = (file_node_t*) calloc(sizeof(file_node_t), sizeof(char));
        node_copy_name(object, firstItemInDartSync);
        object->next = firstItemInDartSync->next;
        firstItemInDartSync->next = NULL;
        free_linked_list(firstItemInDartSync);
        firstItemInDartSync = NULL;
        free_linked_list(localFileListHead);
        localFileListHead = NULL;
        localFileListHead = object;
    }

    return 1;
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  listdir
 *  Description:  For given path 'name', recursively obtain files and directories
 *                and stores them as file_node_t linked list; makes passed 'object' the head
 *       Return:  1 for success, -1 for failure
 * =====================================================================================
 */
int listdir(char *name, int level, file_node_t* object) {

    // setup
    DIR *dir = NULL;
    struct dirent *entry;
    if (!(dir = opendir(name))){
        if(name) free(name);
        name = NULL;
        return -1;
    }
    
    if (!(entry = readdir(dir))){
        if(name) free(name);
        name = NULL;
        return -1;
    }
    long x = 0;
    
    if (level == 0){ //this means we're in the root directory of DartSync
        firstItemInDartSync = (file_node_t*) calloc(sizeof(file_node_t), sizeof(char));
        object = firstItemInDartSync;
    }

    // recursively put file and directory information in file_node_t linked list
    file_node_t* object_new = NULL;
    do {
       
        if (level+x){
            object_new = (file_node_t*) calloc(sizeof(file_node_t), sizeof(char));
        }
        else{
            if(!x)
                object_new = firstItemInDartSync;
        }

        

        long size = strlen(name) + strlen(entry->d_name)+SHUS_CONSTANT;
        char* full_path = malloc(size*sizeof(char));
        memset(full_path, 0, size*sizeof(char));
        
        
        sprintf(full_path, "%s/%s",name,entry->d_name);
        
        struct stat s;
        if (stat(full_path, &s)){
            free(full_path);
            continue;
        }
    
        if ((S_ISDIR(s.st_mode)))
        {
            if (!strcmp(entry->d_name, STORAGE)) 
                continue;
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) 
                continue;
            
            long d = strlen(name)+SHUS_CONSTANT;
            d*=sizeof(char);
        
            char* path = (char*)malloc(d);
            memset(path, 0, d);
            
            sprintf(path, "%s/%s",name, entry->d_name);

            
            file_name_t* name_new = (file_name_t*)malloc(sizeof(file_name_t));
            
            name_new->name[0] = '\0';
            object_new->name_pointer = name_new;
            name_new->next = NULL;
            
            if (level) 
                name_new->previous = object->name_pointer;
            else 
                name_new->previous = NULL;
            if(level+x) 
                squeeze_in(object, object_new);
            else 
                object_new->next = NULL;
            
            sprintf(object_new->name_pointer->name, "%s", entry->d_name);
            object_new->type = 'd';
            listdir(path, level + 1, object_new);
            
            x++;
        } else { // it's a file..
            file_name_t* name_new = (file_name_t*)malloc(sizeof(file_name_t));

            name_new->name[0] = '\0';
            object_new->name_pointer = name_new;
            if (level) 
                name_new->previous = object->name_pointer;
            else 
                name_new->previous = NULL;
            name_new->next = NULL;
            if (level+x)
                squeeze_in(object, object_new);
            else 
                object_new->next = NULL;
            sprintf(object_new->name_pointer->name, "%s", entry->d_name);
            object_new->type = 'f';
            x++;
        }
        
        if(size)
        free(full_path);
    } while ((entry = readdir(dir)));
    
    

    // cleanup
	
	if(dir) closedir(dir);
    
    if(!x && !level){
        free_linked_list(firstItemInDartSync);
        firstItemInDartSync = NULL;
    }
    
    if(object_new && !object_new->name_pointer){
        free_linked_list(object_new);
    }
    
    if(name) free(name);
    name = NULL;

    return 1;
}


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  crawlAndUpdate
 *  Description:  Call start_traversal and creates new config file (stores version number)
 * =====================================================================================
 */
int crawlAndUpdate() {
    
    if (t_conn<0){
        printf("\nHaven't set tconn yet");
        return -1;
    }
    
    if(port<0){
        port = find_port(t_conn);
    }
    
    
    char dir[BUFSIZ];
    memset(dir, 0, BUFSIZ);
    
    getDartSyncPath_fs(dir);

    // For given path 'dir', recursively obtains file and directory names
    // and stores it as file_node_t linked list in global variable localFileListHead
    if (start_traversal() < 0)
        return -1;
    file_node_t* head = localFileListHead; // NULL if no files or directories

    
    // setup SUPER_DUPER_SECRET_STORAGE and open its config file
    char s[BUFSIZ*100];
    memset(s, 0, BUFSIZ*100);
    char delimiter = '/';
    sprintf(s, "%s%c%s", dir, delimiter, STORAGE);
    mkdir(s, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    memset(s, 0, BUFSIZ*100);
    sprintf(s, "%s%c%s%cconfig", dir, delimiter, STORAGE, delimiter);
    FILE* config = fopen(s, "r");
    char* filestring = NULL;
    
    // if config file and local file list exist, update local file list's version numbers with corresponding entry
    // from config file
    if (config && head && head->name){

        // read config file (serialized version of current global file list) into 'filestring' and deserialize it
        fseek(config, 0L, SEEK_END);
        long bytes = ftell(config);
        fseek(config, 0L, SEEK_SET);
        filestring = (char*)malloc(sizeof(char)*(bytes+BUFSIZ));
        memset(filestring, 0, sizeof(char)*(bytes+BUFSIZ));
        fread(filestring, sizeof(char), bytes, config);
        fclose(config);
        file_node_t* config_head = deserializeFileList(filestring);
        file_node_t* tofree = config_head;
        file_node_t* current = NULL;

        if(!strlen(filestring)){
            current = NULL;
        } else {
            current = head; //of local file list
        }

        // for each local file or directory, find corresponding entry in global file list and update
        // local file's version number
        while (current) {
            tofree = config_head;
            while (tofree && strcmp(current->name, tofree->name)) {
                tofree = tofree->next;
            }
            if(!tofree) {
                current = current->next;
                break;
            }
           
			current->version_number = tofree->version_number;
            current = current->next;
        }

        // cleanup
        free_linked_list(tofree);
        free(filestring);
    }
    
    char* string = serializeFileList(head);
    //if local file head exists, rewrite config
    if(head && head->name){
        write_config(string);
    }

    if(globalFileListHead){
        free_linked_list(globalFileListHead);
        globalFileListHead = NULL;
    }
    

    // set global file list to local file list
    if(head && !head->name)
        globalFileListHead = NULL;
    else
        globalFileListHead = deserializeFileList(string);
    
    free_linked_list(head);
    head = NULL;
    
    if(string)
        free(string);
    
    return 1;
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
        
        if(current->numPeers > 0 && current->newpeerip){
            sprintf(peers, "%s", current->newpeerip[0]);
        }
  
        for (long i = 1; WHO_AM_I && i < current->numPeers; i++) {
            sprintf(peers+strlen(peers), ",%s", current->newpeerip[i]);
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
        //printf("DeserializeFileList received null\n");
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
        if(current->numPeers > 0){
            current->newpeerip  = (char**)malloc(sizeof(char*)* current->numPeers);
        } else current->newpeerip = NULL;
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
	
	if(WHO_AM_I)
		return;
	
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
 *         Name:  monitor_files
 *  Description:  crawls files every second and sends FILE_UPDATE segment to tracker if needed
 * =====================================================================================
 */
int monitor_files() {
    
   if (pthread_mutex_lock(&file_list_lock)) {
        perror("\tpthread_mutex_lock\n");
        return -1;
    }
    char* current = serializeFileList(globalFileListHead);
    pthread_mutex_unlock(&file_list_lock);
    
    while (1) {
        
        /*  
        if(num_dl_threads){
            pthread_mutex_unlock(&dl_num_lock);
            continue;
        }

        if(dlPeerTableHead){
            pthread_mutex_unlock(&dl_num_lock);
            continue;
        }
        
        */
        if (pthread_mutex_lock(&file_list_lock)) {
            perror("\tpthread_mutex_lock\n");
            if(current){
                free(current);
                current = NULL;
            }
            //pthread_mutex_unlock(&dl_num_lock);
            return -1;
        }


         if(pthread_mutex_lock(&dl_num_lock)){
            perror("\tpthread_mutex_lock\n");
            if(current){
                free(current);
                current = NULL;
            }
            pthread_mutex_unlock(&file_list_lock);
            return -1;
        }

        if(num_dl_threads){
            printf("\n[%s/%d] MONITORING: File change modifications going on. Backing off. %d\n", 
                __FILE__, __LINE__, num_dl_threads);
            pthread_mutex_unlock(&dl_num_lock);
            pthread_mutex_unlock(&file_list_lock);
            sleep(1);
            continue;
        }

        /*  if(dlPeerTableHead){
            pthread_mutex_unlock(&dl_num_lock);
            pthread_mutex_unlock(&file_list_lock);
            continue;
        } */
        
        // crawl and update root directory
        if( crawlAndUpdate() < 0){
            pthread_mutex_unlock(&dl_num_lock);
            pthread_mutex_unlock(&file_list_lock);
            sleep(1);
            continue;
        }
        
        char* newFileList = serializeFileList(globalFileListHead);
        
        // sends new list to tracker in FILE_UPDATE segment if needed
        if(current){
            if(newFileList && strcmp(newFileList, current))
                send_fileupdate(newFileList);    
            else if(!newFileList)
                send_fileupdate(newFileList);
        }
		else if(newFileList){
                send_fileupdate(newFileList);
        }

        // cleanup
        if(current){
            free(current);
            current = NULL;
        }
        current = newFileList;
        
        pthread_mutex_unlock(&dl_num_lock);
        pthread_mutex_unlock(&file_list_lock);
       
        sleep(1);
    }

    if(current){
        free(current);
        current = NULL;
    }
    return 1;
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
    
	
	if(differences->common_files)
		do_version_comparisons_commmon(&differences->common_files);
	if(differences->deleted_head)
		do_version_comparisons_deleted(&differences->deleted_head);
	if(differences->modified_head)
		do_version_comparisons_modified(&differences->modified_head);
	
    addFileNodesToList(differences->new_head);

    char* s = serializeFileList(globalFileListHead);
    
    if(!WHO_AM_I) //if not tracker write file
        write_config(s);

    // cleanup
    free_linked_list(globalFileListHead);
    globalFileListHead = deserializeFileList(s);
    free(s);
    return;
}




/*
 this function removes files that have lower version numbers
 from our list
*/
void do_version_comparisons_modified(file_node_t** head_of_mod_p){
	
	file_node_t* head_of_mod = * head_of_mod_p;
	
	
    if(!head_of_mod)
        return;
    
    if(!globalFileListHead)
        return;
    
    
    file_node_t* current_our = globalFileListHead;
    file_node_t* current_other = head_of_mod;

    //loop through other file table
    while (current_other) {
        while (current_our && strcmp(current_our->name, current_other->name)) {
            current_our = current_our->next;
        }
        
        if(!current_our){
            printf("shit in comparisons modified");
            break;
        }
        
        // if other < our: ignore file since it means we are up to date
        if(current_other->version_number < current_our->version_number){
            file_node_t* n = current_other->next;
		
            removeFromLinkedList(&head_of_mod, current_other);
            free(current_other);
            current_other = NULL;
            current_other = n;
        
        } else if(current_other->version_number >= current_our->version_number){
            
            // if other == our: we are up to date
           
			
			if(WHO_AM_I){
				
				
				long num = current_our->numPeers;
				
				for(long i = 0; i < num; i++){
					free(current_our->newpeerip[i]);
					current_our->newpeerip[i] = NULL;
				}
				
				if(current_our->newpeerip)free(current_our->newpeerip);
				current_our->newpeerip = NULL;
				
				current_our->newpeerip = (char**)malloc(sizeof(char*) * 1);
				
				long size = strlen(current_other->newpeerip[0])+SHUS_CONSTANT;
				current_our->newpeerip[0] = calloc(size, sizeof(char));
				sprintf(current_our->newpeerip[0], "%s", current_other->newpeerip[0]);
				
				current_our->numPeers = 1;
				current_our->version_number++;
				current_our->size = current_other->size;
				current_our->type = current_other->type;
				current_our->checksum = current_other->checksum;
				
			}
			
			 current_other = current_other->next;
			
        
        }
        
    }

    
	*head_of_mod_p = head_of_mod;
    
}



/*
 * ===  FUNCTION  ======================================================================
 *         Name:  do_version_comparisons
 *  Description:  Bumps up our version number
 ===================================================================================== */
void do_version_comparisons_commmon(file_node_t** head_of_common_p){
	
	
    file_node_t* head_of_common = *head_of_common_p;
	
    if(!head_of_common)
        return;
    
    if(!globalFileListHead)
        return;
    
    
    
    file_node_t* current_our = globalFileListHead;
    
    for (file_node_t* current_other = head_of_common; current_other; current_other = current_other->next) {
        
        
        while (current_our->name && strcmp(current_our->name, current_other->name)) {
            current_our = current_our->next;
        }
        
        if(!current_our){
            printf("Shit comparisons\n");
            continue ;
        }
        
		if (current_our->version_number < current_other->version_number){
			current_our->version_number = current_other->version_number;
		}
        
    }
    
	*head_of_common_p = head_of_common;
	

    
}

/*
 * ===  FUNCTION  ======================================================================
 *         Name:  do_version_comparisons
 *  Description:  Ignores deletion requests for files that are ahead of tracker version
 ===================================================================================== */
void do_version_comparisons_deleted(file_node_t** head_of_deleted_p){
	
	file_node_t* head_of_deleted = *head_of_deleted_p;
    
    if(!head_of_deleted)
        return;
    
    if(!globalFileListHead)
        return;
    
    
    
    file_node_t* current_our = globalFileListHead;
    
    for (file_node_t* current_other = head_of_deleted; current_other; current_other = current_other->next) {
        
        
        while (current_our && strcmp(current_our->name, current_other->name)) {
            current_our = current_our->next;
        }
        
        if(!current_our){
            printf("Shit comparisons\n");
            continue ;
        }
        
        if(current_other->version_number < current_our->version_number){
            file_node_t* n = current_other->next;
            removeFromLinkedList(&head_of_deleted, current_other);
            free(current_other);
            current_other = NULL;
            current_other = n;
        } else if(current_other->version_number >= current_our->version_number){
			
			file_node_t* n = current_our->next;
			removeFromLinkedList(&globalFileListHead, current_our);
			free(current_our);
			current_our = NULL;
			current_our = n;
			
        }
        
        if(!current_other)break;
        
    }
	
	*head_of_deleted_p = head_of_deleted;
    
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
            if (bs_string(ours->numPeers, ours->newpeerip, update->newpeerip[i]) < 0){
                
                
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
 *  Description:  Removes a given ip from suppplied linked list
 * =====================================================================================*/


void removeIPfromLinkedList(file_node_t* head_of_list, char* ip){
    
    if(!head_of_list||!ip)
        return;
    

    for(file_node_t* current = head_of_list;current; current = current->next)
		remove_ip(current, ip);

}



int remove_ip(file_node_t* current, char* ip){
	long num = current->numPeers;
	qs_string(current->newpeerip, 0, num-1);
	long pos = bs_string(num, current->newpeerip, ip);
	
	if(pos < 0){
		printf("The ip %s could not be found in current node", ip);
		return -1;
	}
	
	char** newPeerList = NULL;
	
	if(current->numPeers > 1){
		newPeerList = (char**)malloc(sizeof(char*)*current->numPeers);
	}
	
	long count  = 0;
	
	for(long i = 0; i < num; i++){
		if(pos == i){
			free(current->newpeerip[i]);
			current->newpeerip[i] = NULL;
			continue;
		}
		newPeerList[count++] = current->newpeerip[i];
	}
	current->numPeers = (unsigned int) count;
	
	
	
	free(current->newpeerip);
	current->newpeerip = newPeerList;
	
	return 1;
}



// -------------------------------------------  SUPPORT FUNCTIONS
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
	file_node_t* current = list;
    long size = 0;
    if(!(current && current->name)){
        return -1;
    }
    while(current){
        current = current->next;
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

/* sends passed file list to tracker in FILE_UPDATE segment */
void send_fileupdate(char* newFileList){
    ptp_peer_t* segment = (ptp_peer_t*)malloc(sizeof(ptp_peer_t));
    
	if (!newFileList){
        segment->type = FILE_UPDATE;
        segment = (ptp_peer_t*)malloc(sizeof(ptp_peer_t));
        segment->type = FILE_UPDATE;
        segment->port = port;
        get_my_ip(segment->peer_ip);
        segment->file_table = NULL;
        
        printf("[%s/%d] FILEMONITORING: Sending out this peer's file table.\n%s\n",
               __FILE__, __LINE__, segment->file_table);
        
        if (send_struct(t_conn, segment,PTP_PEER_T) < 0){
            printf("\nFile monitor failed to send out update\n");
        }
        
    } else{
        segment->type = FILE_UPDATE;
        segment->port = port;
        get_my_ip(segment->peer_ip);
        segment->file_table = calloc(strlen(newFileList) + 1, sizeof(char));
        MALLOC_CHECK(stderr, segment->file_table);
        strcpy(segment->file_table, newFileList);
        
        printf("[%s/%d] FILEMONITORING: Sending out this peer's file table.\n%s\n",
               __FILE__, __LINE__, segment->file_table);
        
        if (send_struct(t_conn, segment,PTP_PEER_T) < 0){
            printf("\nFile monitor failed to send out update\n");
        }
        
    }
    free(segment->file_table);
    free(segment);
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


