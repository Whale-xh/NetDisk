#include <func.h>

int tcpInit(int *pSockFd, char *ip, char *port)
{
    *pSockFd = socket(AF_INET, SOCK_STREAM, 0);
    ERROR_CHECK(*pSockFd, -1, "socket");
    struct sockaddr_in addr;
    addr.sin_addr.s_addr = inet_addr(ip);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(port));
    int reuse = 1;
    int ret = setsockopt(*pSockFd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    ERROR_CHECK(ret, -1, "setsocket");
    ret = bind(*pSockFd, (struct sockaddr *)&addr, sizeof(addr));
    ERROR_CHECK(ret, -1, "bind");
    listen(*pSockFd, 10);
    return 0;
}