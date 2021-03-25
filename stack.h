#pragma once

// Data structure to represent a stack
struct stack {
    int max_size;
    int top;     
    unsigned short *items; // Stack items are 16 bit
};


// Function declarations
struct stack* new_stack(int capacity);
void delete_stack(struct stack *pt);
int size(struct stack *pt);
int is_empty(struct stack *pt);
int is_full(struct stack *pt);
void push(struct stack *pt, unsigned short x);
unsigned short pop(struct stack *pt);
