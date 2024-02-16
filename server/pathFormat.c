#include "server.h"

// dest是格式化后的地址，src是格式化前的地址，注意：使用本函数前需要清空dest
//   src = ./         dest =
//   src = ../        dest = ..
//   src = ./path1/path2      dest = path1/path2
//   src = ./path1/path2/path3/    dest = path1/path2/path3
//   src = ../path1/path2          dest = ../path1/path2
int pathFormat(char *dest, const char *src)
{
    char path[1024] = {0};
    strcpy(path, src);
    // 申请一个地址队列
    pathlist_t list;
    // 初始化地址队列
    bzero(&list, sizeof(list));

    char *p = strtok(path, "/");
    if (p)
    {
        if (strcmp(p, "."))
        {
            strcpy(list.path[list.len], p);
            ++list.len;
        }
    }
    while (1)
    {
        p = strtok(NULL, "/");
        if (p)
        {
            if (strcmp(p, ".") == 0)
            {
                continue;
            }
            if (strcmp(p, "..") == 0)
            {
                if (list.len && strcmp(list.path[list.len - 1], ".."))
                {
                    --list.len;
                    continue;
                }
            }
            strcpy(list.path[list.len], p);
            ++list.len;
        }
        else
        {
            break;
        }
    }
    // 把格式化好的地址传入字符串
    for (int i = 0; i < list.len - 1; ++i)
    {
        strcat(dest, list.path[i]);
        strcat(dest, "/");
    }
    strcat(dest, list.path[list.len - 1]);
    return 0;
}

// int main(void){
//     char src[1024];
//     char dest[1024]={0};
//     while(1){
//         bzero(dest,sizeof(dest));
//         scanf("%s",src);
//         pathFormat(dest,src);
//         printf("%s\n",dest);
//     }
//
//     return 0;
// }
