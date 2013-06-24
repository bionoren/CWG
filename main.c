//
//  main.m
//  wwfmax
//
//  Created by Bion Oren on 10/1/12.
//  Copyright (c) 2012 Llama Software. All rights reserved.
//

#define BUILD_DATASTRUCTURES 1

#import <stdlib.h>
#import <stdio.h>
#import <string.h>
#import "CWG-Creator.h"
#import "Justin-CWG-Search.h"
#import "DictionaryManager.h"
#import "assert.h"

int main(int argc, const char * argv[]) {
#if BUILD_DATASTRUCTURES
    {
        int numWords = 173101;
        char *words = calloc(numWords * WORD_LENGTH, sizeof(char));
        assert(words);
        int *wordLengths = malloc(numWords * sizeof(int));
        assert(wordLengths);
        
        FILE *wordFile = fopen("dict.txt", "r");
        assert(wordFile);
        char buffer[40];
        int i = 0;
        char *word = words;
        while(fgets(buffer, 40, wordFile)) {
            int len = (int)strlen(buffer);
            if(buffer[len - 1] == '\n') {
                --len;
            }
            if(len <= WORD_LENGTH) {
                strncpy(word, buffer, len);
                wordLengths[i++] = len;
                assert(i < numWords);
                word += WORD_LENGTH * sizeof(char);
            }
        }
        numWords = i;
        
        printf("evaluating %d words\n", numWords);
        
        const WordInfo info = {.words = words, .numWords = numWords, .lengths = wordLengths};
        createDataStructure(&info);
        free(words);
        free(wordLengths);
    }
#endif
#ifdef DEBUG
    debug();
#endif
    
    DictionaryManager *mgr = createDictManager();

    char word[WORD_LENGTH];
    int length;
    while((length = nextWord(mgr, word))) {
        printf("%.*s\n", length, word);
    }
    resetManager(mgr);
    while((length = nextWord(mgr, word)));

    freeDictManager(mgr);
    destructDictManager();
    return 0;
}