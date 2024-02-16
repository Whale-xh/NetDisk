#include <func.h>
#include "server.h"

ssize_t recvMessageFromClient(int fd, char *buf, int bufLength)
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

int sendTokenToClient(int fd, char *username, char *token)
{
    train_t train;
    memset(&train, 0, sizeof(train));
    /*username*/
    train.length = strlen(username);
    memcpy(train.data, username, train.length);
    ssize_t sendRet = send(fd, &train, sizeof(train.length) + train.length, 0);
    ERROR_CHECK(sendRet, -1, "send username");
    /*token*/
    train.length = strlen(token);
    memcpy(train.data, token, train.length);
    sendRet = send(fd, &train, sizeof(train.length) + train.length, 0);
    ERROR_CHECK(sendRet, -1, "send token");
    return 0;
}

int sendMessageToClient(int fd, char *buf, int sendLength)
{
    train_t train;
    memset(&train, 0, sizeof(train));
    train.length = sendLength;
    memcpy(train.data, buf, sendLength);
    ssize_t sendRet = send(fd, &train, sizeof(train.length) + sendLength, 0);
    ERROR_CHECK(sendRet, -1, "send");
    return 0;
}

int sendCurPath(int fd, char *curPath, int sendLength)
{
    sendMessageToClient(fd, curPath, sendLength);
}

int recvTokenFromClient(int fd, char *username, char *token)
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

int sendFileToClient(int fd, char *fileName, char *md5)
{

    /*文件名*/
    train_t train;
    train.length = strlen(fileName);
    strcpy(train.data, fileName);
    send(fd, &train, train.length + sizeof(train.length), 0);

    /*接收偏移量*/
    off_t offset;
    recv(fd, &offset, sizeof(offset), 0);

    /*文件长度*/
    int filefd = open(md5, O_RDONLY);
    ERROR_CHECK(filefd, -1, "filefd");
    struct stat statbuf;
    fstat(filefd, &statbuf);
    train.length = sizeof(statbuf.st_size);
    memcpy(train.data, &statbuf.st_size, train.length);
    send(fd, &train, sizeof(train.length) + train.length, MSG_NOSIGNAL);

    // 断点续传
    /*文件*/
    sendfile(fd, filefd, &offset, statbuf.st_size - offset);

    close(filefd);

    return 0;
}

int recvFileFromClient(int fd, MYSQL *mysql, threadPool_t *pThreadPool, int pre_id, char *virtualPath)
{

    /*文件名*/
    train_t train;
    recvn(fd, &train.length, sizeof(train.length));
    recvn(fd, train.data, train.length);
    char filepath[4096] = {0};
    char filename[4096] = {0};
    char tmp[1024] = {0};
    memcpy(filepath, train.data, train.length);
    cutTail(filepath, tmp, filename);

    /*文件哈希值*/
    recvn(fd, &train.length, sizeof(train.length));
    recvn(fd, train.data, train.length);
    char md5[128];
    memcpy(md5, train.data, train.length);
    printf("md5=%s\n", md5);
    /*文件路径*/
    char serverPath[512];
    sprintf(serverPath, "%s", md5);
    int filefd = open(serverPath, O_RDWR | O_CREAT | O_APPEND, 0666);
    /*文件长度*/
    off_t filesize;
    recvn(fd, &train.length, sizeof(train.length));
    recvn(fd, train.data, train.length);
    memcpy(&filesize, train.data, sizeof(filesize));
    printf("filesize = %ld\n", filesize);
    ftruncate(filefd, filesize);
    /*文件*/
    char *p = (char *)mmap(NULL, filesize, PROT_READ | PROT_WRITE, MAP_SHARED, filefd, 0);
    recvn(fd, p, filesize);
    puts("trans file ok............");

    /*将文件路径存入数据库*/
    printf("before virtualPath = %s\n", virtualPath);
    strcat(virtualPath, "/");
    strcat(virtualPath, filename);
    printf("after virtualPath = %s\n", virtualPath);
    insertFile(mysql, filename, virtualPath, filesize, md5, pThreadPool->userBuffer[fd], pre_id);

    munmap(p, filesize);
    close(filefd);
    return 0;
}