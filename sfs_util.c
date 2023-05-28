
#include "sfs_util.h"
#include "disk_emu.h"
#include "assert.h"
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

int fatID = 0;

// 初始化FAT，将FAT中的所有节点设置为自由节点
void FAT_init(FileAllocationTable *fat)
{
	// 除了第0个和第1个块，其它全部块初始化均为-1代表没有被分配
	for (int i = 0; i < MAX_DISK; i++)
	{
		fat->table[i] = -1;
	}
	
}
void Super_Block_init(super_block* sb)
{
	sb->BytsPerSec = 512;
	sb->SecPerClus = 8;
	sb->ClusTot = MAX_DISK;
	sb->EntryFat =1;
	sb->EntryFd  =2;
	sb->EntryData = 4;
	strcpy(sb->name,"Jerry Luo");
	sb->MagicNumber = 0x55aa;
}

void RootEntry_init(FD_Room * entry)
{
	for(int i=0;i<128;i++)
	{
		(entry->fds)[i].type = 0xff;
	}
	int root_id=FileDescriptor_allocDir(entry);
	FileDescriptor* root = GetFileDescriptor(entry,root_id);
	root->filename[0] ='/';
	root->filename[1] =0;
	int cur_id = FileDescriptor_alloclink(entry);
	FileDescriptor* cur = GetFileDescriptor(entry,cur_id);
	cur->filename[0] ='.';
	cur->filename[1] =0;
	cur->ref_to = root_id;
	int father_id = FileDescriptor_alloclink(entry);
	FileDescriptor* father = GetFileDescriptor(entry,father_id);
	father->filename[0]='.';
	father->filename[1]='.';
	father->filename[2]=0;
	father->ref_to = root_id;
	root->ref +=2;
	root->dir_fds[0] =cur_id;
	root->dir_fds[1] =father_id;
}
// 获取一个自由块，代表
int FAT_getFreeNode(FileAllocationTable *fat)
{
	
	int i;
	for (i = 4; i < MAX_DISK; i++)
	{
		if (fat->table[i] == -1)
		{
			fat->count++;		// 被占用块 + 1
			return i; 			// 返回第i个块
		}
	}
	
	fprintf(stderr, "Error: Cannot get free block in FAT.\n");
	// exit(0);
	return -1;
}
void FAT_freeNode(FileAllocationTable *fat,uint16_t id)
{
	fat->table[id] =0xffff;
	fat->count --;
}

// 在内存缓冲区中释放掉一个file
// int FAT_freeFile(FileAllocationTable *fat,FileDescriptor* file)
// {	
// 	if(file ==0 ) return -1;
// 	switch(file->type)
// 	{
// 		case 0:
// 		if(file->ref ==0)
// 		{
// 			int start = file->fat_index;
// 			while((fat->table)[start]!=start)
// 			{
// 				int tmp = (fat->table)[start];
// 				(fat->table)[start]=-1;
// 				(fat->count)--;
// 				start = tmp;
// 			}
// 			(fat->table)[start]=-1;
// 			(fat->count)--;
// 			FileDescriptor_destory(file);
// 		}
// 		else
// 		{
// 			file->type = 4;
// 		}
// 		break;
// 		case 1:
// 		int empty = 1;
// 		for(int i=0;i<12;i++)
// 		{	
// 			if(file->dir_fds[i]!=0xffff)
// 			{
// 				empty =0;
// 				break;
// 			}
// 		}
// 		if(!empty)
// 		{
// 			fprintf(stderr, "Error: Dir not empty.\n");
// 			return -1;
// 		}
// 		file->type = 0xff;
// 		break;
// 		case 2:

// 		break;
// 	}
// 	// fprintf(stderr, "Error: Cannot get free block in FAT.\n");
// 	// exit(0);
// 	return -1;
// }




// // 初始化目录表
// void DirectoryDescriptor_init(DirectoryDescriptor *root)
// {
// 	// 将目录的计数初始化为0
// 	root->count = 0;
// }


/*
int DirectoryDescriptor_getFreeSpot(DirectoryDescriptor *root)
{

	if (++root->count < FILE_LIST_SIZE)
	{
		return root->count;
	}
	else
	{

		int i;
		for (i = 0; i < MAX_FILE; i++)
		{
			if (root->table[i].size == EMPTY)
			{
				return i;
			}
		}
	}
	root->count--;
	fprintf(stderr, "Error: Cannot get free block in DD.\n");
	return -1;
}
*/

void FileDescriptor_destory(FileDescriptor* file)
{
	assert(file->ref==0);
	if(file->type==2)
	{
		assert(file->ref_to==0xffff);
	}
	file->type =0xff;
	for(int i=0;i<12;i++)
	file->dir_fds[i]= 0xffff;
	file->fat_index = 0xffff;
	file->timestamp = 0;
	file->fas.opened =0;
	memset(file->filename,0,16);
	file->ref=0;
	file->size=0;
	file->ref_to =0xffff;
}




int FileDescriptor_allocFile(FD_Room* entry)
{
	for(int i=0;i<FILE_LIST_SIZE;i++)
	{
		if((entry->fds)[i].type==0xff)
		{
			(entry->fds)[i].type = 0;
			(entry->fds)[i].size = 0;
			(entry->fds)[i].fas.opened = 0;
			(entry->fds)[i].ref = 0;
			(entry->fds)[i].fat_index = 0xffff;
			(entry->fds)[i].timestamp = time(NULL);
			return i;
		}
	}
	return -1;
}



int FileDescriptor_allocDir(FD_Room* entry)
{
	for(int i=0;i<FILE_LIST_SIZE;i++)
	{
		if((entry->fds)[i].type==0xff)
		{
			(entry->fds)[i].type = 1;
			(entry->fds)[i].size = 0;
			(entry->fds)[i].fas.opened = 0;
			(entry->fds)[i].ref = 0;
			for(int j=0;j<12;j++)
			{
				(entry->fds)[i].dir_fds[j] =0xffff;
			}
			(entry->fds)[i].timestamp = time(NULL);
			return i;
		}
	}
	return -1;
}

int FileDescriptor_alloclink(FD_Room* entry)
{
	for(int i=0;i<FILE_LIST_SIZE;i++)
	{
		if((entry->fds)[i].type==0xff)
		{
			(entry->fds)[i].type = 2;
			(entry->fds)[i].size = 0;
			(entry->fds)[i].fas.opened = 0;
			(entry->fds)[i].ref = 0;
			(entry->fds)[i].ref_to=0xffff;
			(entry->fds)[i].timestamp = time(NULL);
			return i;
		}
	}
	return -1;
}
FileDescriptor* GetFileDescriptor(FD_Room* entry,int id)
{
	assert((id>=0 && id<128));
	return &((entry->fds)[id]);
}
void FileDescriptor_free(FD_Room* entry,int id)
{
	assert((id>=0 && id<128));
	(entry->fds)[id].type = 0xff;
}