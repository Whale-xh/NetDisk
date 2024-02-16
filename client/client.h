#ifndef __CLIENT_H__
#define __CLIENT_H__

#include <func.h>
#include "md5.h"

//发送文件协议
typedef struct train_s
{
    int length;
    char data[1000];
} train_t;

//用户信息
typedef struct user_s{
    char userName[128];
    char passwd[1024];
    // char curPath[128]; //用户当前工作目录
}user_t;

//客户端用户共享资源
typedef struct source_s {
    int fd;
    char longCmd[128];
    char user[128];
    char cmd[128];
    char srcPath[256];
    char dstPath[256];
    char token[4096];
    char curPath[256];
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} source_t;

//tcpConnect.c
int tcpConnect(int sockFd, char *pIp, char *pPort); // 客户端链接

//epollFunc.c
int epollAdd(int fd, int epFd);
int epollDel(int fd, int epFd);

//transMSG.c
int recvn(int sockFd, void *pStart, int len);
ssize_t recvStdin(char *buf, int bufLength); 
int sendMessageToServer(int fd, char *buf, int length);
int recvTokenFromServer(int fd,char *username,char *token);
ssize_t recvMessageFromServer(int fd, char *buf, int bufLength);
ssize_t recvCurPath(int fd,char *curPath,int recvLength);
int sendTokenToServer(int fd, char *username, char *token);
int myCd(const char *path);
int recvFileFromServer(int fd);
int sendFileToServer(int fd, char *localPath, char *virtualPath);

//command.c
int cutCmd(char *input,char *cmd,char *path1,char *path2);
int checkArguments(const char *cmd, const char *srcPath, const char *dstPath, int cutRet);
int checkCmd(const char *cmd);
void print(char *userName,char *curPath);

//login.c
int enterUser(user_t *user);

//worker.c
int sourceInit(source_t *pSource, int fd, char *msgFromSTDIN, char *user, char *cmd, char *srcPath, char *dstPath,char *token,char *curPath);
void *threadFunc(void *arg);

#endif