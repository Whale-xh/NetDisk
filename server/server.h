#ifndef __SERVER_H__
#define __SERVER_H__

#include <func.h>
#include <shadow.h>
#include "jwt.h"

#define LOG_WRITE(msg)                                                                                                                                                                       \
    {                                                                                                                                                                                        \
        int fd = open("/var/log/syslog", O_WRONLY | O_APPEND | O_CREAT, 0666);                                                                                                               \
        time_t now = time(NULL);                                                                                                                                                             \
        time(&now);                                                                                                                                                                          \
        struct tm *localTime = localtime(&now);                                                                                                                                              \
        char message[4096] = {0};                                                                                                                                                            \
        sprintf(message, "[%d-%d-%d][%d:%d:%d]: %s\n", localTime->tm_year + 1900, localTime->tm_mon + 1, localTime->tm_mday, localTime->tm_hour, localTime->tm_min, localTime->tm_sec, msg); \
        write(fd, message, strlen(message));                                                                                                                                                 \
        close(fd);                                                                                                                                                                           \
    }

#define QUERY_CHECK(retval, mysql)                                      \
    do                                                                  \
    {                                                                   \
        if (retval)                                                     \
        {                                                               \
            fprintf(stderr, "mysql_query: %s\n", mysql_error((mysql))); \
            return (-1);                                                \
        }                                                               \
    } while (0)

// 文件传输协议
typedef struct train_s
{
    int length;
    char data[1024];
} train_t;

// 用户信息
typedef struct user_s
{
    char userName[128];
    char passwd[1024];
    // char curPath[128]; //用户当前工作目录
} user_t;

// 线程池
typedef struct task_s
{
    int netFd; // 上传或下载文件任务的客户端fd
    struct task_s *pNext;
} task_t;

typedef struct taskQueue_s
{
    task_t *pFront; // 队首指针
    task_t *pRear;  // 队尾指针
    int size;       // 队列长度
} taskQueue_t;

// 线程池共享资源
typedef struct threadPool_s
{
    int workerNum;             // 线程数量
    pthread_t *workerArr;      // 所有的线程 ID
    taskQueue_t taskQueue;     // 任务队列
    int epollFd;               // epoll 文件
    char pathBuffer[256][256]; // 用户当前工作路径的缓存
    char userBuffer[128][128]; // 当前用户名的缓存
    int socketFd;
    pthread_mutex_t mutex; // 锁
    pthread_cond_t cond;   // 条件变量
} threadPool_t;

// worker.c
int makeWorker(threadPool_t *pThreadPool); // 创建子线程

// threadPool.c
int threadPoolInit(threadPool_t *threadPool, int workerNum); 
int taskEnqueue(taskQueue_t *pTaskQueue, int netFd);         
int taskDequeue(taskQueue_t *pTaskQueue);                   

// command.c
int myCd(const char *path); // 进入某文件夹
int recvn(int sockFd, void *pStart, int len);
int cutPath(char *path, char *fileName);
int cutCmd(char *input, char *cmd, char *path1, char *path2);

// epollFunc.c
int epollAdd(int fd, int epFd); 
int epollDel(int fd, int epFd); 

// tcp.c
int tcpInit(int *pSockFd, char *ip, char *port); 

// transMSG.c
ssize_t recvMessageFromClient(int fd, char *buf, int bufLength);
int sendTokenToClient(int fd, char *username, char *token);
int sendMessageToClient(int fd, char *buf, int sendLength);
int sendCurPath(int fd, char *curPath, int sendLength);
int recvTokenFromClient(int fd, char *username, char *token);
int sendFileToClient(int fd, char *fileName, char *md5);
int recvFileFromClient(int fd, MYSQL *mysql, threadPool_t *pThreadPool, int pre_id, char *virtualPath);

// jwt.c
void createToken(char *username, char *token);

// sql.c
MYSQL *mysqlConnect(char *pDBHost, char *pDBUser, char *pDBPasswd, char *pDBName);
int loadingUserInfo(MYSQL *mysql,user_t tmpuser); 
int registration(MYSQL *mysql,user_t tmpuser);
int insertTreeRoot(MYSQL *mysql, char *userName);
int changePath_1(MYSQL *mysql, const char *username, char *curr_path, const char *dest_path);
int changePath(MYSQL *mysql, const char *username, char *curr_path, const char *dest_path);
int removeFile(MYSQL *mysql, const char *userName, char *curr_path, char *filePath);
int getId(MYSQL *mysql, char *username, char *path, int *pid);
int cutTail(char *path, char *left, char *right);
int makeDir(MYSQL *mysql, char *userName, char *curr_path, char *dirpath);
int changeDir(MYSQL *mysql, const char *username, char *curr_path, const char *dest_path);
int listSearch(MYSQL *mysql, const char *username, const char *curr_path, char *list);
int removeDir(MYSQL *mysql, char *username, char *curr_path, char *dirpath);
int searchMd5(MYSQL *mysql, char *md5, char *size);
int delToMk(MYSQL *mysql, const char *username, char *curr_path, char *filepath);
void insertFile(MYSQL *mysql, char *filename, char *filepath, int filesize, char *filemd5, char *user, int pre_id);
void searchFileMd5(MYSQL *mysql, char *md5, char *curpath, char *username, char *filename);

//路径列表
typedef struct pathlist_s
{
    char path[128][1024];
    int len;
} pathlist_t;

// pathFormat.c
int pathFormat(char *dest, const char *src);


//定时器设计
#define TABLE_SIZE 10

typedef struct Entry
{
    int key;
    int value;  
    struct Entry *next;
} Entry;

typedef struct
{
    Entry *entries[TABLE_SIZE];
} HashSet;

HashSet *create_hash_set();

int hash_function(int key);

bool contains(HashSet *set, int key);

void insert(HashSet *set, int key);

void remove_element(HashSet *set, int key);

void destroy_hash_set(HashSet *set);

typedef struct
{
    Entry *entries[TABLE_SIZE];
} HashTable;

HashTable *create_hash_table();

void insert_table(HashTable *table, int key, int value);

int search_table(HashTable *table, int key, int *pvalue);

#define TIME_WHEEL_SIZE 30

// 设置一个长度为30的循环队列
typedef struct time_wheel_s
{
    // 30个槽位，每个槽位一个集合
    HashSet *set[TIME_WHEEL_SIZE];
    // 当前的槽位
    int curr_slot;
    // 一个哈希表，用于记录fd在哪个槽位的集合里
    HashTable *table;
} time_wheel_t;

void time_wheel_init(time_wheel_t *wheel);

void time_wheel_add(time_wheel_t *tw, int fd);

void advance(time_wheel_t *wheel);




#endif