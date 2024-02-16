#include "server.h"

// 时间轮初始化，给每个槽位创建一个集合，当前槽位指向0号
void time_wheel_init(time_wheel_t *wheel)
{
    for (int i = 0; i < TIME_WHEEL_SIZE; ++i)
    {
        wheel->set[i] = create_hash_set();
    }
    wheel->curr_slot = 0;
    wheel->table = create_hash_table();
    printf("current slot = %d \n", wheel->curr_slot);
}

// 向当前槽位里的集合添加fd
void time_wheel_add(time_wheel_t *tw, int fd)
{
    int slot = 0;
    int ret = search_table(tw->table, fd, &slot);
    if (ret == 0)
    {
        // 如果fd在其他槽里存在，从其他槽的集合里删去fd
        printf("fd exists in slot[%d],remove into slot[%d]\n", slot, tw->curr_slot);
        remove_element(tw->set[slot], fd);
        remove_element((HashSet *)tw->table, fd);
        insert_table(tw->table, fd, tw->curr_slot);
    }
    else
    {
        // 如果是新加的, 向哈希表里插入 fd-槽位 键值对
        printf("fd not exist in table,add fd[%d]-slot[%d] \n", fd, tw->curr_slot);
        insert_table(tw->table, fd, tw->curr_slot);
    }
    insert(tw->set[tw->curr_slot], fd);
}

// 时间轮向后走一个槽位
void advance(time_wheel_t *wheel)
{
    wheel->curr_slot = (wheel->curr_slot + 1) % TIME_WHEEL_SIZE;
    printf("current slot = %d \n", wheel->curr_slot);
    // 遍历哈希表,释放tcp连接
    for (int i = 0; i < TABLE_SIZE; i++)
    {
        Entry *current = wheel->set[wheel->curr_slot]->entries[i];
        while (current != NULL)
        {
            Entry *next = current->next;
            // 在这里把fd = current->key 的tcp连接断开
            close(current->key);
            printf("connect closed, fd = %d, curr_slot = %d \n", current->key, wheel->curr_slot);
            free(current);
            current = next;
        }
    }
    for (int i = 0; i < TABLE_SIZE; ++i)
    {
        wheel->set[wheel->curr_slot]->entries[i] = NULL;
    }
}

// int main(void){
//     //创建时间轮,并初始化
//     time_wheel_t tw;
//     time_wheel_init(&tw);
//     int fd;
//
//     //while(1){
//     //    while(1){
//     //        int result=scanf("%d",&fd);
//     //        if(result == EOF){
//     //            break;
//     //        }
//     //        time_wheel_add(&tw,fd);
//     //    }
//     //    advance(&tw);
//     //}
//
//     while (1) {
//         while (scanf("%d", &fd) != EOF) {
//             time_wheel_add(&tw, fd);
//         }
//         advance(&tw);
//         clearerr(stdin);  // 清除输入流错误标志
//     }
//
//     return 0;
// }
