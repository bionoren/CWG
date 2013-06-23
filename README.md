Original source and documentation - See http://www.pathcom.com/~vadco/cwg.html. I've made some modifications though - see below.

## Summary
This is a deterministic acyclic finite state automaton, also known as prefix and suffix compression on English words. And this implementation is both *really small* and *really fast*.

## Modifications
* The creator has been split up into seperate files for each of the structs.
* The ListFormat array has been modified. Bits 30 - 25 now (optionally) store the number for a letter (1 - 26) that can optionally be part of the format. This letter is used IFF the highest bit of the consuming node's list format index is 1, otherwise it is ignored.
* Header files for easier incorporation into other projects.
* Modifications / additions to debug/log output.
* Modifications to the Justin-CWG-Search sample code to use the modified data structures, including a debug method to validate the datastructure.
* Reformated and updated to use things like stdbool, C99 style for loops, etc, and to compile on OS X.
* Uses lowercase dictionary words

## Notes
The first commit is JohnPaul Adamovsky's original files, and the rest is my fork. He doesn't appear to be on github, and his files don't appear to be licensed in any capacity... but I'll happily transfer the repo to him if he wants it.

This assumes that you have at least one word starting with each of the letters of the alphabet. If you don't, it will crash.
