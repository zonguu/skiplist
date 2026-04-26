#ifndef LIST_ATOMIC_H
#define LIST_ATOMIC_H

#include <stddef.h>
#include <atomic>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct atomic_list_head
{
    std::atomic<struct atomic_list_head*> prev;
    std::atomic<struct atomic_list_head*> next;
};

#define atomic_container_of(ptr, type, member) ({              \
    const typeof(((type *)0)->member) *__mptr = (ptr);  \
    (type *)((char *)__mptr - offsetof(type, member));  \
})


#define atomic_list_is_empty(ptr)                      ((ptr)->prev.load(std::memory_order_acquire) == (ptr))
#define atomic_list_entry(ptr, type, member)           atomic_container_of(ptr, type, member)
#define atomic_list_first_entry(ptr, type, member)     atomic_list_entry((ptr)->next.load(std::memory_order_acquire), type, member)
#define atomic_list_last_entry(ptr, type, member)      atomic_list_entry((ptr)->prev.load(std::memory_order_acquire), type, member)
#define atomic_list_next_entry(ptr, member)            atomic_list_entry((ptr)->member.next.load(std::memory_order_acquire), typeof(*ptr), member)
#define atomic_list_prev_entry(ptr, member)            atomic_list_entry((ptr)->member.prev.load(std::memory_order_acquire), typeof(*ptr), member)

#define atomic_list_for_each(pos, head)    \
    for (pos = (head)->next.load(std::memory_order_acquire); pos != (head); pos = pos->next.load(std::memory_order_acquire))

#define atomic_list_for_each_entry(pos, head, member) \
    for (pos = atomic_list_first_entry(head, typeof(*pos), member); \
    &((pos)->member) != (head); \
    pos = atomic_list_next_entry(pos, member))

#define atomic_list_for_each_entry_reverse(pos, head, member) \
    for (pos = atomic_list_last_entry(head, typeof(*pos), member); \
    &((pos)->member) != (head); \
    pos = atomic_list_prev_entry(pos, member))

#define atomic_list_for_each_entry_safe(pos, n, head, member) \
    for (pos = atomic_list_first_entry(head, typeof(*pos), member), n = atomic_list_next_entry(pos, member); \
    &((pos)->member) != (head); \
    pos = n, n = atomic_list_next_entry(n, member))

#define atomic_list_for_each_entry_reverse_safe(pos, n, head, member) \
    for (pos = atomic_list_last_entry(head, typeof(*pos), member), n = atomic_list_prev_entry(pos, member); \
    &((pos)->member) != (head); \
    pos = n, n = atomic_list_prev_entry(n, member))

static inline void atomic_list_init(struct atomic_list_head *node)
{
    node->prev.store(node, std::memory_order_relaxed);
    node->next.store(node, std::memory_order_relaxed);
}

static inline void __atomic_list_del(struct atomic_list_head *prev, struct atomic_list_head *next)
{
    prev->next.store(next, std::memory_order_release);
    next->prev.store(prev, std::memory_order_release);
}

static inline void atomic_list_del(struct atomic_list_head *node)
{
    __atomic_list_del(node->prev.load(std::memory_order_acquire),
                      node->next.load(std::memory_order_acquire));
}

static inline void atomic_list_add(struct atomic_list_head *newNode, struct atomic_list_head *node)
{
    newNode->next.store(node->next.load(std::memory_order_acquire), std::memory_order_release);
    newNode->prev.store(node, std::memory_order_release);
    node->next.load(std::memory_order_acquire)->prev.store(newNode, std::memory_order_release);
    node->next.store(newNode, std::memory_order_release);
}

static inline void atomic_list_add_prev(struct atomic_list_head *newNode, struct atomic_list_head *node)
{
    newNode->next.store(node, std::memory_order_release);
    newNode->prev.store(node->prev.load(std::memory_order_acquire), std::memory_order_release);
    node->prev.load(std::memory_order_acquire)->next.store(newNode, std::memory_order_release);
    node->prev.store(newNode, std::memory_order_release);
}

static inline void atomic_list_add_head(struct atomic_list_head *node, struct atomic_list_head *head)
{
    atomic_list_add(node, head);
}

static inline void atomic_list_add_tail(struct atomic_list_head *node, struct atomic_list_head *head)
{
    atomic_list_add_prev(node, head);
}

#define ATOMIC_LIST_ENTRY(ptr, type, member)   ((char*)(ptr) - offsetof(type, member))
#define ATOMIC_GET_LIST(ptr, type, member)     ((char*)(ptr) + offsetof(type, member))

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* LIST_ATOMIC_H */
