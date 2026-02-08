#include <stdio.h>
#include <stdlib.h>

//Assignment 3
void swap(int *a, int *b) //copied from swap.c in class
{
    int temp;
    temp = *a;
    *a = *b;
    *b = temp;
}

//Assignment 6
int str_length(char *str) {
    char *p = str;
    while (*p != '\0') {
        p++;
    }
    return (int)(p - str);
}

void string_pointer_test()
{
    char* str_ptr = "dear whoever";

  /*  for (int i = 0; str_ptr[i] != '\0'; i++) {
        printf("%d", str_ptr[i]);//saved this bc i liked it :D
    }*/

    for (int i = 0; str_ptr[i] != '\0'; i++) {
        printf("%c.", str_ptr[i]);
    }

    int len = str_length(str_ptr);
    printf("\nDear whoever lenght: %d , and should be 12\n", len);
}

int main()
{
    printf("Beginning");
    printf("\n--- Assignment 1 ---\n");

    int integer = 5;
    int* integer_pointer = &integer;

    printf("\nInt from address: %p\nInt from int :) : %d", integer_pointer, integer);

    *integer_pointer = 10;
    printf("\nInt from address after change: %p\nInt from int after change :) : %d", integer_pointer, integer);

    printf("\n--- Assignment 2 ---\n");
    int arr[5] = {1, 2, 3, 4, 5};
    int *p = arr;

    printf("Array using pointer:\n");
    for (int i = 0; i < 5; i++) {
        printf("Pointer values - %d \n", *(p + i));
    }
    printf("\n");

    //changing values
    for (int i = 0; i < 5; i++) {
        *(p + i) = rand()%454;
    }

    for (int i = 0; i < 5; i++) {
        printf("Array name - %d \n", arr[i]);
    }

    for (int i = 0; i < 5; i++) {
        printf("With pointer - %d \n", *(p + i));
    }
    printf("\n");

    printf("\n--- Assignment 3 ---\n");
    int a = 10;
    int b = 7;
    printf("A before swap: %d\nB before swap: %d\n", a, b);
    swap(&a, &b);
    printf("A after swap: %d\nB after swap: %d\n", a, b);
    printf("\n--- Assignment 4 ---\n");

    int c = 42;
    int *ptr = &c;
    int **double_ptr = &ptr;

    printf("Value: %d\n", c);
    printf("Pointer 1 points to: %d\n", *ptr);
    printf("Pointer 2 points to: %d\n", **double_ptr);
    printf("\n--- Assignment 5 ---\n");

    int *num = malloc(sizeof(int));
    if (num == NULL) {
        printf("Int fail\n");
        return 1;
    }

    *num = 100;
    printf("Created int: %d\n", *num);

    int *array = malloc(5 * sizeof(int));
    if (array == NULL) {
        printf("Array fail\n");
        free(num); //it should have been created by now so free-ing that as well
        return 1;
    }

    for (int i = 0; i < 5; i++) {
        *(array + i) = rand()%57;
    }

    printf("Created array:\n");
    for (int i = 0; i < 5; i++) {
        printf("%d: %d \n",i, *(array + i));
    }
    printf("\n");

    free(array);
    free(num);

    printf("\n--- Assignment 6 ---\n");
    string_pointer_test();
    char str[100];
    printf("Input your string: ");
    scanf("%s", str);
    printf("%d",str_length(str));
    printf("\n--- Assignment 7 ---\n");
    char str1[] = "Hello";
    char str2[] = "Hi";
    char str3[] = "Bye"; //to be bye bye
    char *all_strings[3];
    all_strings[0] = str1;
    all_strings[1] = str2;
    all_strings[2] = str3;
    printf("Before-\n");
    for (int i = 0; i < 3; i++) {
        printf("%s\n", *(all_strings + i));
    }
    all_strings[2] = "Bye Bye";
    printf("After-\n");
    for (int i = 0; i < 3; i++) {
        printf("%s\n", *(all_strings + i));
    }


    return 0;
}
