#include "sfs_api.h"
#include "disk_emu.h"
#include "sfs_header.h"
#include "sfs_util.h"
#include "assert.h"
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

FileDescriptor* cur_dir;
int id_c ;
FD_Room Entry_Root;
FileAllocationTable fat;
super_block meta;

#define FILE_WRITE 0
#define DIR_WRITE 1
#define LINK_WRITE 2

#define FILE_READ 0
#define DIR_READ 1
#define LINK_READ 2
int write_sfs_file(int fd,char* buf,int length);
int write_sfs_dir(int fd,char* buf,int length);
int write_sfs_link(int fd,char* buf,int length);


int read_sfs_file(int fd,char* buf,int length);
int read_sfs_dir(int fd,char* buf,int length);
int read_sfs_link(int fd,char* buf,int length);
int (*write_sfs[])(int,char*,int) = 
{ 
    write_sfs_file,
    write_sfs_dir,
    write_sfs_link
}; 
int (*read_sfs[])(int,char*,int) = 
{ 
    read_sfs_file,
    read_sfs_dir,
    read_sfs_link
}; 
//  0 成功
// -4 链接目标不存在
// -1 链接源不存在
// -2 原链接已经存在
// -3 创建的空间不够
int sfs_mklink(char * src,char* des)
{
    int fd_src;
    int fd_des;
    if((fd_des=sfs_open(des,0))==-1)
    {
        return -4;
    }
    if((fd_src =sfs_create(src,2))<0)
    {
        sfs_close(fd_des);
        return fd_src;
    }
    (Entry_Root.fds[fd_src]).ref_to = fd_des;
    (Entry_Root.fds[fd_des].ref)++;
    sfs_close(fd_des);
    sfs_close(fd_src);
    return 0;
}
int sfs_write(int fileID,char* buf,int length)
{
    assert(fileID>=0 && fileID <128);
    switch(Entry_Root.fds[fileID].type)
    {
        case 0:
        return (write_sfs[FILE_WRITE])(fileID,buf,length);
        break;
        case 1:
        return -1;
        return (write_sfs[DIR_WRITE])(fileID,buf,length);
        break;
        case 2:
        return (write_sfs[LINK_WRITE])(fileID,buf,length);
        break;
        case 4:
        return (write_sfs[FILE_WRITE])(fileID,buf,length);
        default:
        assert(1<0);
    }
    return 0;
}
int sfs_read(int fileID,char* buf,int length)
{
    assert(fileID>=0 && fileID <128);
    
    switch(Entry_Root.fds[fileID].type)
    {
        case 0:
        return (read_sfs[FILE_WRITE])(fileID,buf,length);
        break;
        case 1:
        return -1;
        return (read_sfs[DIR_WRITE])(fileID,buf,length);
        break;
        case 2:
        return (read_sfs[LINK_WRITE])(fileID,buf,length);
        break;
        case 4:
        return (read_sfs[FILE_WRITE])(fileID,buf,length);
        default:
        assert(1<0);
    }
    return 0;
}

void split(char* str,char key_word,char (*store)[16],int* size)
{
    int i =0;
    if(str[0]=='/')
    {
        strcpy(store[i],"/");
        str++;
        i++;
    }
    char * tmp = strtok(str,"/");
    if(tmp !=NULL)
    {
        assert(strlen(tmp)<=15);
        strcpy(store[i++],tmp);
    }
    
    while(tmp)
    {
        tmp = strtok(NULL,"/");
        if(tmp!=NULL)
        {
            assert(strlen(tmp)<=15);
            strcpy(store[i++],tmp);
        }
    }
    *size = i;
    assert((*size) <10);
    return;
}
//如果mode ==0,在当前目录下找文件
//如果mode ==1,在当前目录下找目录
//如果mode ==2,在当前目录下找链接
//如果mode ==3,随意找任何东西
//如果没有找到 返回-1
static int find_(FileDescriptor* now,char* name,int mode)
{
    assert(now->type==1);
    for(int i=0;i<12;i++)
    {
        if((now->dir_fds)[i]!=0xffff)
        {
            switch(Entry_Root.fds[(now->dir_fds)[i]].type)
            {
                case 0:
                if(mode==0 || mode ==3)
                {   
                    if(strcmp(name,Entry_Root.fds[(now->dir_fds)[i]].filename)==0)
                    {
                        return (now->dir_fds)[i];
                    }
                }
                break;
                case 1:
                if(mode ==1 || mode ==3)
                {
                    if(strcmp(name,Entry_Root.fds[(now->dir_fds)[i]].filename)==0)
                    {
                        return (now->dir_fds)[i];
                    }
                }
                break;
                case 2:
                if(mode==0)
                {
                    if(strcmp(name,Entry_Root.fds[(now->dir_fds)[i]].filename)==0 )
                    {
                        if(Entry_Root.fds[Entry_Root.fds[(now->dir_fds)[i]].ref_to].type==0 \
                        || Entry_Root.fds[Entry_Root.fds[(now->dir_fds)[i]].ref_to].type==4)
                        return Entry_Root.fds[(now->dir_fds)[i]].ref_to;
                    }
                }
                else if(mode ==1)
                {
                    if(strcmp(name,Entry_Root.fds[(now->dir_fds)[i]].filename)==0 && Entry_Root.fds[Entry_Root.fds[(now->dir_fds)[i]].ref_to].type==1)
                    {
                        return Entry_Root.fds[(now->dir_fds)[i]].ref_to;
                    }
                }
                else if(mode==2 || mode ==3)
                {
                    if(strcmp(name,Entry_Root.fds[(now->dir_fds)[i]].filename)==0)
                    {
                        return (now->dir_fds)[i];
                    }
                }
                break;
                case 3:
                break;
                case 4:
                break;
                default:
                break;
            }
        }
    }
    return -1;
}
//返回0  创建成功
//返回-1 路径中存在有目录未创建
//返回-2 目标已存在
//返回-3 创建失败 空间不够
// >=0 创建的文件描述符
int sfs_create(char* name,uint16_t type)
{
    FileDescriptor* find_s_fd =0;
    int father_fd =0;
    char store[10][16] ={0};
    int size=0;
    int i=0;
    char cp_name[100]={0};
    strcpy(cp_name,name);
    split(cp_name,'/',store,&size);
    if(strcmp(store[0],"/")==0)
    {
        find_s_fd = &(Entry_Root.fds[i++]);
        father_fd=0;
    }
    else
    {
        find_s_fd = cur_dir;
        father_fd = id_c;
    }
    for(;i<size-1;i++)
    {
        int fd = find_(find_s_fd,store[i],1);
        if(fd ==-1) return -1;
        find_s_fd = GetFileDescriptor(&Entry_Root,fd);
        father_fd = fd;
    }
    int fd = find_(find_s_fd,store[size-1],type);
    if(fd!=-1) return -2;
    switch(type)
    {
        case 0:
        {
            int fileId = FileDescriptor_allocFile(&Entry_Root);
            if(fileId ==-1) return -3;
            FileDescriptor* fileDt = GetFileDescriptor(&Entry_Root,fileId);
            fileDt->fat_index = FAT_getFreeNode(&fat);
            if(fileDt->fat_index == -1) 
            {
                FileDescriptor_destory(fileDt);
                FileDescriptor_free(&Entry_Root,fileId);
                return -3;
            }
            fat.table[fileDt->fat_index] = fileDt->fat_index;
            strcpy(fileDt->filename,store[size-1]);
            int is_free=1;
            for(int i=0;i<12;i++)
            {
                if((find_s_fd->dir_fds)[i]==0xffff)
                {
                    is_free=0;
                    (find_s_fd->dir_fds)[i] = fileId;
                    break;
                }
            }
            if(is_free) 
            {
                FAT_freeNode(&fat,fileDt->fat_index);
                FileDescriptor_destory(fileDt);
                FileDescriptor_free(&Entry_Root,fileId);
                return -3;
            }
            return fileId;
        }
        break;
        case 1:
        {
            int fileId = FileDescriptor_allocDir(&Entry_Root);
            if(fileId ==-1) return -3;
            FileDescriptor* fileDt = GetFileDescriptor(&Entry_Root,fileId);
            strcpy(fileDt->filename,store[size-1]);
            int is_free=1;
            for(int i=0;i<12;i++)
            {
                if((find_s_fd->dir_fds)[i]==0xffff)
                {
                    is_free=0;
                    (find_s_fd->dir_fds)[i] = fileId;
                    break;
                }
            }
            if(is_free) 
            {
                FileDescriptor_destory(fileDt);
                FileDescriptor_free(&Entry_Root,fileId);
                return -3;
            }
            int cur_id = FileDescriptor_alloclink(&Entry_Root);
            if(cur_id ==-1)
            {
                FileDescriptor_destory(fileDt);
                FileDescriptor_free(&Entry_Root,fileId);
                return -3;
            }
            FileDescriptor* cur = GetFileDescriptor(&Entry_Root,cur_id);
            int father_id = FileDescriptor_alloclink(&Entry_Root);
            if(father_id==-1)
            {
                FileDescriptor_destory(cur);
                FileDescriptor_free(&Entry_Root,cur_id);
                FileDescriptor_destory(fileDt);
                FileDescriptor_free(&Entry_Root,fileId);
                return -3;
            }
            FileDescriptor* father = GetFileDescriptor(&Entry_Root,father_id);
            cur->filename[0] ='.';
            cur->filename[1] =0;
            cur->ref_to = fileId;
            father->filename[0]='.';
            father->filename[1]='.';
            father->filename[2]=0;
            father->ref_to = father_fd;
            fileDt->ref ++;
            Entry_Root.fds[father_fd].ref++;
            fileDt->dir_fds[0] =cur_id;
            fileDt->dir_fds[1] =father_id;
            return fileId;
        }
        break;
        case 2:
        {
            int fileId = FileDescriptor_alloclink(&Entry_Root);
            if(fileId ==-1) return -3;
            FileDescriptor* fileDt = GetFileDescriptor(&Entry_Root,fileId);
            strcpy(fileDt->filename,store[size-1]);
            int is_free=1;
            for(int i=0;i<12;i++)
            {
                if((find_s_fd->dir_fds)[i]==0xffff)
                {
                    is_free=0;
                    (find_s_fd->dir_fds)[i] = fileId;
                    break;
                }
            }
            if(is_free) 
            {
                FileDescriptor_destory(fileDt);
                FileDescriptor_free(&Entry_Root,fileId);
                return -3;
            }
            fileDt->ref_to = 0xfffe;
            //初始化待链接
            return fileId;
        }
        break;
        default:
        assert(1<0);
        break;
    }
}

//返回0  删除
//返回-1 目标不存在
//返回-2 目录不为空
int sfs_remove(char* name,uint16_t type)
{
    FileDescriptor* find_s_fd =0;
    int father_fd =0;
    char store[10][16] ={0};
    int size=0;
    int i=0;
    char cp_name[100]={0};
    strcpy(cp_name,name);
    split(cp_name,'/',store,&size);
    if(strcmp(store[0],"/")==0)
    {
        find_s_fd = &(Entry_Root.fds[i++]);
        father_fd=0;
    }
    else
    {
        find_s_fd = cur_dir;
        father_fd = id_c;
    }
    for(;i<size-1;i++)
    {
        int fd = find_(find_s_fd,store[i],1);
        if(fd ==-1) return -1;
        find_s_fd = GetFileDescriptor(&Entry_Root,fd);
        father_fd = fd;
    }
    int fd = find_(find_s_fd,store[size-1],type);
    if(fd==-1) return -1;

    switch(type)
    {
        case 0:
        {
            int fileID = fd;
            FileDescriptor* fileDt = GetFileDescriptor(&Entry_Root,fileID);
            if(fileDt->ref==0)
            {
                int start = fileDt->fat_index;
                while(fat.table[start]!=start)
                {
                    int tmp = start;
                    start = fat.table[start];
                    FAT_freeNode(&fat,tmp);
                }
                FAT_freeNode(&fat,start);
                FileDescriptor_destory(fileDt);
                FileDescriptor_free(&Entry_Root,fileID);
            }
            else
                fileDt->type =4;
            for(int i=0;i<12;i++)
            {
                if((Entry_Root.fds[father_fd]).dir_fds[i] == fileID)
                {
                    (Entry_Root.fds[father_fd]).dir_fds[i] =0xffff;
                    break;
                }
            }
            return 0;
        }
        break;
        case 1:
        {
            int fileID = fd;
            FileDescriptor* fileDt = GetFileDescriptor(&Entry_Root,fileID);
            int is_can_free =1;
            for(int i=0;i<12;i++)
            {
                if((fileDt->dir_fds)[i] !=0xffff)
                {
                    is_can_free =0;
                    break;
                }
            }
            if(!is_can_free) return -2;
            FileDescriptor_destory(fileDt);
            FileDescriptor_free(&Entry_Root,fileID);
            for(int i=0;i<12;i++)
            {
                if((Entry_Root.fds[father_fd]).dir_fds[i] == fileID)
                {
                    (Entry_Root.fds[father_fd]).dir_fds[i] =0xffff;
                    break;
                }
            }
            return 0;
        }
        break;
        case 2:
        {
            int fileID = fd;
            FileDescriptor* fileDt = GetFileDescriptor(&Entry_Root,fileID);
            assert(fileDt->ref_to!=0xffff);
            assert(fileDt->ref ==0);
            Entry_Root.fds[fileDt->ref_to].ref--;
            if(Entry_Root.fds[fileDt->ref_to].ref==0 && Entry_Root.fds[fileDt->ref_to].type==4)
            {
                int start = (Entry_Root.fds[fileDt->ref_to]).fat_index;
                while(fat.table[start]!=start)
                {
                    int tmp = start;
                    start = fat.table[start];
                    FAT_freeNode(&fat,tmp);
                }
                FAT_freeNode(&fat,start);
                FileDescriptor_destory(&Entry_Root.fds[fileDt->ref_to]);
                FileDescriptor_free(&Entry_Root,fileDt->ref_to);
            }
            fileDt->ref_to =0xffff;
            FileDescriptor_destory(fileDt);
            FileDescriptor_free(&Entry_Root,fileID);
            for(int i=0;i<12;i++)
            {
                if((Entry_Root.fds[father_fd]).dir_fds[i] == fileID)
                {
                    (Entry_Root.fds[father_fd]).dir_fds[i] =0xffff;
                    break;
                }
            }
            return 0;
        }
        break;
        default:
        assert(1<0);
    }
}
void sfs_cd(char *name)
{
    if(strcmp(name,"/")==0)
    {
        cur_dir = &(Entry_Root.fds[0]);
        id_c  =0;
        return;
    }
    FileDescriptor* find_s_fd =0;
    char store[10][16] ={0};
    int size=0;
    int i=0;
    char cp_name[100]={0};
    strcpy(cp_name,name);
    split(cp_name,'/',store,&size);
    if(strcmp(store[0],"/")==0)
        find_s_fd = &(Entry_Root.fds[i++]);
    else
        find_s_fd = cur_dir;

    for(;i<size-1;i++)
    {
        int fd = find_(find_s_fd,store[i],1);
        if(fd ==-1)
        {
            printf("目录不存在!\n");
            return;
        }
        find_s_fd = GetFileDescriptor(&Entry_Root,fd);
    }
    int fd = find_(find_s_fd,store[size-1],1);
    int fd_2 = find_(find_s_fd,store[size-1],0);
    int fd_3 = find_(find_s_fd,store[size-1],2);
    if(fd==-1)
    {
        if(fd_2 >=0 || fd_3 >=0) printf("%s 不是一个目录!\n",name);
        else
        {
            printf("目录不存在!\n");
        }
        return;
    }
    cur_dir = &(Entry_Root.fds[fd]);
    id_c  = fd;
}
//-1 不存在
//其他 文件描述符
int sfs_open(char *name,uint16_t type)
{
    if(strcmp(name,"/")==0)
    {
        Entry_Root.fds[0].fas.opened=1;
        return 0;
    }
    FileDescriptor* find_s_fd =0;
    char store[10][16] ={0};
    int size=0;
    int i=0;
    char cp_name[100]={0};
    strcpy(cp_name,name);
    split(cp_name,'/',store,&size);
    if(strcmp(store[0],"/")==0)
        find_s_fd = &(Entry_Root.fds[i++]);
    else
        find_s_fd = cur_dir;

    for(;i<size-1;i++)
    {
        int fd = find_(find_s_fd,store[i],1);
        if(fd ==-1)
        {
            return -1;
        }
        find_s_fd = GetFileDescriptor(&Entry_Root,fd);
    }
    int fd = find_(find_s_fd,store[size-1],type);
    if(fd!=-1) 
    {
        Entry_Root.fds[fd].fas.opened=1;
    }
    return fd;
}
int sfs_close(int fileID)
{ 
    assert(fileID>=0 && fileID<128);
    Entry_Root.fds[fileID].fas.opened=0;
    return 0;
}


int write_sfs_dir(int fd,char* buf,int length)
{
    assert(1>0);
    return 0;
}
int write_sfs_link(int fd,char* buf,int length)
{
    FileDescriptor * this_file = &Entry_Root.fds[fd];
    assert(this_file->type==2);
    assert((this_file->ref==0)&&(this_file->ref_to !=0xffff));
    if(Entry_Root.fds[this_file->ref_to].type==1) return -1;
    return write_sfs_file(this_file->ref_to,buf,length); 
}
//-1失败
//0 成功
int write_sfs_file(int fd,char* buf,int length)
{
    assert(fd>=0 && fd<128);
    FileDescriptor * this_file = &Entry_Root.fds[fd];
    assert(this_file->type ==0);
    if(this_file->fas.opened ==0 ) return -1;
    int16_t node_index = this_file->fat_index;

    // 重新写入文件前，回收该文件所占用的全部空间
    while (node_index != fat.table[node_index])
    {
        fat.table[node_index] = -1;
        fat.count--;
        node_index = fat.table[node_index];
    }
    fat.table[node_index] = -1;
    fat.count--;
    //  写入文件长度
    this_file->size = length;
    // 写入文件
    int block_num = length / NB_BLOCK;
    int res = length % NB_BLOCK;
    // 如果文件的长度不是 块长的整数倍，应该多分配一个块
    if (res > 0)
        block_num += 1;
    
    // 声明一个块
    void *blockWrite = (void *)malloc(NB_BLOCK);
    
    if (blockWrite == 0) return -1;
    // 全部置为0
    int16_t last_node_index = -1;
    memset(blockWrite, 0, NB_BLOCK);
    
    while (length > 0)
    {
        if (length > NB_BLOCK)
        {
            memcpy(blockWrite, (void *)buf, NB_BLOCK);

            int16_t free_node_index = FAT_getFreeNode(&fat);
            assert(free_node_index !=-1);
            ds_write_blocks(free_node_index, 1, blockWrite); // 写入一个块
            if (last_node_index != -1)
            {
                fat.table[last_node_index] = free_node_index;
            }
            else
            {
                last_node_index = free_node_index;
                Entry_Root.fds[fd].fat_index = free_node_index;
            }
            length -= NB_BLOCK; 
            buf += NB_BLOCK; // 指针后移
        }
        else
        {
            memcpy(blockWrite, (void *)buf, length);
            int16_t free_node_index = FAT_getFreeNode(&fat);
            assert(free_node_index !=-1);
            ds_write_blocks(free_node_index, 1, blockWrite); // 写入一个块
            if (last_node_index != -1)
            {
                fat.table[last_node_index] = free_node_index;
            }
            else
            {
                last_node_index = free_node_index;
                Entry_Root.fds[fd].fat_index = free_node_index;
            }
            buf += length;
            length -= length; 
        }
    }
    fat.table[last_node_index] = last_node_index; //标识文件结束
    free(blockWrite);
}

int read_sfs_dir(int fileID, char *buf, int length)
{
    assert(1<0);
    return 0;
}
int read_sfs_link(int fileID, char *buf, int length)
{
    FileDescriptor * this_file = &(Entry_Root.fds[fileID]);
    assert(this_file->type==2);
    assert((this_file->ref==0)&&(this_file->ref_to !=0xffff));
    if(Entry_Root.fds[this_file->ref_to].type==1) return -1;
    // printf("%s ",Entry_Root.fds[this_file->ref_to].filename);
    return read_sfs_file(this_file->ref_to,buf,length); 
}
int read_sfs_file(int fileID, char *buf, int length)
{
    assert(Entry_Root.fds[fileID].type==0||Entry_Root.fds[fileID].type==4);
    if (Entry_Root.fds[fileID].fas.opened == 0)
        return -1;
    
    int8_t read_flag = 1;

    void *blockRead = (void *)malloc(NB_BLOCK);
    // 全部置为0
    int16_t read_node_index = Entry_Root.fds[fileID].fat_index;

    memset(blockRead, 0, NB_BLOCK);

    while (length > 0 )
    {
        if (length > NB_BLOCK)
        {
            ds_read_blocks(read_node_index, 1, blockRead);
            memcpy(buf, blockRead, NB_BLOCK);
            length -= NB_BLOCK;
            buf += NB_BLOCK; // 指针后移
        }
        else
        {
            ds_read_blocks(read_node_index, 1, blockRead);
            memcpy(buf, blockRead, length);
            buf += length; // 指针后移
            length -= length;
        }
        int16_t next_read_node_index = fat.table[read_node_index];
        if (next_read_node_index == read_node_index)
            break;
        else
            read_node_index = next_read_node_index;
    }
    free(blockRead);
    return 0;
}



void sfs_system_info()
{
    printf("\n ==== File System Info ==== \n");
    printf(" ==== made by %s ===\n",meta.name);
    printf(" ==== Sector Size %d ===\n",(int)meta.BytsPerSec);
    printf(" ==== Cluster  %d ===\n",(int)meta.SecPerClus);
    printf(" ==== Magic Number:%x ===\n",(int)meta.MagicNumber);
    printf(" Disk Size %d Bytes\n", NB_BLOCK * MAX_DISK);
    printf(" Already Use %d Bytes, (%.2f)%% \n", NB_BLOCK * fat.count, (fat.count * 100.0f / MAX_DISK));
    printf(" MAX file %d \n",(int)FILE_LIST_SIZE);
}

void sfs_system_init()
{
    int fresh = access(SAVE_FILE_NAME, 0);
    if (fresh)
    {
        // 创建磁盘文件
        ds_init_fresh(SAVE_FILE_NAME, NB_BLOCK, MAX_DISK);
        // 初始化 FAT
        FAT_init(&fat);
        // 初始化 目录
        RootEntry_init(&Entry_Root);
        Super_Block_init(&meta);
        ds_write_blocks(0, 1, (void *)&meta); //  第0个块保存有目录表
        ds_write_blocks(1, 1, (void *)&fat);  // 第1个块保存FAT
        ds_write_blocks(2, 2, (void *)&Entry_Root);  // 第1个块保存FAT
        // 第0-3已经被分配
        fat.table[0] = 0;
        fat.table[1] = 1;
        fat.table[2] = 2;
        fat.table[3] = 3;
        fat.count = 4;
    }
    else
    {
        ds_init(SAVE_FILE_NAME, NB_BLOCK, MAX_DISK);
        ds_read_blocks(0, 1, (void *)&meta);
        ds_read_blocks(1, 1, (void *)&fat);
        ds_read_blocks(2, 2, (void *)&Entry_Root);
    }
    cur_dir = &(Entry_Root.fds[0]);
    id_c =0;
}

void sfs_system_close()
{
    // 系统退出之前关闭所有文件
    for (int i = 0; i < FILE_LIST_SIZE; i++)
    {
        Entry_Root.fds[i].fas.opened=0;
    }

    ds_write_blocks(0, 1, (void *)&meta); //  第0个块保存有目录表
    ds_write_blocks(1, 1, (void *)&fat);  // 第1个块保存FAT
    ds_write_blocks(2, 2, (void *)&Entry_Root);  // 第1个块保存FAT
    ds_close();
}

void sfs_ls()
{
    int i;
    printf("\n");
    for (i = 0; i < 12; i++)
    {
        if (((cur_dir->dir_fds)[i])!=0xffff)
        {
            char type =0;
            switch(Entry_Root.fds[(cur_dir->dir_fds)[i]].type)
            {
                case 0:
                type = '-';
                break;
                case 1:
                type = 'd';
                break;
                case 2:
                type = 'l';
                break;
            }
            float kb = Entry_Root.fds[(cur_dir->dir_fds)[i]].size / 1024.0;
            char *tm = ctime(&((Entry_Root.fds[(cur_dir->dir_fds)[i]]).timestamp));
            tm[24] = '\0';
            printf("\e[97m%c\t%25s\t%.2fKB\t",type,tm, kb);
            switch(Entry_Root.fds[(cur_dir->dir_fds)[i]].type)
            {
                case 0:
                printf("\e[97m%s\n",Entry_Root.fds[(cur_dir->dir_fds)[i]].filename);
                break;
                case 1:
                printf("\e[96m%s\n\e[97m",Entry_Root.fds[(cur_dir->dir_fds)[i]].filename);
                break;
                case 2:
                printf("\e[92m%s\e[97m\t -> \e[96m%s\e[97m\n",Entry_Root.fds[(cur_dir->dir_fds)[i]].filename,Entry_Root.fds[Entry_Root.fds[(cur_dir->dir_fds)[i]].ref_to].filename);

                break;
    
            }
            
        }
    }
}






// static int __sfs_open___(char *name,FileDescriptor *dir)
// {
//     int fileID = getIndexOfFileInDirectory(name,dir,&Entry_Root);
//     // 如果找到了文件，将文件打开并返回

//     if (fileID != -1)
//     {
//         Entry_Root.fds[fileID].fas.opened=1;
//         return fileID;
//     }
//     // 没有找到文件，则新增一个文件，初始化该文件的FCB，并将该FCB加入到目录中
//     for(int i=0;i<FILE_LIST_SIZE;i++)
//     {
//         if(Entry_Root.fds[i].type==0xff)
//         {
//             fileID = i;
//             break;
//         }
//     }
    
//     FileDescriptor_createFile(name, &Entry_Root.fds[fileID]);

//     // 给当前文件分配一个新的块
//     int free_node_index = FAT_getFreeNode(&fat);
//     if(free_node_index == -1)
//     {
//         FileDescriptor_destory(&Entry_Root.fds[fileID]);
//         return -1;
//     }
//     Entry_Root.fds[fileID].fat_index =free_node_index;
//     fat.table[free_node_index] = free_node_index; // 文件最后一个块的 内容等于它本身的编号

//     ds_write_blocks(1,1, (void *)&fat);
//     ds_write_blocks(2,2, (void *)&Entry_Root);
//     return fileID;
// }








// int sfs_write(int fileID, char *buf, int length)
// {
//     if (Entry_Root.fds[fileID].fas.opened == 0 || Entry_Root.fds[fileID].type!=0)
//         return -1;
    
//     int16_t node_index = Entry_Root.fds[fileID].fat_index;

//     // 重新写入文件前，回收该文件所占用的全部空间
//     while (node_index != fat.table[node_index])
//     {
//         fat.table[node_index] = -1;
//         fat.count--;
//         node_index = fat.table[node_index];
//     }
//     fat.table[node_index] = -1;
//     fat.count--;
//     //  写入文件长度
//     Entry_Root.fds[fileID].size = length;
//     // 写入文件
//     int block_num = length / NB_BLOCK;
//     int res = length % NB_BLOCK;
//     // 如果文件的长度不是 块长的整数倍，应该多分配一个块
//     if (res > 0)
//         block_num += 1;
    
//     // 声明一个块
//     void *blockWrite = (void *)malloc(NB_BLOCK);
    
//     if (blockWrite == 0) return -1;
//     // 全部置为0
//     int16_t last_node_index = -1;
//     memset(blockWrite, 0, NB_BLOCK);
    
//     while (length > 0)
//     {
//         if (length > NB_BLOCK)
//         {
//             memcpy(blockWrite, (void *)buf, NB_BLOCK);

//             int16_t free_node_index = FAT_getFreeNode(&fat);
//             ds_write_blocks(free_node_index, 1, blockWrite); // 写入一个块
//             if (last_node_index != -1)
//             {
//                 fat.table[last_node_index] = free_node_index;
//             }
//             else
//             {
//                 last_node_index = free_node_index;
//                 Entry_Root.fds[fileID].fat_index = free_node_index;
//             }
//             length -= NB_BLOCK; 
//             buf += NB_BLOCK; // 指针后移
//         }
//         else
//         {
//             memcpy(blockWrite, (void *)buf, length);
//             int16_t free_node_index = FAT_getFreeNode(&fat);
//             ds_write_blocks(free_node_index, 1, blockWrite); // 写入一个块
//             if (last_node_index != -1)
//             {
//                 fat.table[last_node_index] = free_node_index;
//             }
//             else
//             {
//                 last_node_index = free_node_index;
//                 Entry_Root.fds[fileID].fat_index = free_node_index;
//             }
//             buf += length;
//             length -= length; 
//         }
//     }
//     fat.table[last_node_index] = last_node_index; //标识文件结束
//     free(blockWrite);
//     return 0;
// }

// int sfs_read(int fileID, char *buf, int length)
// {
//     if (Entry_Root.fds[fileID].fas.opened == 0)
//         return -1;
    
//     int8_t read_flag = 1;

//     void *blockRead = (void *)malloc(NB_BLOCK);
//     // 全部置为0
//     int16_t read_node_index = root.table[fileID].fat_index;

//     memset(blockRead, 0, NB_BLOCK);

//     while (length > 0 )
//     {
//         if (length > NB_BLOCK)
//         {
//             ds_read_blocks(read_node_index, 1, blockRead);
//             memcpy(buf, blockRead, NB_BLOCK);
//             length -= NB_BLOCK;
//             buf += NB_BLOCK; // 指针后移
//         }
//         else
//         {
//             ds_read_blocks(read_node_index, 1, blockRead);
//             memcpy(buf, blockRead, length);
//             buf += length; // 指针后移
//             length -= length;
//         }
//         int16_t next_read_node_index = fat.table[read_node_index];
//         if (next_read_node_index == read_node_index)
//             break;
//         else
//             read_node_index = next_read_node_index;
//     }
//     free(blockRead);
//     return 0;
// }
// int sfs_rm(int fd)
// {
//     if(root.table[fd].filename[0]==0)
//     return -1;
//     if(root)
//     return 0;
// }

