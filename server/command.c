#include <func.h>
#include "server.h"

// 进入某文件夹
int myCd(const char *path) 
{
    int ret = chdir(path);
    ERROR_CHECK(ret, -1, "chdir");
    return 0;
}

//自定义接收完整文件数据
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

// 将input拆解为cmd、path1、path2
int cutCmd(char *input, char *cmd, char *path1, char *path2)
{
    char tmp[1024] = {0};
    char *p1, *p2, *p3;
    memcpy(tmp, input, strlen(input) - 1);
    tmp[strlen(input)] = '\0';
    p1 = strtok(tmp, " ");
    memcpy(cmd, p1, strlen(p1) + 1);
    p2 = strtok(NULL, " ");
    if (p2 == NULL)
    {
        return 1;
    }
    memcpy(path1, p2, strlen(p2) + 1);
    p3 = strtok(NULL, " ");
    if (p3 == NULL)
    {
        return 1;
    }
    memcpy(path2, p3, strlen(p3) + 1);
    if (strtok(NULL, " "))
    {
        return 0;
    }
    return 1;
}

// 将文件名字fileName根据路径path拆解出来
int cutPath(char *path, char *fileName)
{
    // strtok会改变原来的字符串，所以要复制path到pathTmp
    char pathTmp[1024] = {0};
    strcpy(pathTmp, path);
    char *token = strtok(pathTmp, "/");
    while (token)
    {
        strcpy(fileName, token);
        token = strtok(NULL, "/");
    }
    return 0;
}
