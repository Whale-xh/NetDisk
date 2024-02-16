#include "server.h"

// 连接数据库
MYSQL *mysqlConnect(char *pDBHost, char *pDBUser, char *pDBPasswd, char *pDBName)
{
    MYSQL *mysql = mysql_init(NULL);
    MYSQL *ret = mysql_real_connect(mysql, pDBHost, pDBUser, pDBPasswd, pDBName, 0, NULL, 0);
    if (!ret)
    {
        fprintf(stderr, "mysql_real_connect: %s\n", mysql_error(mysql));
        return NULL;
    }
    return mysql;
}

// 加载用户信息
int loadingUserInfo(MYSQL *mysql,user_t tmpuser)
{
    char sql[2048] = {0};
    sprintf(sql, "select salt, passwd, tomb from user where username = '%s';", tmpuser.userName);
    int qret = mysql_query(mysql, sql);
    if (qret != 0)
    {
        fprintf(stderr, "mysql_query:%s\n", mysql_error(mysql));
        return -1;
    }
    // 将找到的密码读取出来，如果没有找到，则说明用户名不正确
    // 或者用户并未注册
    MYSQL_RES *result = mysql_store_result(mysql);
    MYSQL_ROW row;
    row = mysql_fetch_row(result);

    if (!row)
    {
        return -999;
    }

    if (row[1] != NULL)
    {
        if (row[2][0] == '0')
        {
            char *salt_result = crypt(tmpuser.passwd, row[0]);
            char *compare = row[1];
            return strcmp(compare, salt_result);
        }
        else
        {
            fprintf(stderr, "用户已注销！\n");
            return -999;
        }
    }
    else
    {
        return -999;
    }
    return 0;
}

int salt(char *saltnum)
{
    srand(time(NULL));
    printf("salt begin\n");
    saltnum[0] = '$';
    saltnum[1] = '6';
    saltnum[2] = '$';
    // 随机生成中间16位
    const char char16[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

    for (int i = 0; i < 16; i++)
    {
        saltnum[3 + i] = char16[rand() % (sizeof(char16) - 1)];
    }
    saltnum[19] = '$';
    printf("salt end\n");

    return 0;
}

int registration(MYSQL *mysql,user_t tmpuser)
{
    // 获取盐值
    char saltnum[128] = {0};
    salt(saltnum);
    // 加密
    puts(saltnum);
    char *saltresult = crypt(tmpuser.passwd, saltnum);

    memcpy(tmpuser.passwd, saltresult, 20);
    puts(saltresult);
    // 先查询用户名是否重复，不能申请已注册的用户名
    char sql[2048] = {0};
    sprintf(sql, "select username from user where username = '%s';", tmpuser.userName);
    int qret = mysql_query(mysql, sql);
    if (qret != 0)
    {
        fprintf(stderr, "mysql_query:%s\n", mysql_error(mysql));
        return -1;
    }
    MYSQL_RES *result = mysql_store_result(mysql);
    MYSQL_ROW row;
    if ((row = mysql_fetch_row(result)) != NULL)
    {
        fprintf(stderr, "userName has exited!\n");
        return 0;
    }

    // 注册，添加新用户
    memset(sql, 0, sizeof(sql));
    sprintf(sql, "insert into user (username,salt,passwd,tomb) values ('%s','%s','%s',0);", tmpuser.userName, saltnum, saltresult);
    qret = mysql_query(mysql, sql);
    if (qret != 0)
    {
        fprintf(stderr, "mysql_query:%s\n", mysql_error(mysql));
        return -1;
    }

    return 1;
}

// 用户第一次注册时，更新tree表,插入当前目录
int insertTreeRoot(MYSQL *mysql, char *userName)
{
    time_t createtime;
    time(&createtime);
    char *Time = ctime(&createtime);
    if (Time[strlen(Time) - 1] == '\n')
        Time[strlen(Time) - 1] = '\0';
    char sql[1024] = {0};
    sprintf(sql, "insert into tree (name, username, path, pre_id, md5, size, createtime, tomb) values ('/', '%s', '/', -1, '0', 0, '%s',0);", userName, Time);
    int qret = mysql_query(mysql, sql);
    if (qret != 0)
    {
        fprintf(stderr, "mkDir:%s\n", mysql_error(mysql));
    }
    return 0;
}

int changePath_1(MYSQL *mysql, const char *username, char *curr_path, const char *dest_path)
{
    char sq[1024]; // query字符串,保存mysql指令

    // cd 不接地址，直接回到根目录
    if (strlen(dest_path) == 0)
    {
        sprintf(curr_path, "/");
    }

    // cd 接. 无事发生
    if (strcmp(dest_path, ".") == 0)
    {
        return 0;
    }

    // cd 接.. 回到上级目录(如果已经在根目录就什么都不做)
    if (strcmp(dest_path, "..") == 0)
    {
        if (strcmp(curr_path, "/") == 0)
        {
            return 0;
        }
        // 找到当前目录的上级目录的id
        sprintf(sq, "select pre_id from tree where username = '%s' and path = '%s';", username, curr_path);
        int qret = mysql_query(mysql, sq);
        QUERY_CHECK(qret, mysql);
        MYSQL_RES *result = mysql_store_result(mysql);
        MYSQL_ROW row = mysql_fetch_row(result);
        // row[0]就是上级目录的id

        // 找到上级目录的路径
        sprintf(sq, "select path from tree where id = '%s';", row[0]);
        qret = mysql_query(mysql, sq);
        QUERY_CHECK(qret, mysql);
        result = mysql_store_result(mysql);
        row = mysql_fetch_row(result);
        // row[0]就是path
        strcpy(curr_path, row[0]);
        return 0;
    }

    // 不是.或..
    // 找到当前所在目录在数据库中的id
    sprintf(sq, "select id from tree where username = '%s' and path = '%s';", username, curr_path);
    int qret = mysql_query(mysql, sq);
    QUERY_CHECK(qret, mysql);

    MYSQL_RES *result = mysql_store_result(mysql);
    MYSQL_ROW row = mysql_fetch_row(result);
    // row[0]就是当前目录的id

    // 找到pre_id = row[0] 的所有表项
    sprintf(sq, "select path from tree where pre_id = %s and tomb = 0 ;", row[0]);
    qret = mysql_query(mysql, sq);
    QUERY_CHECK(qret, mysql);

    result = mysql_store_result(mysql);
    char path[1024] = {0};
    if (strcmp(curr_path, "/") == 0)
    {
        sprintf(path, "%s%s", curr_path, dest_path);
    }
    else
    {
        sprintf(path, "%s/%s", curr_path, dest_path);
    }
    while ((row = mysql_fetch_row(result)) != NULL)
    {
        // printf("path=%s,row[0]=%s\n",path,row[0]);
        if (strcmp(path, row[0]) == 0)
        {
            strcpy(curr_path, row[0]);
            return 0;
        }
    }

    return -1;
}

int changePath(MYSQL *mysql, const char *username, char *curr_path, const char *dest_path)
{

    if (strlen(dest_path) == 0)
    {
        strcpy(curr_path, "/");
        return 0;
    }
    char dest[1024] = {0};
    strcpy(dest, dest_path);
    char *p = strtok(dest, "/");
    if (p)
    {
        int ret = changePath_1(mysql, username, curr_path, p);
        printf("*p=%s,curr_path= %s\n", p, curr_path);
        if (ret == -1)
        {
            return -1;
        }
    }
    while (1)
    {
        p = strtok(NULL, "/");
        if (p)
        {
            int ret = changePath_1(mysql, username, curr_path, p);
            printf("*p=%s,curr_path= %s\n", p, curr_path);
            if (ret == -1)
            {
                return -1;
            }
        }
        else
        {
            break;
        }
    }
    return 0;
}

// 删除文件
int removeFile(MYSQL *mysql, const char *userName, char *curr_path, char *filePath)
{
    char path[256] = {0};
    memcpy(path, curr_path, strlen(curr_path));
    if (changePath(mysql, userName, path, filePath) == -1)
    {
        return 0;
    }
    else
    {
        char sql[1024] = {0};
        sprintf(sql, "update tree set tomb=1 where path='%s' and username = '%s';", path, userName);
        int qret = mysql_query(mysql, sql);
        if (qret != 0)
        {
            fprintf(stderr, "rm:%s\n", mysql_error(mysql));
        }
        return 1;
    }
}

// path=/path1/path2/path3 则找的是path3的id
int getId(MYSQL *mysql, char *username, char *path, int *pid)
{
    char sql[1024] = {0};
    sprintf(sql, "select id from tree where username='%s' and path='%s';", username, path);
    int qret = mysql_query(mysql, sql);
    if (qret != 0)
    {
        fprintf(stderr, "file id:%s\n", mysql_error(mysql));
    }
    MYSQL_RES *result = mysql_store_result(mysql);
    MYSQL_ROW row = mysql_fetch_row(result);
    printf("atoi begin\n");
    printf("file id = %s\n", row[0]);
    *pid = atoi(row[0]);
    return 0;
}

// path=/path1/path2/path3  则left=/path1/path2  right=path3
int cutTail(char *path, char *left, char *right)
{
    if (strcmp(path, ".") == 0 || strcmp(path, "./") == 0)
    {
        strcpy(left, ".");
        strcpy(right, ".");
        return 0;
    }
    if (strcmp(path, "..") == 0 || strcmp(path, "../") == 0)
    {
        strcpy(left, "..");
        strcpy(right, "..");
        return 0;
    }
    char tmp[1024] = {0};
    memcpy(tmp, path, strlen(path));
    memcpy(left, path, strlen(path));
    char *p = strtok(tmp, "/");
    while (1)
    {
        char *tmp = strtok(NULL, "/");
        if (tmp == NULL)
        {
            memcpy(right, p, strlen(p) + 1);
            if (strlen(path) == strlen(right))
            {
                strcpy(left, ".");
                break;
            }
            left[strlen(path) - strlen(right) - 1] = '\0';
            if (strlen(path) - strlen(right) - 1 == 0)
            {
                left[0] = '/';
            }
            break;
        }
        p = tmp;
    }
    return 0;
}

// 创建目录
int makeDir(MYSQL *mysql, char *userName, char *curr_path, char *dirpath)
{
    char path[256] = {0};
    memcpy(path, curr_path, strlen(curr_path));
    printf("curPath = %s\n", path);
    printf("userName = %s\n", userName);
    printf("dirpath = %s\n", dirpath);
    if (changePath(mysql, userName, path, dirpath) != -1)
    {
        char sql[1024] = {0};
        sprintf(sql, "select * from tree where path = '%s' and username = '%s' and tomb = 1;", path, userName);
        int qret = mysql_query(mysql, sql);
        if (qret != 0)
        {
            fprintf(stderr, "makeDir:%s\n", mysql_error(mysql));
        }
        MYSQL_RES *result = mysql_store_result(mysql);
        MYSQL_ROW row = mysql_fetch_row(result);
        if (!row)
        {
            printf("文件夹已存在\n");
            return 0;
        }
        else
        {
            bzero(sql, sizeof(sql));
            time_t createtime;
            time(&createtime);
            char *t = ctime(&createtime);
            if (t[strlen(t) - 1] == '\n')
                t[strlen(t) - 1] = '\0';
            sprintf(sql, "update tree set tomb = 0, createtime = '%s' where path = '%s' and username = '%s';", t, path, userName);
            int qret = mysql_query(mysql, sql);
            if (qret != 0)
            {
                fprintf(stderr, "makeDir:%s\n", mysql_error(mysql));
            }
            return 1;
        }
    }
    else
    {
        puts("555");
        bzero(path, sizeof(path));
        memcpy(path, curr_path, strlen(curr_path));
        char left[128] = {0}, right[128] = {0};
        cutTail(dirpath, left, right);
        printf("left = %s\nright = %s\n", left, right);
        if (changePath(mysql, userName, path, left) == -1)
        {
            printf("路径不存在\n");
            return 0;
        }
        else
        {
            int id;
            getId(mysql, userName, path, &id);
            if (strcmp(path, "/"))
            {
                strcat(path, "/");
            }
            strcat(path, right);
            char sql[1024] = {0};

            time_t createtime;
            time(&createtime);
            char *t = ctime(&createtime);
            if (t[strlen(t) - 1] == '\n')
                t[strlen(t) - 1] = '\0';
            sprintf(sql, "insert into tree (name,username,path,pre_id,md5,tomb,size, createtime) values('%s','%s','%s',%d,'0',0,0, '%s') ;", right, userName, path, id, t);
            int qret = mysql_query(mysql, sql);
            if (qret != 0)
            {
                fprintf(stderr, "makeDir:%s\n", mysql_error(mysql));
            }
        }
    }
    return 1;
}

// 改变路径，dest_path为0则进入根目录
int changeDir(MYSQL *mysql, const char *username, char *curr_path, const char *dest_path)
{
    if (strlen(dest_path) == 0)
    {
        strcpy(curr_path, "/");
        return 0;
    }
    char format_dest[1024] = {0};
    pathFormat(format_dest, dest_path);
    printf("format_dest = %s \n", format_dest);
    if (strlen(format_dest) == 0)
    {
        return 0;
    }
    char *buf;
    int go_up = 0;

    char path[128][128];
    bzero(path, sizeof(path));
    int top = 0;
    while (1)
    {
        if (go_up == 0 && top == 0)
        {
            buf = strtok(format_dest, "/");
        }
        else
        {
            buf = strtok(NULL, "/");
        }
        if (buf == NULL)
        {
            break;
        }
        if (strcmp(buf, "..") == 0)
        {
            ++go_up;
        }
        else
        {
            strcpy(path[top], buf);
            ++top;
        }
    }

    bzero(format_dest, sizeof(format_dest));
    for (int i = 0; i < top; ++i)
    {
        strcat(format_dest, path[i]);
        if (i < top - 1)
        {
            strcat(format_dest, "/");
        }
    }

    printf("go up %d, format_dest = %s\n", go_up, format_dest);

    char up_path[512] = {0};
    strcpy(up_path, curr_path);
    if (go_up)
    {
        int i;
        for (i = strlen(curr_path) - 1; i > 1; --i)
        {
            printf("up_path[%d]=%c\n", i, up_path[i]);
            if (up_path[i] == '/')
            {
                --go_up;
            }
            if (go_up == 0)
            {
                break;
            }
        }
        bzero(up_path, sizeof(up_path));
        strncpy(up_path, curr_path, i);
    }

    printf("up_path = %s\n", up_path);

    if (strlen(format_dest))
    {
        if (strcmp(up_path, "/") == 0)
        {
            strcat(up_path, format_dest);
        }
        else
        {
            strcat(up_path, "/");
            strcat(up_path, format_dest);
        }
    }
    //    printf("final path = %s \n",up_path);

    char sq[1024] = {0};
    // fprintf(stdout,"select * from tree where path = '%s' and user = '%s' and tomb = 0;\n",up_path,username);
    sprintf(sq, "select * from tree where path = '%s' and username = '%s' and tomb = 0 and md5 = '0';", up_path, username);
    int qret = mysql_query(mysql, sq);
    QUERY_CHECK(qret, mysql);

    MYSQL_RES *result = mysql_store_result(mysql);
    MYSQL_ROW row = mysql_fetch_row(result);
    if (row == NULL)
    {
        return -1;
    }
    else
    {
        strcpy(curr_path, up_path);
        return 0;
    }
}

// 列出当前目录文件
int listSearch(MYSQL *mysql, const char *username, const char *curr_path, char *list)
{
    // sq保存query语句
    char sq[1024] = {0};
    sprintf(sq, "select id from tree where username = '%s' and path = '%s';\n", username, curr_path);
    int qret = mysql_query(mysql, sq);
    QUERY_CHECK(qret, mysql);
    MYSQL_RES *result = mysql_store_result(mysql);
    MYSQL_ROW row = mysql_fetch_row(result);
    // row[0]就是当前目录的id
    sprintf(sq, "select name,md5,size,createtime from tree where pre_id = %s and tomb = 0;", row[0]);
    qret = mysql_query(mysql, sq);
    QUERY_CHECK(qret, mysql);
    result = mysql_store_result(mysql);
    while ((row = mysql_fetch_row(result)) != NULL)
    {
        strcat(list, row[3]);
        strcat(list, "  ");
        strcat(list, row[2]);
        strcat(list, "\t");
        strcat(list, row[0]);
        if (strcmp(row[1], "0") == 0)
        {
            strcat(list, "/");
        }
        strcat(list, "\n");
    }
    return 0;
}

// 删除目录
int removeDir(MYSQL *mysql, char *username, char *curr_path, char *dirpath)
{
    char path[256] = {0};
    memcpy(path, curr_path, strlen(curr_path));
    if (changePath(mysql, username, path, dirpath) == -1)
    {
        return 0;
    }
    else
    {
        char sql[1024] = {0};
        sprintf(sql, "select id from tree  where path='%s' and md5='0' and username = '%s';", path, username);
        int qret = mysql_query(mysql, sql);
        if (qret != 0)
        {
            fprintf(stderr, "rmdir select:%s\n", mysql_error(mysql));
        }
        MYSQL_RES *result = mysql_store_result(mysql);
        MYSQL_ROW row = mysql_fetch_row(result);
        if (row == NULL)
        {
            return 0;
        }
        else
        {
            bzero(sql, sizeof(sql));
            sprintf(sql, "select * from tree where pre_id=%d and username = '%s' and tomb = 0;", atoi(row[0]), username);
            qret = mysql_query(mysql, sql);
            if (qret != 0)
            {
                fprintf(stderr, "rmdir select:%s\n", mysql_error(mysql));
            }
            MYSQL_RES *ret = mysql_store_result(mysql);
            MYSQL_ROW r = mysql_fetch_row(ret);
            if (r == NULL)
            {
                bzero(sql, sizeof(sql));
                sprintf(sql, "update tree set tomb=1 where path='%s' and username = '%s';", path, username);
                qret = mysql_query(mysql, sql);
                if (qret != 0)
                {
                    fprintf(stderr, "rmdir update:%s\n", mysql_error(mysql));
                }
                return 1;
            }
            else
            {
                return 0;
            }
        }
    }
    return 0;
}

// 查找文件md5,找到返回1
int searchMd5(MYSQL *mysql, char *md5, char *size)
{
    char sql[1024] = {0};
    sprintf(sql, "select size from tree where md5='%s' and tomb = 0;", md5);
    int qret = mysql_query(mysql, sql);
    if (qret != 0)
    {
        fprintf(stderr, "searchMd5:%s\n", mysql_error(mysql));
    }
    MYSQL_RES *result = mysql_store_result(mysql);
    MYSQL_ROW row = mysql_fetch_row(result);
    if (row == NULL)
    {
        return 0;
    }
    else
    {
        strcpy(size, row[0]);
        return 1;
    }
}

// 查找数据库是否存在tomb=1且path=filepath的信息，有就将tomb置为0并返回1，没有就返回0
int delToMk(MYSQL *mysql, const char *username, char *curr_path, char *filepath)
{
    char path[256] = {0};
    memcpy(path, curr_path, strlen(curr_path));
    changePath(mysql, username, path, filepath);
    char sql[1024] = {0};
    sprintf(sql, "select * from tree where path='%s' and username='%s' and tomb=1;", path, username);
    int qret = mysql_query(mysql, sql);
    if (qret != 0)
    {
        fprintf(stderr, "delToMk:%s\n", mysql_error(mysql));
    }
    MYSQL_RES *ret = mysql_store_result(mysql);
    MYSQL_ROW row = mysql_fetch_row(ret);
    if (row != NULL)
    {

        memset(sql, 0, sizeof(sql));
        sprintf(sql, "update tree set tomb=0 where where path='%s' and username='%s';", path, username);
        qret = mysql_query(mysql, sql);
        if (qret != 0)
        {
            fprintf(stderr, "delToMk:%s\n", mysql_error(mysql));
        }
        return 1;
    }
    return 0;
}

// 插入文件到数据库
void insertFile(MYSQL *mysql, char *filename, char *filepath, int filesize, char *filemd5, char *user, int pre_id)
{
    char sql[1024] = {0};
    time_t createtime;
    time(&createtime);
    char *t = ctime(&createtime);
    if (t[strlen(t) - 1] == '\n')
        t[strlen(t) - 1] = '\0';
    sprintf(sql, "insert into tree (name,username,path, pre_id,md5,size,createtime,tomb) values('%s','%s','%s',%d ,'%s', %d,'%s', %d);", filename, user, filepath, pre_id, filemd5, filesize, t, 0);
    int qret = mysql_query(mysql, sql);
    if (qret != 0)
    {
        fprintf(stderr, "insertFile:%s\n", mysql_error(mysql));
    }
}

// 查找要下载的文件的md5
void searchFileMd5(MYSQL *mysql, char *md5, char *curpath, char *username, char *filename)
{
    char sql[1024] = {0};
    char path[128] = {0};
    strcpy(path, curpath);
    strcat(path, "/");
    strcat(path, filename);
    sprintf(sql, "select md5 from tree where path='%s' and username='%s' and name='%s';", path, username, filename);
    int qret = mysql_query(mysql, sql);
    if (qret != 0)
    {
        fprintf(stderr, "searchFileName:%s\n", mysql_error(mysql));
    }
    MYSQL_RES *ret = mysql_store_result(mysql);
    MYSQL_ROW row = mysql_fetch_row(ret);
    strcpy(md5, row[0]);
}