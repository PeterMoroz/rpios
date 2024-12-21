#ifndef __LIST_H__
#define __LIST_H_

#define DEFINE_LIST(node_type) \
	struct node_type *head; \
	struct node_type *tail; \
	uint32_t size; \
} node_type##_list_t;

#define DEFINE_LINK(node_type) \
	struct node_type *next##node_type; \
	struct node_type *prev##node_type;

#define INITIALIZE_LIST(list) \
	list.head = list.tail = (void *)0; \
	list.size = 0;

#define IMPLEMENT_LIST(node_type) \
void append_##node_type##_list(node_type##_list_t *list, struct node_type *node) { \
	list->tail->next##node_type = node; \
	node->prev##node_type = list->tail; \
	list->tail = node; \
	node->next##node_type = (void *)0; \
	list->size += 1; \
} \
\
void push_##node_type##_list(node_type##list_t *list, struct node_type *node) { \
	node->next##node_type = list->head; \
	node->prev##node_type = (void *)0; \
	list->head = node; \
	list->size += 1; \
} \
\
struct node_type* peek_##node_type##_list(node_type##_list_t *list) { \
	return list->head; \
} \
struct node_type* pop_##node_type##_list(node_type##_list_t *list) { \
	struct node_type *res = list->head; \
	list->head = list->head->next##node_type; \
	list->head->prev##node_type = (void *)0; \
	list->size -= 1; \
	return res; \
} \
\
uint32_t size_##node_type##_list(node_type##_list_t *list) { \
	return list->size; \
} \
\
struct node_type* next_##node_type##_list(struct node_type *node) { \
	return node->next##node_type; \
} \

#endif
