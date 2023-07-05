#include <stddef.h>

#include "utils.h"

void
wsi_list_init(struct wsi_list *list)
{
    list->prev = list;
    list->next = list;
}

void
wsi_list_insert(struct wsi_list *list, struct wsi_list *elm)
{
    elm->prev = list;
    elm->next = list->next;
    list->next = elm;
    elm->next->prev = elm;
}

void
wsi_list_remove(struct wsi_list *elm)
{
    elm->prev->next = elm->next;
    elm->next->prev = elm->prev;
    elm->next = NULL;
    elm->prev = NULL;
}

int
wsi_list_length(const struct wsi_list *list)
{
    struct wsi_list *e;
    int count;

    count = 0;
    e = list->next;
    while (e != list) {
        e = e->next;
        count++;
    }

    return count;
}

int
wsi_list_empty(const struct wsi_list *list)
{
    return list->next == list;
}

void
wsi_list_insert_list(struct wsi_list *list, struct wsi_list *other)
{
    if (wsi_list_empty(other))
        return;

    other->next->prev = list;
    other->prev->next = list->next;
    list->next->prev = other->prev;
    list->next = other->next;
}
