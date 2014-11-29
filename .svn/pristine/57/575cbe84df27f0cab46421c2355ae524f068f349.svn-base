/*
 * =====================================================================================
 *
 *       Filename:  encrypt.h
 *
 *    Description:  Compile AES library
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

#ifndef  encrypt_INC
#define  encrypt_INC

// ---------------- Prerequisites e.g., Requires "math.h"

// ---------------- Constants

// ---------------- Public Variables

// ---------------- Functions

/* Returns expected size of output file. Next multiple of 16 */
unsigned long get_length(unsigned long len);

/* Encrypts a string. Takes pointer to input and original string length, allocates space for output, puts length of output in length pointer.
 * Key and IV (initialization vector) can be anything as long as they are consistent. 
 */
char* encrypt_string(char *original, unsigned long int *length);

/* Decrypts a string. Takes pointer to input and encrypted string length, allocates space for output, puts length of output in length pointer.
 * Key and IV (initialization vector) can be anything as long as they are consistent.
 */
char* decrypt_string(char *encrypted, unsigned long int *length);

#endif   /* ----- #ifndef encrypt_INC  ----- */
