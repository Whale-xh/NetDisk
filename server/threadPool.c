#include "server.h"
#include <func.h>

int threadPoolInit(threadPool_t *pThreadPool, int workerNum)
{
    pThreadPool->workerNum = workerNum;
    pThreadPool->workerArr = (pthread_t *)calloc(workerNum, sizeof(pthread_t));
    pThreadPool->taskQueue.pFront = NULL;
    pThreadPool->taskQueue.pRear = NULL;
    pThreadPool->taskQueue.size = 0;
    pThreadPool->epollFd = -1;
    bzero(pThreadPool->pathBuffer, sizeof(pThreadPool->pathBuffer));
    bzero(pThreadPool->userBuffer, sizeof(pThreadPool->userBuffer));
    pthread_mutex_init(&pThreadPool->mutex, NULL);
    pthread_cond_init(&pThreadPool->cond, NULL);
    return 0;
}

int taskEnqueue(taskQueue_t *pTaskQueue, int netFd) // 任务入队
{
    task_t *pTask = (task_t *)calloc(1, sizeof(task_t));
    pTask->netFd = netFd;
    if (pTaskQueue->size == 0)
    {
        pTaskQueue->pFront = pTask;
        pTaskQueue->pRear = pTask;
    }
    else
    {
        pTaskQueue->pRear->pNext = pTask;
        pTaskQueue->pRear = pTask;
    }
    ++pTaskQueue->size;
    return 0;
}

int taskDequeue(taskQueue_t *pTaskQueue) // 任务出队
{
    task_t *pCur = pTaskQueue->pFront;
    pTaskQueue->pFront = pCur->pNext;
    free(pCur);
    --pTaskQueue->size;
    return 0;
}