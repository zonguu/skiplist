#ifndef LIST_TEMPLATE_H
#define LIST_TEMPLATE_H

#include <stddef.h>

void a
{
    offsetof
}

template<typename T, typename M>
static inline size_t template_offsetof(const M T::* member)
{
    return reinterpret_cast<size_t>(&(reinterpret_cast<T*>(0)->*member));
}

template<typename T, typename M, size_t I, size_t H>
static inline size_t template_offsetof_array(const M (T::* member)[H])
{
    return reinterpret_cast<size_t>(&(reinterpret_cast<T*>(0)->*member)[I]);
}

static inline bool template_list_is_empty(list_head* ptr)
{
    return (ptr)->prev == (ptr)->next;
}

template<typename T, typename M>
static inline T* template_list_entry(list_head* ptr, const M T::* member)
{
    return reinterpret_cast<T*>(reinterpret_cast<char*>(ptr) - template_offsetof(member));
}

template<typename T, typename M, size_t I>
static inline T* template_list_entry1(list_head* ptr, const M T::* member)
{
    return reinterpret_cast<T*>(reinterpret_cast<char*>(ptr) - offsetof(T, member[I]));
    // return reinterpret_cast<T*>(reinterpret_cast<char*>(ptr) - template_offsetof(member));
}

template <typename T, typename M, size_t I, size_t H>
static inline T *template_list_entry_array(list_head *ptr, const M (T::*member)[H], size_t index)
{
    return reinterpret_cast<T*>(reinterpret_cast<char*>(ptr) - template_offsetof_array<T, M, I, H>(member));
}

template<typename T, typename M>
T* template_list_first_entry(list_head* head, const M T::* member)
{
    return template_list_entry((head)->next, member);
}

template<typename T, typename M>
T* template_list_last_entry(list_head* head, const M T::* member)
{
    return template_list_entry((head)->prev, member);
}

template<typename T, typename M>
static inline T* template_list_next_entry(list_head* ptr, const M T::* member)
{
    return template_list_entry((ptr)->next, member);
}

template<typename T, typename M>
static inline T* template_list_prev_entry(list_head* ptr, const M T::* member)
{
    return template_list_entry((ptr)->prev, member);
}

template<typename T, typename M, typename Func>
void template_list_for_each_safe(const list_head *head, M T::*member, Func &&func)
{
    const size_t offset = template_offsetof(member);
    for (list_head *pos = head->next, *n = pos->next; pos != head; pos = n, n = n->next)
    {
        std::forward<Func>(func)(pos, n);
    }
}

template<typename T, typename M, typename Func>
void template_list_for_each_safe(list_head *pos, list_head *n, const list_head *head, M T::*member, Func &&func)
{
    const size_t offset = template_offsetof(member);
    for (pos = head->next, n = pos->next; pos != head; pos = n, n = n->next)
    {
        std::forward<Func>(func)(pos);
    }
}

template<typename T, typename M, typename Func>
void template_list_for_each_entry_break(T **entry, const list_head *head, const M T::* member, Func &&func)
{
    const size_t offset = template_offsetof(member);
    for (list_head *pos = head->next; pos != head; pos = pos->next)
    {
        *entry = reinterpret_cast<T*>(reinterpret_cast<char*>(pos) - offset);
        if (std::forward<Func>(func)(*entry))
        {
            break;
        }
    }
}

template<typename T, typename M, typename Func>
list_head* template_list_for_each_break(const list_head *head, const M T::* member, Func &&func)
{
    list_head *pos;
    const size_t offset = template_offsetof(member);
    for (pos = head->next; pos != head; pos = pos->next)
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
void template_list_for_each_break_array(list_head **pos, const list_head *head, const M T::* member, Func &&func)
{
    const size_t offset = template_offsetof_array(member);
    for (*pos = head->next; *pos != head; *pos = (*pos)->next)
    {
        T* entry = reinterpret_cast<T*>(reinterpret_cast<char*>(*pos) - offset);
        if (std::forward<Func>(func)(entry))
        {
            break;
        }
    }
}

template<typename T, typename M, typename Func>
void template_list_for_each_break(list_head **pos, const list_head *head, const M T::* member, Func &&func)
{
    const size_t offset = template_offsetof(member);
    for (*pos = head->next; *pos != head; *pos = (*pos)->next)
    {
        T* entry = reinterpret_cast<T*>(reinterpret_cast<char*>(*pos) - offset);
        if (std::forward<Func>(func)(entry))
        {
            break;
        }
    }
}

template<typename T, typename M, typename Func>
void template_list_for_each_entry(list_head *head, const M T::* member, Func &&func)
{
    const size_t offset = template_offsetof(member);
    for (list_head *pos = head->next; pos != head; pos = pos->next)
    {
        T *entry = reinterpret_cast<T*>(reinterpret_cast<char*>(pos) - offset);
        std::forward<Func>(func)(entry);
    }
}

template<typename T, typename M, size_t I, size_t H, typename Func>
void template_list_for_each_entry_array(list_head *head, const M (T::* member)[H], Func &&func)
{
    const size_t offset = template_offsetof_array<T, M, I, H>(member);
    for (list_head *pos = head->next; pos != head; pos = pos->next)
    {
        T *entry = reinterpret_cast<T*>(reinterpret_cast<char*>(pos) - offset);
        // T *entry = template_list_entry1(pos, member);
        std::forward<Func>(func)(entry);
    }
}

template<typename T, typename M, typename Func>
void template_list_for_each_entry_safe(list_head *head, const M T::* member, Func &&func)
{
    const size_t offset = template_offsetof(member);
    for (list_head *pos = head->next, *n = pos->next; pos != head; pos = n, n = n->next)
    {
        T *entry = reinterpret_cast<T*>(reinterpret_cast<char*>(pos) - offset);
        std::forward<Func>(func)(entry);
    }
}

template<typename T, typename M, size_t I, size_t H, typename Func>
void template_list_for_each_entry_array_safe(list_head *head, const M (T::* member)[H], Func &&func)
{
    const size_t offset = template_offsetof_array<T, M, I, H>(member);
    for (list_head *pos = head->next, *n = pos->next; pos != head; pos = n, n = n->next)
    {
        T *entry = reinterpret_cast<T*>(reinterpret_cast<char*>(pos) - offset);
        // T *entry = template_list_entry1(pos, member);
        std::forward<Func>(func)(entry);
    }
}

template<typename T, typename M, typename Func>
void template_list_for_each_entry_reverse(list_head *head, M T::*member, Func &&func)
{
    const size_t offset = template_offsetof(member);
    for (list_head *pos = head->prev; pos != head; pos = pos->prev)
    {
        T *entry = reinterpret_cast<T*>(reinterpret_cast<char*>(pos) - offset);
        func(entry);
    }
}

#endif /* LIST_TEMPLATE_H */
