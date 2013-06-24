//
//  CWG-Creator.h
//  wwfmax
//
//  Created by Bion Oren on 11/10/12.
//  Copyright (c) 2012 Llama Software. All rights reserved.
//

#ifndef wwfmax_CWG_Creator_h
#define wwfmax_CWG_Creator_h

typedef struct {
    char *words;
    int numWords;
    int *lengths;
} WordInfo;

int createDataStructure(const WordInfo *info);

#endif