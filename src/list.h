#ifndef LIST_H
#define LIST_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct list_head
{
    struct list_head *prev;
    struct list_head *next;
};

#define container_of(ptr, type, member) ({              \
    const typeof(((type *)0)->member) *__mptr = (ptr);  \
    (type *)((char *)__mptr - offsetof(type, member));  \
})


#define list_is_empty(ptr)                      ((ptr)->prev == ptr)
#define list_entry(ptr, type, member)           container_of(ptr, type, member)
#define list_first_entry(ptr, type, member)     list_entry((ptr)->next, type, member)
#define list_last_entry(ptr, type, member)      list_entry((ptr)->prev, type, member)
#define list_next_entry(ptr, member)            list_entry((ptr)->member.next, typeof(*ptr), member)
#define list_prev_entry(ptr, member)            list_entry((ptr)->member.prev, typeof(*ptr), member)

#define list_for_each(pos, head)    \
    for (pos = (head)->next; pos != (head); pos = pos->next)

#define list_for_each_entry(pos, head, member) \
    for (pos = list_first_entry(head, typeof(*pos), member); \
    &((pos)->member) != (head); \
    pos = list_next_entry(pos, member))

#define list_for_each_entry_reverse(pos, head, member) \
    for (pos = list_last_entry(head, typeof(*pos), member); \
    &((pos)->member) != (head); \
    pos = list_prev_entry(pos, member))

#define list_for_each_entry_safe(pos, n, head, member) \
    for (pos = list_first_entry(head, typeof(*pos), member), n = list_next_entry(pos, member); \
    &((pos)->member) != (head); \
    pos = n, n = list_next_entry(n, member))

#define list_for_each_entry_reverse_safe(pos, n, head, member) \
    for (pos = list_last_entry(head, typeof(*pos), member), n = list_prev_entry(pos, member); \
    &((pos)->member) != (head); \
    pos = n, n = list_prev_entry(n, member))

static inline void list_init(struct list_head *node)
{
    node->prev = node;
    node->next = node;
}

static inline void __list_del(struct list_head *prev, struct list_head *next)
{
    prev->next = next;
    next->prev = prev;
}

static inline void list_del(struct list_head *node)
{
    __list_del(node->prev, node->next);
}

static inline void list_add(list_head *newNode, list_head *node)
{
    newNode->next       = node->next;
    newNode->prev       = node;
    node->next->prev    = newNode;
    node->next          = newNode;
}

static inline void list_add_prev(list_head *newNode, list_head *node)
{
    newNode->next       = node;
    newNode->prev       = node->prev;
    node->prev->next    = newNode;
    node->prev          = newNode;
}

static inline void list_add_head(list_head *node, list_head *head)
{
    list_add(node, head);
}

static inline void list_add_tail(list_head *node, list_head *head)
{
    // node->prev          = head->prev;
    // node->next          = head;
    // head->prev->next    = node;
    // head->prev          = node;
    list_add_prev(node, head);
}

#define LIST_ENTRY(ptr, type, member)   ((char*)(ptr) - offsetof(type, member))
#define GET_LIST(ptr, type, member)     ((char*)(ptr) + offsetof(type, member))

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* LIST_H */
