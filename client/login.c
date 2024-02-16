#include <func.h>
#include "client.h"

//用户登录
int enterUser(user_t *user)
{
    memset(user, 0, sizeof(user_t));
    char s1[] = "Enter username: ";
    write(STDOUT_FILENO, s1, strlen(s1));
    scanf("%s", user->userName);
    char *realPasswd = getpass("Enter password: ");
    strcpy(user->passwd, realPasswd);
    // strcpy(user->curPath,getcwd(NULL, 0));
    return 0;
}