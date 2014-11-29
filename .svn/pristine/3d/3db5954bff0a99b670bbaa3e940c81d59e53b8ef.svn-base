#include "qs.h"
void replace_c(char* s, char c, char r);
void fix1(char** x,long first, long last);
void fix2(char** x,long first, long last);

long  bs_number(long n, long array[],  long  to_find){
	if (!array) return -1;
    long ia, ib, imid, index, found;
    ia = found = 0;
    ib = n-1;
    index = -1;
    while (!found && (ia <= ib)) {
        imid = (ia + ib)/2;
        if(to_find == array[imid]) {
            found++;
            index = imid;
        }
        else if (to_find < array[imid])
            ib = imid-1;
        else
            ia = imid+1;
    }
    return(index);
}


long bs_string(long n, char** array, char* to_find){
	if (!array) return -1;
    long ia, ib, imid, index, found;
    
    
    replace_c(to_find, '/', '\\');
    fix1(array, 0, n-1);
    
    
    ia = found = 0;
    ib = n-1;
    index = -1;
    
    while (!found && (ia <= ib)) {
        imid = (ia + ib)/2;
        if( !strcmp(to_find, array[imid])) {
            found++;
            index = imid;
        }
        else if (strcmp(to_find, array[imid]) < 0)
            ib = imid-1;
        else
            ia = imid+1;
    }
    
    fix2(array, 0, n-1);
    replace_c(to_find, '\\', '/');
    
    return(index);
}


void fix1(char** x,long first, long last){
    
    for(long i = 0; i <= last; i++){
        replace_c(x[i], '/', '\\');
    }

}

void fix2(char** x,long first, long last){
    
    for(long i = 0; i <= last; i++){
        replace_c(x[i], '\\', '/');
    }
    
}


void qs_string(char** x,long first, long last){
    
	if (!x) return;
    
    fix1(x, first, last);
    
    long pivot,j,i;
    char* temp;
    if(first<last){
        pivot=first;
        i=first;
        j=last;
        while(i<j){
            while(strcmp(x[i], x[pivot]) <= 0 && i<last)
                i++;
            while(strcmp(x[j], x[pivot]) > 0)
                j--;
            if(i<j){
                temp=x[i];
                x[i]=x[j];
                x[j]=temp;
            }
        }
        temp=x[pivot];
        x[pivot]=x[j];
        x[j]=temp;
        qs_string(x,first,j-1);
        qs_string(x,j+1,last);
    }
    
    fix2(x, first, last);
}


void qs_number(long* x, long first, long last) {
	if (!x) return;
    long pivot, j, i;
    long temp;
    if(first<last){
        pivot=first;
        i=first;
        j=last;
        while(i<j){
            while(x[i] <= x[pivot] && i<last)
                i++;
            while( x[j] > x[pivot])
                j--;
            if(i<j){
                temp=x[i];
                x[i]=x[j];
                x[j]=temp;
            }
        }
        temp=x[pivot];
        x[pivot]=x[j];
        x[j]=temp;
        qs_number(x,first,j-1);
        qs_number(x,j+1,last);
    }
}



/* Find character c in string s and replace it will null */
void replace_c(char* s, char c, char r){
    while (*s != '\0') {
        if(*s == c) *s = r;
        s++;
    }
}
