// Compile with zlib-1.2.8/libz.a

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include "../utils/data_structures.h"


// UncompressLength must be already assigned
char *compress_string(char *original, unsigned long int *uncompressLength, unsigned long int *compressLength) {
    *compressLength = compressBound(*uncompressLength); //upper bound on compressed string length
    char *compressBuf = (char *)calloc(*compressLength, sizeof(char));  
    compress((unsigned char *)compressBuf, compressLength, (unsigned char *)original, *uncompressLength);
    compressBuf = (char*)realloc(compressBuf, *compressLength);
    return compressBuf;
}


// Both uncompressLength and compressLength must be already assigned
char *decompress_string(char *compressed, unsigned long int *uncompressLength, unsigned long int *compressLength) {
    char *uncompressBuf = (char *)calloc(*uncompressLength, sizeof(char));
    uncompress((unsigned char *)uncompressBuf, uncompressLength, (unsigned char *)compressed, *compressLength);
    uncompressBuf = (char*) realloc(uncompressBuf, *uncompressLength);
    return uncompressBuf;
}


/* Stdin de/compress string example */
/*int main(int argc, char** argv) {
    if (argc == 1) {
        printf("Pass string to compress as an argument!\n");
        exit(0);
    }

    unsigned long int uncompressLength;
    unsigned long int compressLength;
    
    uncompressLength = strlen(argv[1])+1;
    printf("Original string of size %lu: %s\n\n", uncompressLength, argv[1]);
    
    char *compressed_string = compress_string(argv[1], &uncompressLength, &compressLength);
    printf("Compressed string of size %lu: %s\n\n", compressLength, compressed_string);

    char *decompressed_string = decompress_string(compressed_string, &uncompressLength, &compressLength);
    printf("Decompressed string of size %lu: %s\n\n", uncompressLength, decompressed_string);

    if (strcmp(argv[1], decompressed_string) == 0) {
        printf("Strings match\n");
    }
    else {
        printf("Strings don't match\n");
    }

    free(compressed_string);
    free(decompressed_string);

    exit(0);
}*/