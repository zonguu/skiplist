#ifndef LIST_TEMPLATE_ATOMIC_H
#define LIST_TEMPLATE_ATOMIC_H

#include <stddef.h>

template<typename T, typename M>
static inline size_t atomic_template_offsetof(const M T::* member)
{
    return reinterpret_cast<size_t>(&(reinterpret_cast<T*>(0)->*member));
}

template<typename T, typename M, size_t I, size_t H>
static inline size_t atomic_template_offsetof_array(const M (T::* member)[H])
{
    return reinterpret_cast<size_t>(&(reinterpret_cast<T*>(0)->*member)[I]);
}

static inline bool atomic_template_list_is_empty(atomic_list_head* ptr)
{
    return (ptr)->prev.load(std::memory_order_acquire) == (ptr)->next.load(std::memory_order_acquire);
}

template<typename T, typename M>
static inline T* atomic_template_list_entry(atomic_list_head* ptr, const M T::* member)
{
    return reinterpret_cast<T*>(reinterpret_cast<char*>(ptr) - atomic_template_offsetof(member));
}

template<typename T, typename M, size_t I>
static inline T* atomic_template_list_entry1(atomic_list_head* ptr, const M T::* member)
{
    return reinterpret_cast<T*>(reinterpret_cast<char*>(ptr) - offsetof(T, member[I]));
}

template <typename T, typename M, size_t I, size_t H>
static inline T *atomic_template_list_entry_array(atomic_list_head *ptr, const M (T::*member)[H], size_t index)
{
    return reinterpret_cast<T*>(reinterpret_cast<char*>(ptr) - atomic_template_offsetof_array<T, M, I, H>(member));
}

template<typename T, typename M>
T* atomic_template_list_first_entry(atomic_list_head* head, const M T::* member)
{
    return atomic_template_list_entry((head)->next.load(std::memory_order_acquire), member);
}

template<typename T, typename M>
T* atomic_template_list_last_entry(atomic_list_head* head, const M T::* member)
{
    return atomic_template_list_entry((head)->prev.load(std::memory_order_acquire), member);
}

template<typename T, typename M>
static inline T* atomic_template_list_next_entry(atomic_list_head* ptr, const M T::* member)
{
    return atomic_template_list_entry((ptr)->next.load(std::memory_order_acquire), member);
}

template<typename T, typename M>
static inline T* atomic_template_list_prev_entry(atomic_list_head* ptr, const M T::* member)
{
    return atomic_template_list_entry((ptr)->prev.load(std::memory_order_acquire), member);
}

template<typename T, typename M, typename Func>
void atomic_template_list_for_each_safe(const atomic_list_head *head, M T::*member, Func &&func)
{
    const size_t offset = atomic_template_offsetof(member);
    for (atomic_list_head *pos = head->next.load(std::memory_order_acquire),
                         *n = pos->next.load(std::memory_order_acquire);
         pos != head;
         pos = n, n = n->next.load(std::memory_order_acquire))
    {
        std::forward<Func>(func)(pos, n);
    }
}

template<typename T, typename M, typename Func>
void atomic_template_list_for_each_safe(atomic_list_head *pos, atomic_list_head *n, const atomic_list_head *head, M T::*member, Func &&func)
{
    const size_t offset = atomic_template_offsetof(member);
    for (pos = head->next.load(std::memory_order_acquire),
         n = pos->next.load(std::memory_order_acquire);
         pos != head;
         pos = n, n = n->next.load(std::memory_order_acquire))
    {
        std::forward<Func>(func)(pos);
    }
}

template<typename T, typename M, typename Func>
void atomic_template_list_for_each_entry_break(T **entry, const atomic_list_head *head, const M T::* member, Func &&func)
{
    const size_t offset = atomic_template_offsetof(member);
    for (atomic_list_head *pos = head->next.load(std::memory_order_acquire);
         pos != head;
         pos = pos->next.load(std::memory_order_acquire))
    {
        *entry = reinterpret_cast<T*>(reinterpret_cast<char*>(pos) - offset);
        if (std::forward<Func>(func)(*entry))
        {
            break;
        }
    }
}

template<typename T, typename M, typename Func>
atomic_list_head* atomic_template_list_for_each_break(const atomic_list_head *head, const M T::* member, Func &&func)
{
    atomic_list_head *pos;
    const size_t offset = atomic_template_offsetof(member);
    for (pos = head->next.load(std::memory_order_acquire);
         pos != head;
         pos = pos->next.load(std::memory_order_acquire))
    {
        T* entry = reinterpret_cast<T*>(reinterpret_cast<char*>(pos) - offset);
        if (std::forward<Func>(func)(entry))
        {
            break;
        }
    }
    return pos;
}

template<typename T, typename M, size_t I, size_t H, typename Func>
void atomic_template_list_for_each_break_array(atomic_list_head **pos, const atomic_list_head *head, const M T::* member, Func &&func)
{
    const size_t offset = atomic_template_offsetof_array(member);
    for (*pos = head->next.load(std::memory_order_acquire);
         *pos != head;
         *pos = (*pos)->next.load(std::memory_order_acquire))
    {
        T* entry = reinterpret_cast<T*>(reinterpret_cast<char*>(*pos) - offset);
        if (std::forward<Func>(func)(entry))
        {
            break;
        }
    }
}

template<typename T, typename M, typename Func>
void atomic_template_list_for_each_break(atomic_list_head **pos, const atomic_list_head *head, const M T::* member, Func &&func)
{
    const size_t offset = atomic_template_offsetof(member);
    for (*pos = head->next.load(std::memory_order_acquire);
         *pos != head;
         *pos = (*pos)->next.load(std::memory_order_acquire))
    {
        T* entry = reinterpret_cast<T*>(reinterpret_cast<char*>(*pos) - offset);
        if (std::forward<Func>(func)(entry))
        {
            break;
        }
    }
}

template<typename T, typename M, typename Func>
void atomic_template_list_for_each_entry(atomic_list_head *head, const M T::* member, Func &&func)
{
    const size_t offset = atomic_template_offsetof(member);
    for (atomic_list_head *pos = head->next.load(std::memory_order_acquire);
         pos != head;
         pos = pos->next.load(std::memory_order_acquire))
    {
        T *entry = reinterpret_cast<T*>(reinterpret_cast<char*>(pos) - offset);
        std::forward<Func>(func)(entry);
    }
}

template<typename T, typename M, size_t I, size_t H, typename Func>
void atomic_template_list_for_each_entry_array(atomic_list_head *head, const M (T::* member)[H], Func &&func)
{
    const size_t offset = atomic_template_offsetof_array<T, M, I, H>(member);
    for (atomic_list_head *pos = head->next.load(std::memory_order_acquire);
         pos != head;
         pos = pos->next.load(std::memory_order_acquire))
    {
        T *entry = reinterpret_cast<T*>(reinterpret_cast<char*>(pos) - offset);
        std::forward<Func>(func)(entry);
    }
}

template<typename T, typename M, typename Func>
void atomic_template_list_for_each_entry_safe(atomic_list_head *head, const M T::* member, Func &&func)
{
    const size_t offset = atomic_template_offsetof(member);
    for (atomic_list_head *pos = head->next.load(std::memory_order_acquire),
                         *n = pos->next.load(std::memory_order_acquire);
         pos != head;
         pos = n, n = n->next.load(std::memory_order_acquire))
    {
        T *entry = reinterpret_cast<T*>(reinterpret_cast<char*>(pos) - offset);
        std::forward<Func>(func)(entry);
    }
}

template<typename T, typename M, size_t I, size_t H, typename Func>
void atomic_template_list_for_each_entry_array_safe(atomic_list_head *head, const M (T::* member)[H], Func &&func)
{
    const size_t offset = atomic_template_offsetof_array<T, M, I, H>(member);
    for (atomic_list_head *pos = head->next.load(std::memory_order_acquire),
                         *n = pos->next.load(std::memory_order_acquire);
         pos != head;
         pos = n, n = n->next.load(std::memory_order_acquire))
    {
        T *entry = reinterpret_cast<T*>(reinterpret_cast<char*>(pos) - offset);
        std::forward<Func>(func)(entry);
    }
}

template<typename T, typename M, typename Func>
void atomic_template_list_for_each_entry_reverse(atomic_list_head *head, M T::*member, Func &&func)
{
    const size_t offset = atomic_template_offsetof(member);
    for (atomic_list_head *pos = head->prev.load(std::memory_order_acquire);
         pos != head;
         pos = pos->prev.load(std::memory_order_acquire))
    {
        T *entry = reinterpret_cast<T*>(reinterpret_cast<char*>(pos) - offset);
        func(entry);
    }
}

#endif /* LIST_TEMPLATE_ATOMIC_H */
