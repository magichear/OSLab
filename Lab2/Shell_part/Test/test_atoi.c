#include<stdio.h>
#include<string.h>
#include<stdlib.h>

void main(){
    char str[4] = {'2','3','.','4'};
    int num = 0;
    num = atoi(str);

    if((double)(num) != atof(str))
        printf("Failed\n");

    system("pause");
}