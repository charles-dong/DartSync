#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* TCP protocol checksum applied to a file. Returns 0 if file read error.*/
unsigned short checksum(char* filename) {
	unsigned long sum = 0;

	FILE *f;
	f = fopen(filename, "r");
	if (f == NULL) {
		printf("Error reading file %s\n", filename);
		return 0;
	}

	fseek(f,0,SEEK_END);
	long fileLen = ftell(f);
	fseek(f,0,SEEK_SET);

	char* buffer = (char *)malloc(2);

	while (fileLen > 1) {
		memset(buffer, 0, 2);
		fread(buffer, 2, 1, f);
		sum += *buffer;
		fileLen -= 2;
	}

	if (fileLen > 0) {
		memset(buffer, 0, 2);
		fread(buffer, 1, 1, f);
		sum += *buffer;
	}

	while (sum>>16) {
      sum = (sum & 0xFFFF) + (sum >> 16);
    }

    fclose(f);
    free(buffer);
    return ((unsigned short) ~sum);
}
