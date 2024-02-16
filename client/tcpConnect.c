#include <func.h>

//tcp连接
int tcpConnect(int sockFd, char *pIp, char *pPort)
{
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(pIp);
    addr.sin_port = htons(atoi(pPort));
    int ret = connect(sockFd, (struct sockaddr *)&addr, sizeof(addr));
    ERROR_CHECK(ret, -1, "connect");
    return 0;
}