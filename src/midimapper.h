/*
    MIDI mapper
*/

#ifndef _MIDI_MAPPER_H
#define _MIDI_MAPPER_H
#define USE_EXNG_TYPES
#include <stdio.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/midi.h>

// Types
#ifdef USE_EXNG_TYPES
typedef unsigned char      uint8;
typedef unsigned short     uint16;
typedef unsigned long      uint32;
typedef unsigned long long uint64;
typedef signed char        sint8;
typedef signed short       sint16;
typedef signed long        sint32;
typedef signed long long   sint64;
typedef float              float32;
typedef double             float64;
typedef enum {
    false = FALSE,
    true  = TRUE
} bool;
#endif

/* in remap.c */
bool   loadSetup(const char* configFile);
void   freeSetup(void);
sint32 remapMIDIData(uint8* dBuf, uint8* sBuf, sint32 len);

typedef struct Table_t Table;
typedef struct Channel_t Channel;
//typedef struct Directive_t Directive;

#define MIDI_TABLE_SIZE      128
#define MIDI_NUM_CONTROLLERS 128
#define MIDI_NUM_CHANNELS    16

struct Table_t {
    Table* next;
    uint32 idHash;
    uint8  data[MIDI_TABLE_SIZE];
};

struct Channel_t {
    uint8* programMap;                            /* remaps program changes */
    uint8* progBankMSBMap;                        /* per remapped program bank MSB*/
    uint8* progBankLSBMap;                        /* per remapped program bank LSB*/
    uint8* progTransMap;                          /* per remapped program transpose */
    uint8* noteMap[MIDI_TABLE_SIZE];              /* per remapped program note map (percussion parts) */
    uint8* velocityMap;                           /* remaps note on velocity */
    uint8* controlMap;                            /* remaps controller numbers */
    uint8* controlRangeMap[MIDI_NUM_CONTROLLERS]; /* remaps controller ranges */
    uint8* controlInit;                           /* initial controller values */
    uint8  output;                                /* output channel */
    sint8  currProgIn;                            /* current program (input) */
    sint8  currTrans;
    uint8  currBankLSB;
};

#define D_TABLE           0
#define D_CURVE           1
#define D_CHANNEL         2
#define D_PROG            3
#define D_PROG_BANKMSB    4
#define D_PROG_BANKLSB    5
#define D_PROG_TRANSPOSE  6
#define D_KEYMAP          7
#define D_VELOCITY        8
#define D_CONTROL         9
#define D_CONTROL_RANGE  10
#define D_CONTROL_INIT   11
#define D_END            12
#define D_NUM_DIRECTIVES 12

#endif
