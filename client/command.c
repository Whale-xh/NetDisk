#include <func.h>
#include "client.h"

// 进入某文件夹
int myCd(const char *path) 
{
    int ret = chdir(path);
    ERROR_CHECK(ret, -1, "chdir");
    return 0;
}

// 打印命令行终端
void print(char *userName, char *curPath)
{

    printf("[\033[32m%s@Netdisk :\033[36m~%s\033[0m]$ ", userName, curPath);
    fflush(stdout);
}

// 命令拆解
int cutCmd(char *input, char *cmd, char *srcPath, char *dstPath)
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
    memcpy(srcPath, p2, strlen(p2) + 1);
    p3 = strtok(NULL, " ");
    if (p3 == NULL)
    {
        return 1;
    }
    memcpy(dstPath, p3, strlen(p3) + 1);
    if (strtok(NULL, " "))
    {
        return 0;
    }
    return 1;
}

//命令检查
int checkCmd(const char *cmd)
{
    if (strcmp(cmd, "cd") == 0 || strcmp(cmd, "ls") == 0 || strcmp(cmd, "gets") == 0 || strcmp(cmd, "puts") == 0 || strcmp(cmd, "rm") == 0 || strcmp(cmd, "rmdir") == 0 || strcmp(cmd, "mkdir") == 0 || strcmp(cmd, "pwd") == 0)
    {
        return 1;
    }
    return 0;
}

//命令参数检查
int checkArguments(const char *cmd, const char *srcPath, const char *dstPath, int cutRet)
{
    if (!checkCmd(cmd))
    {
        printf("\n%s: command not found\n", cmd);
        return 0;
    }
    if (!cutRet)
    {
        printf("%s: too many arguments\n", cmd);
        return 0;
    }
    if ((strcmp(cmd, "pwd") == 0 || strcmp(cmd, "ls") == 0) && strlen(srcPath) != 0)
    {
        printf("%s: too many arguments\n", cmd);
        return 0;
    }
    else if ((strcmp(cmd, "mkdir") == 0 || strcmp(cmd, "rmdir") == 0 || strcmp(cmd, "remove") == 0 || strcmp(cmd, "cd") == 0) && strlen(dstPath) != 0)
    {
        printf("%s: too many arguments\n", cmd);
        return 0;
    }
    else if ((strcmp(cmd, "mkdir") == 0 || strcmp(cmd, "rmdir") == 0 || strcmp(cmd, "remove") == 0 || strcmp(cmd, "gets") == 0 || strcmp(cmd, "puts") == 0) && strlen(srcPath) == 0)
    {
        printf("%s: need an argument\n", cmd);
        return 0;
    }
    //下载与上传文件时，添加绝对路径功能
    // else if ((strcmp(cmd, "puts") == 0 || strcmp(cmd, "gets") == 0) && (strlen(srcPath) == 0 || strlen(dstPath) == 0)) {
    //     printf("%s: need two arguments\n", cmd);
    //     return 0;
    // }
    if (strcmp(cmd, "puts") == 0)
    {
        myCd("./upload");
        int ret = open(srcPath, O_RDWR);
        if (ret == -1)
        {
            printf("puts: no file [%s]\n",srcPath);
            close(ret);
            myCd("../");
            return 0;
        }
        myCd("../");
    }
    return 1;
}
