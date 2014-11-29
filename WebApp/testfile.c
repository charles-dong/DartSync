#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

int main(){
	srand(time(NULL));
	while (1) {
		int status = remove("testfile.txt");
		if (status == 0) 
			printf("Deleted test file\n");
		FILE *f = fopen("testfile.txt", "a+");
		char buffer[100];
		int num1 = rand()%100;
		int num2 = rand()%100;
		int num3 = rand()%100;
		int num4 = rand()%100;
		int num5 = rand()%100;

		time_t num6;
		time(&num6);
		memset(buffer, 0, 100);
		sprintf(buffer, "%d\n%d\n%d\n%d\n%d\n%d", num1, num2, num3, num4, num5, (int)num6);
		fwrite(buffer, sizeof(char), sizeof(buffer), f);
		printf("Wrote test file\n");
		fclose(f);
		sleep(5);
	}
	return 0;
}
