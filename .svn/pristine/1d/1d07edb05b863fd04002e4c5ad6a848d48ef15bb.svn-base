// Make library: cd aes; gcc -c aescrypt.c aeskey.c aes_modes.c aestab.o; ar rcs libaes.a *.o
// Compile: gcc -ggdb encrypt.c aes/libaes.a

#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include "encrypt.h"
#include "aes/aes.h"

char key[] = "iruvyusooooomuch";
char iv[16] = "ethanandshuhugg";

unsigned long get_length(unsigned long len) {
    while (len % 16 != 0) len++; 
    return len;
}

char* encrypt_string(char *original, unsigned long int *length) {
    aes_encrypt_ctx ctx[1];
    unsigned char vector[16];
    memcpy(vector, iv, 16);
    aes_encrypt_key128((const unsigned char*)key, ctx);
    *length = get_length(*length);

    char* encrypted = (char *)calloc(*length, 1);

    aes_cbc_encrypt((const unsigned char *)original, (unsigned char *)encrypted, (int)*length, vector, ctx);
    return encrypted;
}

char* decrypt_string(char *encrypted, unsigned long int *length) {
    aes_decrypt_ctx ctx[1];
    unsigned char vector[16];
    memcpy(vector, iv, 16);
    aes_decrypt_key128((const unsigned char*)key, ctx);
    *length = get_length(*length);

    char *decrypted = (char *)calloc(*length, 1);
    
    aes_cbc_decrypt((const unsigned char *)encrypted, (unsigned char *)decrypted, (int)*length, vector, ctx);
    return decrypted;
}


/* Sample usage from stdin or defaults to Hello World */
/*int main(int argc, char** argv) {
    srand(time(NULL));
    const unsigned char key[] = "my key";

    unsigned char iv[16]; 
    for(int i = 0; i < 16; i++) {
        iv[i] = rand() & 0xFF;
    }

    aes_init();    

    char *original;
    if (argc == 1) {
        printf("You can pass in a string to encrypt as an argument!\n");
        original = (char *)calloc(12, 1);
        strcpy(original, "Hello world!");
    }
    else {
        original = (char *)calloc(strlen(argv[1])+1, 1);
        strcpy(original, argv[1]);
    }

    unsigned long int length = strlen(original)+1;

    printf("Original string of length %lu: %s\n", length, original);

    char *encrypted = encrypt_string(original, &length);
    printf("Encrypted string of length %lu: %s\n", length, encrypted);

    char *decrypted = decrypt_string(encrypted, &length);
    printf("Decrypted string of length %lu: %s\n", length, decrypted);

    if (strcmp(original, decrypted) == 0) {
        printf("Strings match\n");
    }
    else {
        printf("Strings don't match\n");
    }

    return 0;
}*/
