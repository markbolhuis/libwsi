#ifndef WSI_SRC_XCB_UTILS_H
#define WSI_SRC_XCB_UTILS_H

#include <stddef.h>

struct wsi_list {
    struct wsi_list *prev;
    struct wsi_list *next;
};

static inline void
wsi_list_init(struct wsi_list *list)
{
    list->prev = list;
    list->next = list;
}

static inline void
wsi_list_insert(struct wsi_list *list, struct wsi_list *elm)
{
    elm->prev = list;
    elm->next = list->next;
    list->next = elm;
    elm->next->prev = elm;
}

static inline void
wsi_list_remove(struct wsi_list *elm)
{
    elm->prev->next = elm->next;
    elm->next->prev = elm->prev;
    elm->next = NULL;
    elm->prev = NULL;
}

static inline int
wsi_list_length(const struct wsi_list *list)
{
    int count = 0;
    struct wsi_list *e = list->next;
    while (e != list) {
        e = e->next;
        count++;
    }
    return count;
}

static inline int
wsi_list_empty(const struct wsi_list *list)
{
    return list->next == list;
}

static inline void
wsi_list_insert_list(struct wsi_list *list, struct wsi_list *other)
{
    if (wsi_list_empty(other)) {
        return;
    }

    other->next->prev = list;
    other->prev->next = list->next;
    list->next->prev = other->prev;
    list->next = other->next;
}

#define wsi_container_of(ptr, sample, member) \
	(__typeof__(sample))((char *)(ptr) - \
			     offsetof(__typeof__(*sample), member))

#define wsi_list_for_each(pos, head, member)	\
	for (pos = wsi_container_of((head)->next, pos, member);\
	     &pos->member != (head); \
	     pos = wsi_container_of(pos->member.next, pos, member))

#define wsi_list_for_each_safe(pos, tmp, head, member) \
	for (pos = wsi_container_of((head)->next, pos, member), \
	     tmp = wsi_container_of((pos)->member.next, tmp, member); \
	     &pos->member != (head); \
	     pos = tmp, \
	     tmp = wsi_container_of(pos->member.next, tmp, member))

#define wsi_list_for_each_reverse(pos, head, member) \
	for (pos = wsi_container_of((head)->prev, pos, member); \
	     &pos->member != (head); \
	     pos = wsi_container_of(pos->member.prev, pos, member))

#define wsi_list_for_each_reverse_safe(pos, tmp, head, member) \
	for (pos = wsi_container_of((head)->prev, pos, member), \
	     tmp = wsi_container_of((pos)->member.prev, tmp, member); \
	     &pos->member != (head); \
	     pos = tmp, \
	     tmp = wsi_container_of(pos->member.prev, tmp, member))

#endif
