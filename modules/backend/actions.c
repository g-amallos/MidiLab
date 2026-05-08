#include <backend.h>
#include <stdlib.h>


struct action_node {
    OnClickFunc func;
    struct action_node* next;
};

typedef struct actions {
    int size;
    struct action_node* actions;
    struct action_node* last;
} *Actions;


struct actions _frameActions = {0, NULL, NULL};
Actions actions = &_frameActions;


void _addAction(OnClickFunc func) {
    if (!func) return;
    struct action_node* node = malloc(sizeof(struct action_node));

    if (!node) return;
    node->func = func;
    node->next = NULL;

    if (actions->last) actions->last->next = node;
    else actions->actions = node;

    actions->last = node;
    (actions->size)++;
}

void actionDefer(OnClickFunc func) {
    _addAction(func);
}


int actionIsQueueEmpty() {
    return actions->actions==NULL;
}

void actionExecuteAndRemoveFirst() {
    if (!(actions->actions)) return;

    OnClickFunc func = actions->actions->func;
    struct action_node* toFree = actions->actions;
    actions->actions = actions->actions->next;
    if (!(actions->actions)) actions->last = NULL;
    (actions->size)--;
    free(toFree);

    func();
}

void actionExecuteAllDeferred() {
    while (actions->actions) actionExecuteAndRemoveFirst();
}