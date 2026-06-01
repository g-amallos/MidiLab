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





void midiActionAdd(MidiEvent event, double time) {
    if (!event) return;
    struct midi_action_node* node = malloc(sizeof(struct midi_action_node));

    if (!node) return;
    node->event = event;
    node->time = time;
    node->next = NULL;

    if (midiActions->last) midiActions->last->next = node;
    else midiActions->events = node;

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


void midiActionExecuteFrame() {
    double time = GetTime();

    int size=midiActions->size;
    struct midi_action_node* prev = midiActions->events;
    for (int i=0; i<size && prev; i++) {
        if (prev->next) {
            struct midi_action_node* cur = prev->next;
            if (cur->time<=time) _midiActionExecuteNext(prev);
        }
        prev = prev->next;
    }
}