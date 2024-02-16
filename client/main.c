#include "client.h"
#include <func.h>

char *ip = "192.168.47.128";
char *port = "1234";

int main(int argc, char *argv[])
{

    char cmd[128] = {0};
    char srcPath[128] = {0};
    char dstPath[128] = {0};
    char buf[256] = {0};
    char Name[128] = {0};
    char Token[4096] = {0};
    char curPath[4096] = {0};

    // TCP连接
    int sockFd = socket(AF_INET, SOCK_STREAM, 0);
    ERROR_CHECK(sockFd, -1, "socket");
    tcpConnect(sockFd, ip, port);

    puts("============================================");
    puts("|                  NetDisk                 |");
    puts("|              Author by whale             |");
    puts("============================================");
    puts("|              Suported commands:          |");
    puts("| cd、pwd、ls、rm、mkdir、rmdir、puts、gets |");
    puts("============================================");
    // 用户登录
    user_t user;
    bzero(&user, sizeof(user));
    while (1)
    {
        char msg[] = "Enter 'l' to log in,enter 's' to sign up: ";
        write(STDOUT_FILENO, msg, sizeof(msg));
        char word[32] = {0};
        scanf("%s", word);
        if (strcmp(word, "l") != 0 && strcmp(word, "s") != 0)
        {
            continue;
        }
        // 登录
        else if (strcmp(word, "l") == 0)
        {
            sendMessageToServer(sockFd, "log in", 6);
            enterUser(&user);
            
            ssize_t sendRet = send(sockFd, &user, sizeof(user), MSG_NOSIGNAL);
            ERROR_CHECK(sendRet, -1, "send");
            
            bzero(buf, sizeof(buf));
            ssize_t recvRet = recv(sockFd, buf, sizeof(buf), 0);
            if (!recvRet)
            {
                close(sockFd);
                return 0;
            }
            else if (strcmp(buf, "noUser") == 0)
            {
                printf("%s: user does not exist\n\n", user.userName);
                continue;
            }
            else if (strcmp(buf, "passwdOK") == 0)
            {
                bzero(Name, sizeof(Name));
                bzero(Token, sizeof(Token));
                recvTokenFromServer(sockFd, Name, Token);
                printf("Welcome %s\n\n", user.userName);
                print(user.userName, "/");
                break;
            }
            else if (strcmp(buf, "passwdWrong") == 0)
            {
                printf("Wrong password\n\n");
                continue;
            }
        }

        // 注册
        else if (strcmp(word, "s") == 0)
        {
            sendMessageToServer(sockFd, "sign up", 7);
            enterUser(&user);
            ssize_t sendRet = send(sockFd, &user, sizeof(user), MSG_NOSIGNAL);
            ERROR_CHECK(sendRet, -1, "send");
            bzero(buf, sizeof(buf));
            ssize_t recvRet = recv(sockFd, buf, 1, MSG_WAITALL);
            if (!recvRet)
            {
                close(sockFd);
                return 0;
            }
            else if (strcmp(buf, "userExist") == 0)
            {
                printf("%s: username already exist\n\n", user.userName);
                continue;
            }
            else if (strcmp(buf, "registOk") == 0)
            {
                printf("Sign up finished\n");
                print(user.userName, "/");
                break;
            }
        }
    } // end while(1)

    // 监听socket和STDIN
    int epFd = epoll_create(1);
    ERROR_CHECK(epFd, -1, "epoll_create");
    epollAdd(STDIN_FILENO, epFd);
    epollAdd(sockFd, epFd);
    struct epoll_event readyArr[2];
    char msgFromServer[128] = {0};
    // char errCmdPath[4096] = {0};
    while (1)
    {
        int readyNum = epoll_wait(epFd, readyArr, 2, -1);
        for (int i = 0; i < readyNum; ++i)
        {
            if (readyArr[i].data.fd == STDIN_FILENO)
            {
                char msgFromSTDIN[128] = {0};
                ssize_t stdinLength = recvStdin(msgFromSTDIN, sizeof(msgFromSTDIN));
                if (stdinLength == 0)
                {
                    close(epFd);
                    close(sockFd);
                    return 0;
                }
                if (strcmp(msgFromSTDIN, "\n") == 0)
                {
                    continue;
                }
                bzero(cmd, sizeof(cmd));
                bzero(srcPath, sizeof(srcPath));
                bzero(dstPath, sizeof(dstPath));
                int cutRet = cutCmd(msgFromSTDIN, cmd, srcPath, dstPath);
                if (!checkArguments(cmd, srcPath, dstPath, cutRet))
                {
                    print(user.userName, "/");
                    continue;
                }
                // puts 和 gets 命令交给子线程处理
                // gets fileName:先进入文件所在目录，然后执行该命令； puts fileName:先进入文件想要保存的目录，然后上传到当前目录
                // 后期可设计为可在绝对路径下载/上传文件
                if (strcmp(cmd, "puts") == 0 || strcmp(cmd, "gets") == 0)
                {
                    sendMessageToServer(sockFd, msgFromSTDIN, sizeof(msgFromSTDIN));

                    char recvFdFromServer[8] = {0};
                    recvMessageFromServer(sockFd, recvFdFromServer, sizeof(recvFdFromServer));

                    recvCurPath(sockFd, curPath, sizeof(curPath));

                    source_t source;
                    sourceInit(&source, atoi(recvFdFromServer), msgFromSTDIN, Name, cmd, srcPath, dstPath, Token, curPath);
                    // 创建线程处理命令
                    pthread_t thread;
                    pthread_create(&thread, NULL, threadFunc, &source);
                }
                else
                {
                    // 其他短命令发给服务器主线程处理
                    sendMessageToServer(sockFd, msgFromSTDIN, stdinLength);
                }
            }
            // 接收server服务器非gets/puts命令的数据
            else if (readyArr[i].data.fd == sockFd)
            {
                ssize_t recvRet = recvMessageFromServer(sockFd, msgFromServer, sizeof(msgFromServer));
                if (!recvRet)
                {
                    close(sockFd);
                    close(epFd);
                    return 0;
                }
                if (strcmp(msgFromServer, "Not display"))
                {
                    printf("%s\n", msgFromServer);
                }
                recvCurPath(sockFd, curPath, sizeof(curPath));
                print(user.userName, curPath);
            }
        }
    }

    return 0;
}