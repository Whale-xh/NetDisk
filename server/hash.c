#include "server.h"

HashSet *create_hash_set()
{
    HashSet *set = (HashSet *)malloc(sizeof(HashSet));
    for (int i = 0; i < TABLE_SIZE; i++)
    {
        set->entries[i] = NULL;
    }
    return set;
}

int hash_function(int key)
{
    return key % TABLE_SIZE;
}

bool contains(HashSet *set, int key)
{
    int index = hash_function(key);
    Entry *current = set->entries[index];
    while (current != NULL)
    {
        if (current->key == key)
        {
            return true; // 元素存在于集合中
        }
        current = current->next;
    }
    return false; // 元素不存在于集合中
}

void insert(HashSet *set, int key)
{
    if (contains(set, key))
    {
        return; // 元素已经存在于集合中，不进行插入
    }
    int index = hash_function(key);
    Entry *entry = (Entry *)malloc(sizeof(Entry));
    entry->key = key;
    entry->next = set->entries[index];
    set->entries[index] = entry;
}

void remove_element(HashSet *set, int key)
{
    int index = hash_function(key);
    Entry *current = set->entries[index];
    Entry *previous = NULL;
    while (current != NULL)
    {
        if (current->key == key)
        {
            if (previous == NULL)
            {
                // 要删除的元素是链表的第一个节点
                set->entries[index] = current->next;
            }
            else
            {
                previous->next = current->next;
            }
            free(current);
            return;
        }
        previous = current;
        current = current->next;
    }
}

void destroy_hash_set(HashSet *set)
{
    for (int i = 0; i < TABLE_SIZE; i++)
    {
        Entry *current = set->entries[i];
        while (current != NULL)
        {
            Entry *next = current->next;
            free(current);
            current = next;
        }
    }
    free(set);
}

// int main() {
//     HashSet* set = create_hash_set();
//
//     // 向集合中插入元素
//     insert(set, 1);
//     insert(set, 2);
//     insert(set, 3);
//
//     // 检查元素是否存在于集合中
//     printf("Contains 2: %s\n", contains(set, 2) ? "true" : "false");
//     printf("Contains 4: %s\n", contains(set, 4) ? "true" : "false");
//
//     // 从集合中移除元素
//     remove_element(set, 2);
//
//     printf("Contains 2: %s\n", contains(set, 2) ? "true" : "false");
//     // 销毁集合
//     destroy_hash_set(set);
//
//     return 0;
// }
