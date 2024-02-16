#include <func.h>
#include "client.h"

// 自定义一定能接收完整文件
int recvn(int sockFd, void *pStart, int len)
{
    int total = 0;
    char *p = (char *)pStart;
    while (total < len)
    {
        int ret = recv(sockFd, p + total, len - total, 0);
        total += ret;
    }
    return 0;
}

// 接收来自键盘的数据
ssize_t recvStdin(char *buf, int bufLength)
{
    memset(buf, 0, bufLength);
    ssize_t rdRet = read(STDIN_FILENO, buf, bufLength);
    ERROR_CHECK(rdRet, -1, "read");
    return rdRet;
}

// 发送信息到server
int sendMessageToServer(int fd, char *buf, int length)
{
    train_t train;
    memset(&train, 0, sizeof(train));
    train.length = length;
    memcpy(train.data, buf, length);
    ssize_t sendRet = send(fd, &train, sizeof(train.length) + length, MSG_NOSIGNAL);
    ERROR_CHECK(sendRet, -1, "send");
    return 0;
}

// 接收server发来的token
int recvTokenFromServer(int fd, char *username, char *token)
{
    train_t train;
    memset(&train, 0, sizeof(train));
    /*username*/
    recvn(fd, &train.length, sizeof(train.length));
    recvn(fd, train.data, train.length);
    memcpy(username, train.data, train.length);
    /*token*/
    recvn(fd, &train.length, sizeof(train.length));
    recvn(fd, train.data, train.length);
    memcpy(token, train.data, train.length);
    return 0;
}

// 接收server的信息
ssize_t recvMessageFromServer(int fd, char *buf, int bufLength)
{
    train_t train;
    memset(buf, 0, bufLength);
    memset(&train, 0, sizeof(train));
    ssize_t recvRet = recv(fd, &train.length, sizeof(train.length), MSG_WAITALL);
    ERROR_CHECK(recvRet, -1, "recv");
    recvRet = recv(fd, train.data, train.length, MSG_WAITALL);
    ERROR_CHECK(recvRet, -1, "recv");
    memcpy(buf, train.data, train.length);
    return recvRet;
}

// 接收server发来的当前工作路径
ssize_t recvCurPath(int fd, char *curPath, int recvLength)
{
    recvMessageFromServer(fd, curPath, recvLength);
}

// 给server发送token
int sendTokenToServer(int fd, char *username, char *token)
{
    train_t train;
    memset(&train, 0, sizeof(train));
    /*username*/
    train.length = strlen(username);
    memcpy(train.data, username, train.length);
    ssize_t sendRet = send(fd, &train, sizeof(train.length) + train.length, MSG_NOSIGNAL);
    ERROR_CHECK(sendRet, -1, "send username");
    /*token*/
    train.length = strlen(token);
    memcpy(train.data, token, train.length);
    sendRet = send(fd, &train, sizeof(train.length) + train.length, MSG_NOSIGNAL);
    ERROR_CHECK(sendRet, -1, "send token");
    return 0;
}

// 接收server发来的文件
int recvFileFromServer(int fd)
{

    /*文件名*/
    train_t train;
    recvn(fd, &train.length, sizeof(train.length));
    recvn(fd, train.data, train.length);
    char fileName[4096] = {0};
    strcpy(fileName, train.data);

    int filefd = open(fileName, O_RDWR | O_CREAT | O_APPEND, 0666);
    off_t offset = lseek(filefd, 0, SEEK_END);
    send(fd, &offset, sizeof(offset), 0);

    off_t filesize;
    recvn(fd, &train.length, sizeof(train.length));
    recvn(fd, train.data, train.length);
    memcpy(&filesize, train.data, sizeof(filesize));

    /*文件*/
    ftruncate(filefd, filesize);
    char *p = (char *)mmap(NULL, filesize, PROT_READ | PROT_WRITE, MAP_SHARED, filefd, 0);
    recvn(fd, p + offset, filesize - offset);
    munmap(p, filesize);

    close(filefd);
    return 0;
}

// 给server发送文件
int sendFileToServer(int fd, char *localPath, char *virtualPath)
{
    /*文件名*/
    train_t train;
    train.length = strlen(localPath);
    memcpy(train.data, localPath, train.length);
    send(fd, &train, sizeof(train.length) + train.length, MSG_NOSIGNAL);
    /*文件哈希值*/
    char md5[128];
    getFileMd5(localPath, md5);
    train.length = strlen(md5);
    memcpy(train.data, md5, train.length);
    send(fd, &train, sizeof(train.length) + train.length, MSG_NOSIGNAL);
    /*文件长度*/
    int filefd = open(localPath, O_RDONLY);
    ERROR_CHECK(filefd, -1, "filefd");
    struct stat statbuf;
    fstat(filefd, &statbuf);
    printf("%d\n", (int)statbuf.st_size);
    train.length = sizeof(statbuf.st_size);
    memcpy(train.data, &statbuf.st_size, train.length);
    send(fd, &train, sizeof(train.length) + train.length, MSG_NOSIGNAL);
    /*文件*/
    sendfile(fd, filefd, NULL, statbuf.st_size);

    close(filefd);

    return 0;
}