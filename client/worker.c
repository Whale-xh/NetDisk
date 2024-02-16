#include "client.h"

extern char *ip;
extern char *port;

// 初始化资源
int sourceInit(source_t *pSource, int fd, char *msgFromSTDIN, char *user, char *cmd, char *srcPath, char *dstPath, char *token, char *curPath)
{
    pSource->fd = fd;
    bzero(pSource->longCmd, sizeof(pSource->longCmd));
    strcpy(pSource->longCmd, msgFromSTDIN);
    bzero(pSource->user, sizeof(pSource->user));
    strcpy(pSource->user, user);
    bzero(pSource->cmd, sizeof(pSource->cmd));
    strcpy(pSource->cmd, cmd);
    bzero(pSource->srcPath, sizeof(pSource->srcPath));
    strcpy(pSource->srcPath, srcPath);
    bzero(pSource->dstPath, sizeof(pSource->dstPath));
    strcpy(pSource->dstPath, dstPath);
    bzero(pSource->token, sizeof(pSource->token));
    strcpy(pSource->token, token);
    bzero(pSource->curPath, sizeof(pSource->curPath));
    strcpy(pSource->curPath, curPath);
    pthread_mutex_init(&pSource->mutex, NULL);
    pthread_cond_init(&pSource->cond, NULL);
    return 0;
}

// 子线程处理函数
void *threadFunc(void *arg)
{
    source_t *pSource = (source_t *)arg;

    // 子线程与server服务端创建新连接
    int sockFd = socket(AF_INET, SOCK_STREAM, 0);
    tcpConnect(sockFd, ip, port);

    // puts命令
    if (strcmp(pSource->cmd, "puts") == 0)
    {
        // int sockFd= socket(AF_INET, SOCK_STREAM, 0);
        // tcpConnect(sockFd,ip,port);

        sendMessageToServer(sockFd, "puts", 4);
        char sendFdToServer[8] = {0};
        sprintf(sendFdToServer, "%d", pSource->fd);
        sendMessageToServer(sockFd, sendFdToServer, strlen(sendFdToServer));

        // 给server发送未经拆解的长命令
        sendMessageToServer(sockFd, pSource->longCmd, sizeof(pSource->longCmd));

        // 给server发送token进行验证
        sendTokenToServer(sockFd, pSource->user, pSource->token);

        // 发送md5给server
        char md5[128] = {0};

        myCd("./upload");
        getFileMd5(pSource->srcPath, md5);
        sendMessageToServer(sockFd, md5, strlen(md5));
        myCd("..");

        char msgFromServer[128] = {0};
        ssize_t recvRet = recvMessageFromServer(sockFd, msgFromServer, sizeof(msgFromServer));
        if (strcmp(msgFromServer, "second pass") == 0)
        {
            printf("second pass: puts [%s] is ok...\n", pSource->srcPath);
        }
        else if (strcmp(msgFromServer, "send file") == 0)
        {
            myCd("./upload");
            sendFileToServer(sockFd, pSource->srcPath, pSource->dstPath);
            myCd("../");
        }
        else if (strcmp(msgFromServer, "No file") == 0)
        {
            printf("puts: [%s]: No such file or diectory\n", pSource->dstPath);
        }
        close(sockFd);
    }
    // gets命令
    else
    {
        // int sockFd= socket(AF_INET, SOCK_STREAM, 0);
        // tcpConnect(sockFd,ip,port);

        sendMessageToServer(sockFd, "gets", 4);

        char sendFdToServer[8] = {0};
        sprintf(sendFdToServer, "%d", pSource->fd);
        sendMessageToServer(sockFd, sendFdToServer, strlen(sendFdToServer));

        // 给server发送未经拆解的长命令
        sendMessageToServer(sockFd, pSource->longCmd, sizeof(pSource->longCmd));

        // 给server发送token进行验证
        sendTokenToServer(sockFd, pSource->user, pSource->token);

        char msgFromServer[128] = {0};
        recvMessageFromServer(sockFd, msgFromServer, sizeof(msgFromServer));
        if (strcmp(msgFromServer, "Token ok") == 0)
        {
            myCd("./download");
            recvFileFromServer(sockFd);
            myCd("../");
        }
        else
        {
            puts("Token Wrong,download fail");
        }
        close(sockFd);
    }
    print(pSource->user, pSource->curPath);
}