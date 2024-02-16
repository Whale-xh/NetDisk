#include "server.h"
#include "jwt.h"

// 数据库信息
extern char *DBHost;
extern char *DBUser;
extern char *DBPasswd;
extern char *DBName;

void *handleEvent(void *arg)
{
    threadPool_t *pThreadPool = (threadPool_t *)arg;

    // 子线程与MySQL连接
    pthread_mutex_lock(&pThreadPool->mutex);
    MYSQL *mysql = mysqlConnect(DBHost, DBUser, DBPasswd, DBName);
    pthread_mutex_unlock(&pThreadPool->mutex);
    printf("Thread [%lu] connect mysql sucessful...\n",pthread_self());

    while (1)
    {
        printf("Worker [%lu] is waiting task...\n", pthread_self());
        pthread_mutex_lock(&pThreadPool->mutex);
        while (!pThreadPool->taskQueue.size)
        {
            pthread_cond_wait(&pThreadPool->cond, &pThreadPool->mutex);
        }
        printf("Worker [%lu] get a task and will go to work!\n", pthread_self());
        int netFd = pThreadPool->taskQueue.pFront->netFd;
        taskDequeue(&pThreadPool->taskQueue);
        pthread_mutex_unlock(&pThreadPool->mutex);

        // 接收客户端未进行拆解的命令
        char msgFromClient[128] = {0};
        int recvRet = recvMessageFromClient(netFd, msgFromClient, sizeof(msgFromClient));
        if (!recvRet)
        {
            close(netFd);
            printf("Client [%d] disconnected..\n", netFd);
            continue;
        }
        LOG_WRITE(msgFromClient);

        char cmd[128] = {0};
        char srcPath[128] = {0};
        char dstPath[128] = {0};
        char Name[128] = {0};
        char Token[4096] = {0};
        cutCmd(msgFromClient, cmd, srcPath, dstPath);

        if (strcmp(cmd, "puts") == 0)
        {
            // 接收client的token进行验证
            recvTokenFromClient(netFd, Name, Token);
            int ret = verify(Name, Token);
            if (ret == 0)
            {
                close(netFd);
                continue;
            }

            char curPath[256] = {0};
            strcpy(curPath, pThreadPool->pathBuffer[netFd]);

            // 验证文件md5
            char md5[128] = {0};
            ssize_t recvRet = recvMessageFromClient(netFd, md5, sizeof(md5));
            if (!recvRet)
            {
                close(netFd);
                printf("\nClient [%d] disconnected...\n", netFd);
                continue;
            }
            int pre_id = 0;

            getId(mysql, pThreadPool->userBuffer[netFd], curPath, &pre_id);

            char fileSize[64] = {0};

            if (searchMd5(mysql, md5, fileSize))
            {
                puts("Md5 exist....\n");
                if (strcmp(pThreadPool->pathBuffer[netFd], "/") == 0)
                {
                    strcat(curPath, srcPath);
                    insertFile(mysql, srcPath, curPath, atoi(fileSize), md5, pThreadPool->userBuffer[netFd], pre_id);
                }
                else
                {
                    strcat(curPath, "/");
                    strcat(curPath, srcPath);
                    insertFile(mysql, srcPath, curPath, atoi(fileSize), md5, pThreadPool->userBuffer[netFd], pre_id);
                }
                sendMessageToClient(netFd, "second pass", 11);
            }
            else
            {
                puts("Strats trans file......");
                sendMessageToClient(netFd, "send file", 9);
                myCd("./fileDir");
                char virtualPath[256] = {0};
                strcpy(virtualPath, pThreadPool->pathBuffer[netFd]);
                printf("virtualPath = %s\n", virtualPath);
                recvFileFromClient(netFd, mysql, pThreadPool, pre_id, virtualPath);
                myCd("../");
            }
        }
        else if (strcmp(cmd, "gets") == 0)
        {
            // 接收client的token进行验证
            recvTokenFromClient(netFd, Name, Token);
            int ret = verify(Name, Token);
            if (ret == 0)
            {
                puts("Token wrong");
                sendMessageToClient(netFd, "Token wrong", 11);
                close(netFd);
                continue;
            }
            else
            {
                puts("Token ok");
                printf("srcPath = %s\n", srcPath);
                sendMessageToClient(netFd, "Token ok", 8);
                char md5[128] = {0};
                searchFileMd5(mysql, md5, pThreadPool->pathBuffer[netFd], pThreadPool->userBuffer[netFd], srcPath);
                myCd("./fileDir");
                sendFileToClient(netFd, srcPath, md5);
                myCd("../");
            }
        }
        printf("Command [%s] is ok...\n", cmd);
    }
}

int makeWorker(threadPool_t *pThreadPool)
{
    for (int i = 0; i < pThreadPool->workerNum; ++i)
    {
        pthread_create(&pThreadPool->workerArr[i], NULL, handleEvent, (void *)pThreadPool);
    }
}