#ifndef WSI_SRC_COCOA_UTILS_H
#define WSI_SRC_COCOA_UTILS_H

#include <stddef.h>

struct wsi_list {
    struct wsi_list *prev;
    struct wsi_list *next;
};

void
wsi_list_init(struct wsi_list *list);

void
wsi_list_insert(struct wsi_list *list, struct wsi_list *elm);

void
wsi_list_remove(struct wsi_list *elm);

int
wsi_list_length(const struct wsi_list *list);

int
wsi_list_empty(const struct wsi_list *list);

void
wsi_list_insert_list(struct wsi_list *list, struct wsi_list *other);

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
