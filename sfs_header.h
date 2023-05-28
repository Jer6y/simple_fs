
#ifndef min
	#define min( a, b ) ( ((a) < (b)) ? (a) : (b) )
#endif

#ifndef SFS_API_HEADER
#define SFS_API_HEADER

#include <time.h>
#include <stdint.h>

#define NB_BLOCK 4096   // 每个块有4096个 byte     
#define MAX_DISK 2047   // 磁盘总共有多少个块

typedef struct FileAccessStatus {       // 文件当前状态
    int opened;                         // 该文件是否已经打开
} FileAccessStatus;

typedef struct FileDescriptor {         // FCB
    uint8_t             type;           // 自增:文件类型 dir or file 或者其他
    // type: 0-> file  1->dir 2->l  0xff -> 未分配
    uint16_t             dir_fds[12];    
    // 如果type 为 dir 那么dir_fds 就是 这个目录包含的其他文件或者目录的描述符地址
    // 这个地址应该是相对偏移量也就是fileID
    uint8_t             ref; 
    // 还有多少个文件描述符是指向自己的
    uint16_t            ref_to;
    // 如果是硬连接 那么指向的FileID是哪个
    char                filename[16];   // 文件名
    int16_t             fat_index;      // 在fat 上的index
    time_t              timestamp;      // 创建时间
    int                 size;           // 文件大小
	FileAccessStatus    fas;            // 文件状态
    uint8_t             padding[2];
    // 保留字符
}__attribute__((packed)) FileDescriptor;

#define FILE_LIST_SIZE (NB_BLOCK*2 / sizeof(FileDescriptor))
// 目录区 存放文件描述符 占用2 块 
// 我的文件系统 一共可以存放128个文件
 
// // 目录结构体 整个文件系统中只有一个
// typedef struct DirectoryDescriptor {    // 描述目录信息

//     // 文件列表，最多保存 NB_BLOCK / sizeof(FileDescriptor) 个文件
//     FileDescriptor table[FILE_LIST_SIZE];     
//     int count;                          // 文件个数
// } DirectoryDescriptor;


// 目录区 存放文件描述符 占用2 块 
// 我的文件系统 一共可以存放128个文件
typedef struct FD_Room
{
    FileDescriptor fds[FILE_LIST_SIZE];
}__attribute__((packed)) FD_Room;


// 文件分配表，整个文件系统只有一个
typedef struct FileAllocationTable {
    int16_t table[MAX_DISK];           // 初始的时候为全-1，代表没有被分配
    int16_t count;                          // 已经分配的块的个数
} FileAllocationTable;

//元数据区域 占用磁盘的0块
typedef struct super_block
{
    /* 0 */  char name[16] ; //厂商名
    /* 16 */ uint16_t BytsPerSec;  //每个Sector的字节 只能是512 1024 2048 4096
    /* 18 */ uint16_t SecPerClus;  //每个簇的Sector   只能是1 2 4 8 
    /* 20 */ uint32_t ClusTot;     //总共磁盘的大小 单位是Cluster
    /* 24 */ uint32_t EntryFat;    //FAT区起始的Cluster号 FAT区默认只有一个占用一个Cluster
    /* 28 */ uint32_t EntryFd;     //目录区起始的Cluster号 目录去默认有两个 每个目录项的结构体在上面 一共有128个可容纳的文件
    /* 32 */ uint32_t EntryData;   //data区起始Cluster号 大小可以用ClusTot - 1 - 1 -2
    /* 36 */ uint16_t MagicNumber; //魔数
    /* */    uint8_t padding[4056];

}__attribute__((packed)) super_block;
#endif
