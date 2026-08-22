#include <backend.h>
#include <stdlib.h>
#include <synth.h>


struct action_node {
    OnClickFunc func;
    struct action_node* next;
};

typedef struct actions {
    int size;
    struct action_node* actions;
    struct action_node* last;
} *Actions;


typedef struct midi_event* MidiEvent;

struct midi_action_node {
    MidiEvent event;
    double time;
    struct midi_action_node* next;
    struct midi_action_node* previous;
};

typedef struct midi_actions {
    int size;
    struct midi_action_node* events;
    struct midi_action_node* last;
} *MidiActions;


struct actions _frameActions = {0, NULL, NULL};
Actions actions = &_frameActions;

struct midi_actions _midiActions = {0, NULL, NULL};
MidiActions midiActions = &_midiActions;


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

void actionClose() {
    struct action_node* toFree;
    struct action_node* node = actions->actions;
    while (node) {
        toFree = node;
        node = node->next;
        free(toFree);
    }

    actions->actions = NULL;
    actions->last = NULL;
    actions->size = 0;
}





void midiActionAdd(MidiEvent event, double time) {
    if (!event) return;
    struct midi_action_node* node = malloc(sizeof(struct midi_action_node));

    if (!node) return;
    node->event = event;
    node->time = time;
    node->next = NULL;
    node->previous = midiActions->last;
    if (midiActions->last) midiActions->last->next = node;
    if (!(midiActions->events)) midiActions->events = node;

    midiActions->last = node;
    (midiActions->size)++;
}


void _midiActionExecuteNext(struct midi_action_node* prev) {
    if (!prev || !(prev->next)) return;

    struct midi_action_node* cur = prev->next;
    prev->next = cur->next;

    // Execute cur->event
    synthExecuteEvent(cur->event);
    midiEventFree(cur->event);
    free(cur);
}


void _midiActionRemoveNode(struct midi_action_node* node) {
    if (!node) return;

    if (node->next) node->next->previous = node->previous;
    if (node->previous) node->previous->next = node->next;

    if (midiActions->events == node) midiActions->events = node->next;
    if (midiActions->last == node) midiActions->last = node->previous;

    (midiActions->size)--;

    midiEventFree(node->event);
    free(node);
}

void _midiActionExecute(struct midi_action_node* node) {
    if (!node) return;

    synthExecuteEvent(node->event);
    _midiActionRemoveNode(node);
}


void midiActionExecuteFrame() {
    double time = GetTime();
    
    struct midi_action_node* node=midiActions->events, *tmp;
    while (node) {
        if (node->time<=time) {
            tmp = node;
            node = node->next;
            _midiActionExecute(tmp);
        } else node = node->next;
    }
}


void midiActionRemoveAll() {    // Doesn't execute anything, only deletes all registered events
    struct midi_action_node* toFree;
    struct midi_action_node* node = midiActions->events;
    while (node) {
        toFree = node;
        node = node->next;
        midiEventFree(toFree->event);
        free(toFree);
    }
    midiActions->events = NULL;
    midiActions->size = 0;
    midiActions->last = NULL;
}

void midiActionClose() {
    midiActionRemoveAll();
}