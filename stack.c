#include <stdio.h>
#include <stdlib.h>

#include "stack.h"

// Initialize a stack
struct stack* new_stack(int capacity) {
    struct stack *pt = (struct stack*)malloc(sizeof(struct stack));

    pt->max_size = capacity;
    pt->top = -1;
    pt->items = (unsigned short*)malloc(sizeof(unsigned short) * capacity);

    return pt;
}

// Delete a stack
void delete_stack(struct stack *pt) {
    free(pt->items);
    free(pt);
}

// Return size of stack
int size(struct stack *pt) {
    return pt->top + 1;
}

// Check if stack is empty or not
int is_empty(struct stack *pt) {
    return pt->top == -1;
}

// Check if stack is full or not
int is_full(struct stack *pt) {
    return pt->top == pt->max_size - 1;
}

// Push an element 'x' to top of stack
void push(struct stack *pt, unsigned short x) {
    // Check if the stack is already full
    if (is_full(pt)) {
        printf("STACK OVERFLOW\nProgram Terminated\n");
        exit(EXIT_FAILURE);
    }

    // Add an element and increment top's index
    pt->items[++pt->top] = x;
}

// Pop an element from the stack
unsigned short pop(struct stack *pt) {
    // Check for stack underflow
    if (is_empty(pt)) {
        printf("STACK UNDERFLOW\nProgram Terminated\n");
        exit(EXIT_FAILURE);
    }

    // Decrement stack size by 1 and return popped element
    return pt->items[pt->top--];
}

