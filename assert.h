#ifndef ASSERT_H
#define ASSERT_H
void panic(char* name);
void assert_fail(char * file,int line);
#define assert(x) if(!(x)) {assert_fail(__FILE__,__LINE__);}

#endif