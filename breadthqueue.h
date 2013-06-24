//
//  breadthqueue.h
//  wwfmax
//
//  Created by Bion Oren on 11/10/12.
//  Copyright (c) 2012 Llama Software. All rights reserved.
//

#ifndef wwfmax_breadthqueue_h
#define wwfmax_breadthqueue_h

#include "tnode.h"

// A queue is required for breadth first traversal, and the rest is self-evident.

struct breadthqueuenode {
    TnodePtr Element;
    struct breadthqueuenode *Next;
};

typedef struct breadthqueuenode BreadthQueueNode;
typedef BreadthQueueNode* BreadthQueueNodePtr;

struct breadthqueue {
    BreadthQueueNodePtr Front;
    BreadthQueueNodePtr Back;
    int Size;
};

typedef struct breadthqueue BreadthQueue;
typedef BreadthQueue* BreadthQueuePtr;

BreadthQueuePtr BreadthQueueInit(void);
void BreadthQueuePopulateReductionArray(BreadthQueuePtr ThisBreadthQueue, TnodePtr Root, TnodePtr **Holder);
int BreadthQueueUseToIndex(BreadthQueuePtr ThisBreadthQueue, TnodePtr Root);

#endif