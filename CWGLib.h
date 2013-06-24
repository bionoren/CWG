//
//  CWGLib.h
//  wwfmax
//
//  Created by Bion Oren on 11/5/12.
//  Copyright (c) 2012 Llama Software. All rights reserved.
//

#ifndef wwfmax_CWGLib_h
#define wwfmax_CWGLib_h

#include <stdbool.h>

#define WORD_LENGTH 15
#define NUMBER_OF_ENGLISH_LETTERS 26
#define INT_BITS 32
#define LOWER_IT 32

#define EXTENDED_LIST_FLAG 0X20000000
#define INTERNAL_LIST_FORMAT_INDEX_MASK 0X3FFE0000
#define LIST_FORMAT_INDEX_MASK 0X1FFE0000
#define LIST_FORMAT_BIT_SHIFT 17
#define CHILD_MASK 0X0001FFFF
#define EOW_FLAG 0X40000000

// Lookup tables used for node encoding and number-string decoding.
static const unsigned int PowersOfTwo[INT_BITS] = {1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384,
    32768, 65536, 131072, 262144, 524288, 1048576, 2097152, 4194304, 8388608, 16777216, 33554432, 67108864, 134217728,
    268435456, 536870912, 1073741824, 2147483648};
// Lookup tables used to extract child-offset values. This maps the value of an unsigned char to the number of bits set for that number
static const unsigned char PopCountTable[256] = { 0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4,
    3, 4, 4, 5, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 1, 2, 2, 3, 2, 3,
    3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4,
    4, 5, 4, 5, 5, 6, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 2, 3, 3, 4,
    3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6,
    6, 7, 4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8 };
static const int ChildListMasks[NUMBER_OF_ENGLISH_LETTERS] = { 0X1, 0X3, 0X7, 0XF, 0X1F, 0X3F, 0X7F, 0XFF, 0X1FF, 0X3FF, 0X7FF, 0XFFF,
    0X1FFF, 0X3FFF, 0X7FFF, 0XFFFF, 0X1FFFF, 0X3FFFF, 0X7FFFF, 0XFFFFF, 0X1FFFFF, 0X3FFFFF, 0X7FFFFF, 0XFFFFFF, 0X1FFFFFF, 0X3FFFFFF };

int ListFormatPopCount(int CompleteChildList, int LetterPosition);
bool moreThanOneBitSet(int i);

#endif
