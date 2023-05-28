#ifndef SFS_API_UTIL
#define SFS_API_UTIL
#include "sfs_header.h"

void FAT_init(FileAllocationTable *fat);
void Super_Block_init(super_block* sb);
void RootEntry_init(FD_Room * entry);

//FAT 磁盘分配器
int FAT_getFreeNode(FileAllocationTable *fat);
void FAT_freeNode(FileAllocationTable *fat,uint16_t id);

//分配表 分配器
int FileDescriptor_allocFile(FD_Room* entry);
int FileDescriptor_allocDir(FD_Room* entry);
int FileDescriptor_alloclink(FD_Room* entry);
FileDescriptor* GetFileDescriptor(FD_Room* entry,int id);
void FileDescriptor_destory(FileDescriptor* file);
void FileDescriptor_free(FD_Room* entry,int id);

#endif