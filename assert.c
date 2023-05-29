#include "assert.h"
#include <stdio.h>
void assert_fail(char * file,int line)
{
    printf("\n===assert====\n");
    printf("==file:%s==\n",file);
    printf("==line:%d==\n",line);
    perror("assert fail!\n");
    while(1);
}
void panic(char *name)
{
    printf("\n===panic====\n");
    printf("==why:%s==\n",name);
    while(1);
}