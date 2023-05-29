#include "sfs_api.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "sfs_header.h"
#define MAX_CMDBUF_SIZE 1024
#define MAX_WRITE_SIZE 4096
#define MAX_FILENAME_SIZE 32
#define MAX_FZ  1024
extern FD_Room Entry_Root;
char cmd_buf[MAX_CMDBUF_SIZE+1] ={0};
char write_buf[MAX_WRITE_SIZE+1] ={0};
int bsize =0;
char t_buf[MAX_FZ+1] ={0};
int t_last = MAX_FZ;
//分析命令行缓冲区
//具体描述:
// 参数:
// 如果指令合法,则操作文件名被解析存放于 file, size是他的大小,max_size是最长的文件名
// 返回值:
// -1  -->  非法指令
//  1  -->  ls
//  2  -->  cd  
//  3  -->  cat  
//  4  -->  touch 
//  5  -->  rm
//  6  -->  echo 
//  7  -->  quit
//  8  -->  mkdir
//  9  -->  rmdir
// 10  -->  mklink
// 11  -->  rmlink
int analyze_cmd(char *file,size_t* size,int max_size)
{
    
    char instruction[MAX_CMDBUF_SIZE+1] ={0};
    int i=0,left=0,right=0;
    while(i<MAX_CMDBUF_SIZE && cmd_buf[i]!=' ' && cmd_buf[i]!= '\0') i++;
    strncpy(instruction,cmd_buf,i);
    // printf("%s\n",instruction);
    // printf("%d\n",i);
    if(strcmp("ls",instruction)==0)
    {
        return 1;
    }
    else if(strcmp("cd",instruction)==0)
    {
        if(i==MAX_CMDBUF_SIZE || cmd_buf[i]==0) return -1;
        while(i<MAX_CMDBUF_SIZE && cmd_buf[i]==' ') i++;
        // printf("%d\n",i);
        if(i==MAX_CMDBUF_SIZE || cmd_buf[i]==0) return -1;
        left = i++;
        // printf("%d\n",left);
        while(i<MAX_CMDBUF_SIZE && cmd_buf[i]!=' ' && cmd_buf[i]!= 0) i++;
        right = i;
        // printf("test\n");
        // printf("%d\n",right - left);
        if(right - left > max_size) return -1;
        *size = right - left;
        strncpy(file,cmd_buf+left,right - left);
        return 2;
    }
    else if(strcmp("cat",instruction)==0)
    {
        if(i==MAX_CMDBUF_SIZE || cmd_buf[i]==0) return -1;
        while(i<MAX_CMDBUF_SIZE && cmd_buf[i]==' ') i++;
        // printf("%d\n",i);
        if(i==MAX_CMDBUF_SIZE || cmd_buf[i]==0) return -1;
        left = i++;
        // printf("%d\n",left);
        while(i<MAX_CMDBUF_SIZE && cmd_buf[i]!=' ' && cmd_buf[i]!= 0) i++;
        right = i;
        // printf("test\n");
        // printf("%d\n",right - left);
        if(right - left > max_size) return -1;
        *size = right - left;
        strncpy(file,cmd_buf+left,right - left);
        return 3;
    }
    else if(strcmp("touch",instruction)==0)
    {
       if(i==MAX_CMDBUF_SIZE || cmd_buf[i]==0) return -1;
        while(i<MAX_CMDBUF_SIZE && cmd_buf[i]==' ') i++;
        // printf("%d\n",i);
        if(i==MAX_CMDBUF_SIZE || cmd_buf[i]==0) return -1;
        left = i++;
        // printf("%d\n",left);
        while(i<MAX_CMDBUF_SIZE && cmd_buf[i]!=' ' && cmd_buf[i]!= 0) i++;
        right = i;
        // printf("test\n");
        // printf("%d\n",right - left);
        if(right - left > max_size) return -1;
        *size = right - left;
        strncpy(file,cmd_buf+left,right - left);
        return 4;
    }
    else if(strcmp("rm",instruction)==0)
    {
        if(i==MAX_CMDBUF_SIZE || cmd_buf[i]==0) return -1;
        while(i<MAX_CMDBUF_SIZE && cmd_buf[i]==' ') i++;
        // printf("%d\n",i);
        if(i==MAX_CMDBUF_SIZE || cmd_buf[i]==0) return -1;
        left = i++;
        // printf("%d\n",left);
        while(i<MAX_CMDBUF_SIZE && cmd_buf[i]!=' ' && cmd_buf[i]!= 0) i++;
        right = i;
        // printf("test\n");
        // printf("%d\n",right - left);
        if(right - left > max_size) return -1;
        *size = right - left;
        strncpy(file,cmd_buf+left,right - left);
        return 5;
    }
    else if(strcmp("echo",instruction)==0)
    {
        if(i==MAX_CMDBUF_SIZE || cmd_buf[i]==0) return -1;
        while(i<MAX_CMDBUF_SIZE && cmd_buf[i]==' ') i++;
        // printf("%d\n",i);
        if(i==MAX_CMDBUF_SIZE || cmd_buf[i]==0) return -1;
        left = i++;
        // printf("%d\n",left);
        while(i<MAX_CMDBUF_SIZE && cmd_buf[i]!=' ' && cmd_buf[i]!= 0) i++;
        right = i;
        // printf("test\n");
        // printf("%d\n",right - left);
        if(right - left > max_size) return -1;
        *size = right - left;
        strncpy(file,cmd_buf+left,right - left);
        if(i==MAX_CMDBUF_SIZE || cmd_buf[i]==0) return -1;
        while(i<MAX_CMDBUF_SIZE && cmd_buf[i]==' ') i++;
        if(i==MAX_CMDBUF_SIZE || cmd_buf[i]==0) return -1;
        left = i++;
        while(i<MAX_CMDBUF_SIZE && cmd_buf[i]!= 0) i++;
        right = i;
        if(right - left > MAX_WRITE_SIZE) return -1;
        strncpy(write_buf,cmd_buf+left,right-left);
        write_buf[right-left]=0;
        bsize = right -left;

        return 6;
    }
    else if(strcmp("quit",instruction)==0)
    {
        return 7;
    }
    else if(strcmp("mkdir",instruction) ==0)
    {
        if(i==MAX_CMDBUF_SIZE || cmd_buf[i]==0) return -1;
        while(i<MAX_CMDBUF_SIZE && cmd_buf[i]==' ') i++;
        // printf("%d\n",i);
        if(i==MAX_CMDBUF_SIZE || cmd_buf[i]==0) return -1;
        left = i++;
        // printf("%d\n",left);
        while(i<MAX_CMDBUF_SIZE && cmd_buf[i]!=' ' && cmd_buf[i]!= 0) i++;
        right = i;
        // printf("test\n");
        // printf("%d\n",right - left);
        if(right - left > max_size) return -1;
        *size = right - left;
        strncpy(file,cmd_buf+left,right - left);
        return 8;
    }
    else if(strcmp("rmdir",instruction) ==0)
    {
        if(i==MAX_CMDBUF_SIZE || cmd_buf[i]==0) return -1;
        while(i<MAX_CMDBUF_SIZE && cmd_buf[i]==' ') i++;
        // printf("%d\n",i);
        if(i==MAX_CMDBUF_SIZE || cmd_buf[i]==0) return -1;
        left = i++;
        // printf("%d\n",left);
        while(i<MAX_CMDBUF_SIZE && cmd_buf[i]!=' ' && cmd_buf[i]!= 0) i++;
        right = i;
        // printf("test\n");
        // printf("%d\n",right - left);
        if(right - left > max_size) return -1;
        *size = right - left;
        strncpy(file,cmd_buf+left,right - left);
        return 9;
    }
    else if(strcmp("mklink",instruction) ==0)
    {
        if(i==MAX_CMDBUF_SIZE || cmd_buf[i]==0) return -1;
        while(i<MAX_CMDBUF_SIZE && cmd_buf[i]==' ') i++;
        // printf("%d\n",i);
        if(i==MAX_CMDBUF_SIZE || cmd_buf[i]==0) return -1;
        left = i++;
        // printf("%d\n",left);
        while(i<MAX_CMDBUF_SIZE && cmd_buf[i]!=' ' && cmd_buf[i]!= 0) i++;
        right = i;
        // printf("test\n");
        // printf("%d\n",right - left);
        if(right - left > max_size) return -1;
        *size = right - left;
        strncpy(file,cmd_buf+left,right - left);
        if(i==MAX_CMDBUF_SIZE || cmd_buf[i]==0) return -1;
        while(i<MAX_CMDBUF_SIZE && cmd_buf[i]==' ') i++;
        if(i==MAX_CMDBUF_SIZE || cmd_buf[i]==0) return -1;
        left = i++;
        while(i<MAX_CMDBUF_SIZE && cmd_buf[i]!= 0) i++;
        right = i;
        if(right - left > MAX_WRITE_SIZE) return -1;
        strncpy(write_buf,cmd_buf+left,right-left);
        write_buf[right-left]=0;
        bsize = right -left;
        return 10;
    }
    else if(strcmp("rmlink",instruction) ==0)
    {
        if(i==MAX_CMDBUF_SIZE || cmd_buf[i]==0) return -1;
        while(i<MAX_CMDBUF_SIZE && cmd_buf[i]==' ') i++;
        // printf("%d\n",i);
        if(i==MAX_CMDBUF_SIZE || cmd_buf[i]==0) return -1;
        left = i++;
        // printf("%d\n",left);
        while(i<MAX_CMDBUF_SIZE && cmd_buf[i]!=' ' && cmd_buf[i]!= 0) i++;
        right = i;
        // printf("test\n");
        // printf("%d\n",right - left);
        if(right - left > max_size) return -1;
        *size = right - left;
        strncpy(file,cmd_buf+left,right - left);
        return 11;
    }
    return -1;
}

int loga()
{
    char c=0;
    char user_name[10] ={0};
    char passwd[20] ={0};
    return 1;
}
int main(void)
{
    sfs_system_init();
    sfs_system_info();
    while(1)
    {
        if(!loga()) 
        {
            printf("输入密码错误!\n");
            continue;
        }
        size_t size =0;
        char file_name[MAX_FILENAME_SIZE+1]={0};
        char buf[MAX_CMDBUF_SIZE+1] ={0};
        int buf_ptr=0;
        char c =0;
        while(buf_ptr!= MAX_CMDBUF_SIZE && (c = getchar())!='\n')
        {
            buf[buf_ptr++] =c;
        }
        if(buf_ptr ==MAX_CMDBUF_SIZE)
        {
            while((c = getchar())!='\n') ;
        }
        strncpy(cmd_buf,buf,buf_ptr);
        cmd_buf[buf_ptr] =0;
        // printf("%s \n",cmd_buf);
        int choice = analyze_cmd(file_name,&size,MAX_FILENAME_SIZE);
        switch(choice)
        {
            case -1:
            printf("error instruction format!\n");
            break;
            case 1:
            sfs_ls();
            break;
            case 2:
            sfs_cd(file_name);
            // return 0;
            break;
            case 3:
            {
                char tmp[2049] ={0};
                // printf("cat\n");
                int fd = sfs_open(file_name);
                if(fd ==-1) 
                {
                    printf("不存在该文件\n");
                }
                else
                { 
                    if(sfs_read(fd,tmp,2048)==-1)
                    {
                        printf("这是一个目录!\n");
                        sfs_close(fd);
                    }
                    else
                    {
                        for(int i=0;tmp[i]!=0;i++)
                        {
                            printf("%c",tmp[i]);
                        }
                        printf("\n");
                        sfs_close(fd);
                    }
                    
                } 
            }
            break;
            case 4:
            {
                int fd = sfs_create(file_name,0);
                //返回-1 路径中有目录未创建
                //返回-2 目标已存在，且类型相同
                //返回-3 创建失败 空间不够
                //返回-4 目标存在，且类型不同
                //返回-5 路径中存在目标为非目录类型
                // >=0 创建的文件描述符
                //保证调用时name长度不超过100
                //目录级数不超过10
                //同时保证最后一个文件名字长度不应该超过15个字符
                if(fd ==-1) printf("路径中有目录未创建!\n");
                else if(fd ==-2) printf("目标已存在!\n");
                else if(fd ==-3) printf("空间不够!\n");
                else if(fd ==-4) printf("目标存在，且类型不同!\n");
                else if(fd ==-5) printf("路径中存在目标为非目录类型!\n");
                else 
                {
                    printf("创建成功!\n");
                }
            }
            break;
            case 5:
            {
                //返回0  删除
                //返回-1 目标不存在
                //返回-2 目录不为空
                //保证name不超过100
                //保证目录级数不超过10级
                int fd = sfs_remove(file_name,0);
                if(fd ==-1) printf("目标不存在!\n");
                else
                {
                    printf("删除成功!\n");
                }
            }
            break;
            case 6:
            {
                int fd = sfs_open(file_name);
                if(fd<0)
                {
                    printf("没有该文件!\n");
                    break;
                }
                if(sfs_write(fd,write_buf,bsize)==-1)
                {
                    printf("你写入了一个目录!\n");
                }
                else
                {
                    printf("写入成功!\n");
                }
                sfs_close(fd);
            }
            break;
            case 7:
            goto ed;
            case 8:
            {
                int fd = sfs_create(file_name,1);
                if(fd ==-1) printf("路径中有目录未创建!\n");
                else if(fd ==-2) printf("目标已存在!\n");
                else if(fd ==-3) printf("空间不够!\n");
                else if(fd ==-4) printf("目标存在，且类型不同!\n");
                else if(fd ==-5) printf("路径中存在目标为非目录类型!\n");
                else
                {
                    printf("创建成功!\n");
                }
            }
            break;
            case 9:
            {
                //返回0  删除
                //返回-1 目标不存在
                //返回-2 目录不为空
                //保证name不超过100
                //保证目录级数不超过10级
                int fd = sfs_remove(file_name,1);
                if(fd ==-1) printf("目录不存在!\n");
                else if(fd ==-2) printf("目录不为空!\n");
                else
                {
                    printf("删除成功!\n");
                }
            }
            break;
            case 10:
            {
                //  0 成功
                // -4 链接目标不存在
                // -1 链接源不存在
                // -2 原链接已经存在
                // -3 创建的空间不够
                // -5 链接源的过程中存在非目录的文件类型
                int a =sfs_mklink(file_name,write_buf);
                if(a==-4)
                {
                    printf("链接目标不存在!\n");

                }
                else if(a==-2)
                {
                    printf("原链接已经存在!\n");
                }
                else if(a==-3)
                {
                    printf("创建的空间不够!\n");
                }
                else if(a==-1)
                {
                    printf("链接源不存在!\n");
                }
                else if(a==-5)
                {
                    printf("链接源的过程中存在非目录的文件类型\n");
                }
                else if(a==0)
                {
                    printf("创建成功!\n");
                }
            }
            break;
            case 11:
            {
                //返回0  删除
                //返回-1 目标不存在
                //返回-2 目录不为空
                //保证name不超过100
                //保证目录级数不超过10级
                if(sfs_remove(file_name,2)==-1)
                {
                    printf("链接不存在!\n");
                }
                else
                {
                    printf("删除成功!\n");
                }
            }
        }

    }

    ed:
    sfs_system_close();
    return 0;
}
