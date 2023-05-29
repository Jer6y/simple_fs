#ifndef SYS_API_H
#define SYS_API_H
#include <stdint.h>
#define SAVE_FILE_NAME "disk.sfs"

// system
void sfs_system_init();
void sfs_system_close();
void sfs_system_info();


// directory
void sfs_ls();
void sfs_cd(char *name);

// file
int sfs_create(char* name,uint16_t type);
int sfs_remove(char* name,uint16_t type);
int sfs_open(char *name);
int sfs_close(int fileID);
int sfs_write(int fileID, char *buf, int length);
int sfs_read(int fileID, char *buf, int length);

int sfs_mklink(char * src,char* des);
void split(char* str,char key_word,char (*store)[16],int* size);

#endif
