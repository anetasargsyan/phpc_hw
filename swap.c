#include <stdio.h>


void populateCArray(int* c, int* a, int* b, int size){
    for(int i=0; i < size; i++){
        c[i] = a[i] + b[i];
        printf("c[%d] = %d\n", i, c[i]);
    }
}
#define SIZE 1000
int main(){
    int a[SIZE], b[SIZE], c[SIZE];
    for(int i=0; i<SIZE; i++){
        a[i] = 5;
        b[i] = 1000 - i;
    }

    populateCArray(c, a, b, SIZE);
    return 0;
}

void swap(int *a, int *b){
    int temp;
    temp = *a;
    *a = *b;
    *b = temp;
}
