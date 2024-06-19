#ifndef DEFINES_H
#define DEFINES_H

#ifndef EMULATOR_BUILD
#define EMULATOR_BUILD "undefined"
#endif

#define GEARGRAFX_TITLE "Geargrafx"
#define GEARGRAFX_VERSION EMULATOR_BUILD
static const char * GEARGRAFX_TITLE_ASCII = 
"   ____                                 __      \n"
"  / ___| ___  __ _ _ __ __ _ _ __ __ _ / _|_  __\n"
" | |  _ / _ \\/ _` | '__/ _` | '__/ _` | |_\\ \\/ /\n"
" | |_| |  __/ (_| | | | (_| | | | (_| |  _|>  < \n"
"  \\____|\\___|\\__,_|_|  \\__, |_|  \\__,_|_| /_/\\_\\\n"
"                       |___/                    \n";

#ifdef DEBUG
#define GEARGRAFX_DEBUG 1
#endif

#ifndef NULL
#define NULL 0
#endif

#define SafeDelete(pointer) if(pointer != NULL) {delete pointer; pointer = NULL;}
#define SafeDeleteArray(pointer) if(pointer != NULL) {delete [] pointer; pointer = NULL;}

#define InitPointer(pointer) ((pointer) = NULL)
#define IsValidPointer(pointer) ((pointer) != NULL)

#if defined(MSB_FIRST) || defined(__BIG_ENDIAN__) || (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#define IS_BIG_ENDIAN
#else
#define IS_LITTLE_ENDIAN
#endif

#endif /* DEFINES_H */