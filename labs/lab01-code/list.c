#include "list.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void list_init(list_t *list) {
    list->head = NULL;
    list->size = 0;
}

void list_add(list_t *list, const char *data) {
    node_t *new_node = (node_t *) malloc(sizeof(node_t));

    // Check if str fits in bounds (str ends in null terminator)
    strncpy(new_node->data, data, MAX_LEN - 1);
    new_node->data[MAX_LEN - 1] = '\0';
    new_node->next = NULL;

    // Basic null check for empty list, if empty new node will be head of list
    if (list->head == NULL) {
        list->head = new_node;
    } else {
        node_t *curr = list->head;
        while (curr->next != NULL) {
            curr = curr->next;
        }
        curr->next = new_node;
    }
    list->size++;
}

int list_size(const list_t *list) {
    int size = 0;
    node_t *curr = list->head;
    while (curr != NULL) {
        size++;
        curr = curr->next;
    }
    return size;
}

char *list_get(const list_t *list, int index) {
    node_t *curr = list->head;
    int i = 0;
    while (curr != NULL) {
        if (i == index) {
            return curr->data;
        }
        i++;
        curr = curr->next;
    }
    return NULL;
}

void list_clear(list_t *list) {
    node_t *temp_node;
    node_t *curr = list->head;
    while (curr != NULL) {
        temp_node = curr;
        curr = curr->next;
        free(temp_node);
    }
    list->head = NULL;
    list->size = 0;
}

int list_contains(const list_t *list, const char *query) {
    node_t *curr = list->head;
    while (curr != NULL) {
        if (strcmp(curr->data, query) == 0) {
            return 1;
        } else {
            curr = curr->next;
        }
    }
    return 0;
}

void list_print(const list_t *list) {
    int i = 0;
    node_t *current = list->head;
    while (current != NULL) {
        printf("%d: %s\n", i, current->data);
        current = current->next;
        i++;
    }
}
