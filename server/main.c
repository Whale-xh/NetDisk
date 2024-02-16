#include "server.h"
#include <func.h>

// ip地址信息
char *ip = "192.168.47.128";
char *port = "1234";
char *threadNum = "3";

// 数据库信息
char *DBHost = "localhost";
char *DBUser = "root";
char *DBPasswd = "123";
char *DBName = "myNetdisk";

// int exitPipe[2];
// void sigFunc(int signum)
// {
//     printf("signum = %d\n", signum);
//     write(exitPipe[1], "die", 3);
//     printf("Parent process is going to die!\n");
// }
int main(int argc, char *argv[])
{

    char msgFromClient[256] = {0};
    char cmd[128] = {0};
    char srcPath[128] = {0};
    char dstPath[128] = {0};
    char Token[4096] = {0};

    // 创建线程池
    int workerNum = atoi(threadNum);
    threadPool_t threadPool;
    threadPoolInit(&threadPool, workerNum); // 初始化线程池

    // TCP连接
    int sockFd;
    tcpInit(&sockFd, ip, port); // 初始化TCP链接

    // 连接数据库
    MYSQL *mysql = mysqlConnect(DBHost, DBUser, DBPasswd, DBName);

    // 创建时间轮,并初始化
    time_wheel_t tw;
    time_wheel_init(&tw);

    // 监听
    int epFd = epoll_create(1); // 创建epoll对象
    makeWorker(&threadPool);    // 创建子线程
    threadPool.epollFd = epFd;
    epollAdd(sockFd, epFd);
    int listenNum = workerNum + 1;
    struct epoll_event readyArr[listenNum];

    user_t user;
    time_t timer = time(NULL);
    while (1)
    {
        bzero(&user, sizeof(user));
        int readyNum = epoll_wait(epFd, readyArr, listenNum, 1000);
        ERROR_CHECK(readyNum, -1, "epoll_wait");
        for (int i = 0; i < readyNum; ++i)
        {
            // 事件一：有新连接，先验证密码，再增加监听
            if (readyArr[i].data.fd == sockFd)
            {
                int netFd = accept(sockFd, NULL, NULL);
                ERROR_CHECK(netFd, -1, "accept");
                printf("New connection is connecting...\n\n");

                strcmp(threadPool.pathBuffer[netFd], "/");
                while (1)
                {
                    bzero(msgFromClient, sizeof(msgFromClient));
                    ssize_t recvRet = recvMessageFromClient(netFd, msgFromClient, sizeof(msgFromClient));
                    if (!recvRet)
                    {
                        close(netFd);
                        break;
                    }

                    // 登录
                    if (strcmp(msgFromClient, "log in") == 0)
                    {
                        ssize_t recvRet = recv(netFd, &user, sizeof(user), MSG_WAITALL);
                        ERROR_CHECK(recvRet, -1, "recv");
                        if (!recvRet)
                        {
                            close(netFd);
                            break;
                        }
                        int loadRet = loadingUserInfo(mysql,user); // 密码验证
                        if (loadRet == 0)
                        { // 密码验证正确
                            ssize_t sendRet = send(netFd, "passwdOK", 8, 0);
                            ERROR_CHECK(sendRet, -1, "send");
                            bzero(Token,sizeof(Token));
                            createToken(user.userName, Token);
                            sendTokenToClient(netFd, user.userName, Token);
                            bzero(threadPool.userBuffer[netFd], sizeof(threadPool.userBuffer[netFd]));
                            strcpy(threadPool.userBuffer[netFd], user.userName);
                            bzero(threadPool.pathBuffer[netFd], sizeof(threadPool.pathBuffer[netFd]));
                            strcpy(threadPool.pathBuffer[netFd], "/");
                            epollAdd(netFd, epFd);      // 将此连接加入监听
                            time_wheel_add(&tw, netFd); // 计时，超时断开连接
                            printf("[%s] is online...\n\n", user.userName);
                            break;
                        }
                        else if (loadRet == -999)
                        {
                            ssize_t sendRet = send(netFd, "noUser", 6, 0);
                            ERROR_CHECK(sendRet, -1, "send");
                        }
                        else
                        {
                            ssize_t sendRet = send(netFd, "passwdWrong", 11, 0);
                            ERROR_CHECK(sendRet, -1, "send");
                        }
                    }
                    // 注册
                    else if (strcmp(msgFromClient, "sign up") == 0)
                    {
                        ssize_t recvRet = recv(netFd, &user, sizeof(user), MSG_WAITALL);
                        ERROR_CHECK(recvRet, -1, "recv");
                        if (!recvRet)
                        {
                            close(netFd);
                            break;
                        }
                        int signRet = registration(mysql,user);
                        if (signRet == 1)
                        { // 注册成功
                            send(netFd, "registOk", 8, 0);
                            bzero(threadPool.userBuffer[netFd], sizeof(threadPool.userBuffer[netFd]));
                            strcpy(threadPool.userBuffer[netFd], user.userName);
                            insertTreeRoot(mysql, user.userName);
                            bzero(threadPool.pathBuffer[netFd], sizeof(threadPool.pathBuffer[netFd]));
                            strcpy(threadPool.pathBuffer[netFd], "/");
                            epollAdd(netFd, epFd);
                            time_wheel_add(&tw, netFd);
                            break;
                        }
                        else
                        {
                            send(netFd, "userExist", 9, 0);
                        }
                    }
                    // gets 或者 puts 命令的新连接，交给子线程处理
                    else if (strcmp(msgFromClient, "puts") == 0 || strcmp(msgFromClient, "gets") == 0)
                    {
                        char recvFdFromClient[8] = {0};
                        recvMessageFromClient(netFd, recvFdFromClient, sizeof(recvFdFromClient));
                        int newClientFd = atoi(recvFdFromClient);

                        bzero(threadPool.userBuffer[netFd], sizeof(threadPool.userBuffer[netFd]));
                        strcpy(threadPool.userBuffer[netFd], threadPool.userBuffer[newClientFd]);
                        bzero(threadPool.pathBuffer[netFd], sizeof(threadPool.pathBuffer[netFd]));
                        strcpy(threadPool.pathBuffer[netFd], threadPool.pathBuffer[newClientFd]);

                        pthread_mutex_lock(&threadPool.mutex);
                        taskEnqueue(&threadPool.taskQueue, netFd);
                        pthread_cond_signal(&threadPool.cond);
                        pthread_mutex_unlock(&threadPool.mutex);
                        break;
                    }

                } // end while(1)
            }     // end if (readyArr[i].data.fd == sockFd)

            // 事件二：处理短命令，主线程处理
            else
            {
                int netFd = readyArr[i].data.fd;
                time_wheel_add(&tw, netFd);
                int recvRet = recvMessageFromClient(netFd, msgFromClient, sizeof(msgFromClient));
                if (!recvRet)
                {
                    close(netFd);
                    epollDel(netFd, epFd);
                    printf("[%d] disconnected......\n", netFd);
                    continue;
                }
                LOG_WRITE(msgFromClient);

                // 命令解析
                bzero(cmd, sizeof(cmd));
                bzero(srcPath, sizeof(srcPath));
                bzero(srcPath, sizeof(srcPath));
                cutCmd(msgFromClient, cmd, srcPath, dstPath);
                threadPool_t *pThreadPool = &threadPool;

                if (strcmp(cmd, "cd") == 0)
                {
                    puts("\n===================================");
                    puts("|                cd               |");
                    puts("===================================");
                    int cdRet = changeDir(mysql, pThreadPool->userBuffer[netFd], pThreadPool->pathBuffer[netFd], srcPath);
                    if (cdRet == -1)
                    {
                        sendMessageToClient(netFd, "cd: directory does not exist", 28);
                    }
                    else
                    {
                        sendMessageToClient(netFd, "Not display", 11);
                    }
                    sendCurPath(netFd, pThreadPool->pathBuffer[netFd], sizeof(pThreadPool->pathBuffer[netFd]));
                }
                else if (strcmp(cmd, "ls") == 0)
                {
                    puts("\n===================================");
                    puts("|                ls               |");
                    puts("===================================");
                    char lsTxt[512] = {0};
                    listSearch(mysql, pThreadPool->userBuffer[netFd], pThreadPool->pathBuffer[netFd], lsTxt);
                    strcat(lsTxt, " ");
                    sendMessageToClient(netFd, lsTxt, strlen(lsTxt));
                    sendCurPath(netFd, pThreadPool->pathBuffer[netFd], sizeof(pThreadPool->pathBuffer[netFd]));
                }
                else if (strcmp(cmd, "pwd") == 0)
                {
                    puts("\n===================================");
                    puts("|                pwd              |");
                    puts("===================================");
                    char curPath[256] = {0};
                    strcpy(curPath, "~");
                    strcat(curPath, pThreadPool->pathBuffer[netFd]);
                    sendMessageToClient(netFd, curPath, strlen(curPath));
                    sendCurPath(netFd, pThreadPool->pathBuffer[netFd], sizeof(pThreadPool->pathBuffer[netFd]));
                }
                else if (strcmp(cmd, "rm") == 0)
                {
                    puts("\n===================================");
                    puts("|                 rm              |");
                    puts("===================================");
                    int rmRet = removeFile(mysql, pThreadPool->userBuffer[netFd], pThreadPool->pathBuffer[netFd], srcPath);
                    char rmFileMsg[256] = {0};
                    if (!rmRet)
                    {
                        sprintf(rmFileMsg, "rm [%s] is Fail", srcPath);
                    }
                    else
                    {
                        sprintf(rmFileMsg, "rm [%s] is Ok", srcPath);
                    }
                    sendMessageToClient(netFd, rmFileMsg, strlen(rmFileMsg));
                    sendCurPath(netFd, pThreadPool->pathBuffer[netFd], sizeof(pThreadPool->pathBuffer[netFd]));
                }
                else if (strcmp(cmd, "mkdir") == 0)
                {
                    puts("\n===================================");
                    puts("|               mkdir             |");
                    puts("===================================");
                    int mkdirRet = makeDir(mysql, pThreadPool->userBuffer[netFd], pThreadPool->pathBuffer[netFd], srcPath);
                    char mkDirMsg[256] = {0};
                    if (!mkdirRet)
                    {
                        sprintf(mkDirMsg, "mkdir [%s] is Fail", srcPath);
                    }
                    else
                    {
                        sprintf(mkDirMsg, "mkdir [%s] is Ok", srcPath);
                    }
                    sendMessageToClient(netFd, mkDirMsg, sizeof(mkDirMsg));
                    sendCurPath(netFd, pThreadPool->pathBuffer[netFd], sizeof(pThreadPool->pathBuffer[netFd]));
                }
                else if (strcmp(cmd, "rmdir") == 0)
                {
                    puts("\n===================================");
                    puts("|               rmdir             |");
                    puts("===================================");
                    int rmdirRet = removeDir(mysql, pThreadPool->userBuffer[netFd], pThreadPool->pathBuffer[netFd], srcPath);
                    char rmDirMsg[256] = {0};
                    if (!rmdirRet)
                    {
                        sprintf(rmDirMsg, "rmdir [%s] is Fail", srcPath);
                    }
                    else
                    {
                        sprintf(rmDirMsg, "rmdir [%s] is OK", srcPath);
                    }
                    sendMessageToClient(netFd, rmDirMsg, strlen(rmDirMsg));
                    sendCurPath(netFd, pThreadPool->pathBuffer[netFd], sizeof(pThreadPool->pathBuffer[netFd]));
                }
                // 移交长命令给子线程处理
                else if (strcmp(cmd, "puts") == 0 || strcmp(cmd, "gets") == 0)
                {
                    printf("=======================================\n");
                    printf("|                 %s                |\n", cmd);
                    printf("=======================================\n");
                    char sendFdToClient[8] = {0};
                    sprintf(sendFdToClient, "%d", netFd);
                    sendMessageToClient(netFd, sendFdToClient, strlen(sendFdToClient));
                    sendCurPath(netFd, pThreadPool->pathBuffer[netFd], sizeof(pThreadPool->pathBuffer[netFd]));
                }
            } // end else
        }     // end for (int i = 0; i < readyNum; ++i)

        if (time(NULL) - timer > 0)
        {
            advance(&tw);
            timer = time(NULL);
        }
    }
    return 0;
}