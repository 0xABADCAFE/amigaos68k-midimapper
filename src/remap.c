/*
    Midi table remapping
*/

#include "midimapper.h"
#include <math.h>

Table*   tableList  = 0;
Channel* channels   = 0;
uint32*  directives = 0;

#define FILE_PARSE_BUFFER 256

/* Parser Directives */
void parseTable(FILE*, char*, sint32);
void parseCurve(FILE*, char*, sint32);
void parseChannel(FILE*, char*, sint32);
void parseProgram(FILE*, char*, sint32);
void parseProgBankMSB(FILE*, char*, sint32);
void parseProgBankLSB(FILE*, char*, sint32);
void parseProgTranspose(FILE*, char*, sint32);
void parseNotemap(FILE*, char*, sint32);
void parseVelocity(FILE*, char*, sint32);
void parseController(FILE*, char*, sint32);
void parseCtrlRange(FILE*, char*, sint32);
void parseCtrlInit(FILE*, char*, sint32);

typedef void (*ParseFunc)(FILE*, char*, sint32);
typedef struct {
    const char* name;
    uint32      id;
    ParseFunc   parse;
} Directive;

static Directive dirs[] = {
    /* file directives */
    { "end",             0, 0 },
    { "table:",          0, &parseTable },
    { "curve:",          0, &parseCurve },
    { "channel:",        0, &parseChannel },
    { "program:",        0, &parseProgram },
    { "progbankmsb:",    0, &parseProgBankMSB },
    { "progbanklsb:",    0, &parseProgBankLSB },
    { "progtranspose:",  0, &parseProgTranspose },
    { "keymap:",         0, &parseNotemap },
    { "velocity:",       0, &parseVelocity },
    { "control:",        0, &parseController },
    { "ctrlrange:",      0, &parseCtrlRange },
    { "ctrlinit:",       0, &parseCtrlInit },
/*
    { "modulation:",     0, 0 },
    { "breath:",         0, 0 },
    { "foot:",           0, 0 },
    { "hold:",           0, 0 },
    { "softpedal:",      0, 0 },
    { "sostenuto:",      0, 0 },
    { "attacktime:",     0, 0 },
    { "releasetime:",    0, 0 },

    { "portamento:",     0, 0 },
    { "portamentoctrl:", 0, 0 },
    { "portamentotime:", 0, 0 },

    { "dataentrylsb:",   0, 0 },
    { "volume:",         0, 0 },
    { "expression:",     0, 0 },
    { "dataentrymsb:",   0, 0 },

    { "pan:",            0, 0 },
    { "resonance:",      0, 0 },
    { "brightness:",     0, 0 },
*/
    { 0,                 0, 0}
};


/**************************************************************************/

uint32 hashString(const char* string) {
    /* generates an id hash */
    uint32 hash = 0;
    uint32 mask = 0x80000000;
    uint8* p    = (uint8*)string;
    while (*p) {
        hash = (hash << 1) | ((hash & mask) ? 1 : 0);
        hash ^= (uint32) *p++;
    }
    return hash;
}

void initDirectives(void) {
    sint32 i = 0;
    while (dirs[i].name) {
        dirs[i].id = hashString(dirs[i].name);
        //printf("%s : 0x%08X\n", dirs[i].name, (unsigned)(dirs[i].id));
        i++;
    }
}

bool handleDirective(uint32 id, FILE* file, char* buffer, sint32 in) {
    sint32 i = 0;
    while (dirs[i].name) {
        if (dirs[i].id == id) {
            //printf("%s\n", dirs[i].name);
            if (i == 0) {
                return false;
            }
            dirs[i].parse(file, buffer, in);
        }
        i++;
    }
    return true;
}

/**************************************************************************/

Channel* allocChannels(Table* iniTable) {
    /* allocates the channel set and initialise with initial table data */
    Channel* c = (Channel*)AllocMem(
        MIDI_NUM_CHANNELS * sizeof(Channel),
        MEMF_PUBLIC | MEMF_CLEAR
    );
    if (c) {
        int i;
        for (i=0; i<MIDI_NUM_CHANNELS; i++) {
            c[i].output = i;
        }
    }
    return c;
}

/**************************************************************************/

void freeChannels(Channel* c) {
    /* frees the channel set */
    if (c) {
        FreeMem(c, MIDI_NUM_CHANNELS*sizeof(Channel));
    }
}

/**************************************************************************/

void listChannels(void) {
    int i;
    for (i = 0; i<MIDI_NUM_CHANNELS; i++) {
        printf("channel %d -> %d\n", i + 1, (int)(channels[i].output) + 1);
    }
}

/**************************************************************************/

Table* allocTable(const char* name) {
    /* allocates a table */
    Table* table = (Table*)AllocMem(sizeof(Table), MEMF_PUBLIC);
    if (table) {
        table->next = 0;
        if (name) {
            table->idHash = hashString(name);
        }
        else {
            table->idHash = 0;
        }
    }
    return table;
}

/**************************************************************************/

void freeTable(Table* t) {
    /* frees a table */
    if (t) {
        FreeMem(t, sizeof(Table));
    }
}

/**************************************************************************/

void addTable(Table* table) {
    /* adds a table to the list of tables */
    if (table) {
        if (!(tableList)) {
            tableList = table;
            return;
        }
        else {
            Table* t;
            for (t = tableList; t->next; t = t->next) {
            }
            t->next = table;
        }
    }
}

/**************************************************************************/

Table* findTable(const char* name) {
    /* finds a table by name (if present) */
    uint32 hash;
    Table* t;
    if (!name) {
        return tableList; /* the default table */
    }
    hash = hashString(name);
    for (t = tableList; t; t = t->next) {
        if (hash == t->idHash) {
            return t;
        }
    }
    return 0;
}

/**************************************************************************/

void dumpTable(const Table* t) {
    /* hex dumps a table */
    sint32 i;
    for (i = 0; i < MIDI_TABLE_SIZE; i += 16) {
        printf("\t%3d %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d\n",
            t->data[i],    t->data[i+1],  t->data[i+2],  t->data[i+3],
            t->data[i+4],  t->data[i+5],  t->data[i+6],  t->data[i+7],
            t->data[i+8],  t->data[i+9],  t->data[i+10], t->data[i+11],
            t->data[i+12], t->data[i+13], t->data[i+14], t->data[i+15]
        );
    }
}

/**************************************************************************/

void listTables(void) {
    Table* t;
    sint32 i = 0;
    for (t = tableList; t; t = t->next, i++) {
        printf("Table %ld : idHash 0x%08X\n", i, (unsigned)t->idHash);
    }
}

/**************************************************************************/

bool readWord(FILE* file, char* buffer) {
    /* reads a single non-comment word from a file */
    if (!file) {
        return false;
    }
    else {
        bool gotWord = false;
        while (!gotWord && !feof(file)) {
            if (fscanf(file, "%s", buffer) == 0) {
                return false;
            }
            if (buffer[0] == '/' && buffer[1] == '/') {
                sint32 ch;
                do {
                    ch = fgetc(file);
                } while (ch!='\n');
            }
            else {
                gotWord = true;
            }
        }
        return gotWord;
    }
    return false;
}

/**************************************************************************/

bool parseSetup(const char* fName) {
    FILE* file;
    char* buffer;
    puts("\nparseSetup()");
    if (!(buffer = (char*)AllocMem(FILE_PARSE_BUFFER, MEMF_PUBLIC|MEMF_CLEAR))) {
        puts("*** unable to allocate parse buffer");
        return false;
    }
    if (!(file = fopen(fName, "r"))) {
        FreeMem(buffer, FILE_PARSE_BUFFER);
        printf("*** unable to open %s\n", fName);
        return false;
    }

    while (!(feof(file))) {
        /* pull out a word */
        if (readWord(file, buffer)) {
            uint32 d = hashString(buffer);
            if (handleDirective(d, file, buffer, 0)==false) {
                break;
            }
        }
    }
    fclose(file);
    FreeMem(buffer, FILE_PARSE_BUFFER);
    puts("configuration complete\n");
    return true;
}

/**************************************************************************/

void parseTable(FILE* file, char* buffer, sint32 in) {
    Table* table=0;
    if (readWord(file, buffer)) {
        sint32 fill;
        if (table = allocTable(buffer)) {
            sint32 val = 0;
            sint32 i   = 0;
            printf("Define table %s\n", buffer);
            if (fscanf(file, "%ld {", &fill)==1) {
                for (i=0; i<MIDI_TABLE_SIZE; i++) {
                    table->data[i] = val;
                }
                while (buffer[0]!='}' && !feof(file)) {
                    readWord(file, buffer);
                    if (sscanf(buffer, "%ld:%ld", &i, &val) != 2) {
                        break;
                    }
                    table->data[i] = val;
                }
            }
            else {
                while (i < MIDI_TABLE_SIZE && buffer[0] != '}' && !feof(file)) {
                    readWord(file, buffer);
                    if (sscanf(buffer, "%ld", &val) == 0) {
                        break;
                    }
                    table->data[i++] = val;
                }
            }
            printf(
                "\tread  %ld\n"
                "\tid    0x%08X\n",
                i, (unsigned)table->idHash
            );
        }
        addTable(table);
    }
}

/**************************************************************************/

void parseCurve(FILE* file, char* buffer, sint32 in) {
    Table* table=0;
    if (readWord(file, buffer)) {
        sint32  min   = 0;
        sint32  max   = 127;
        sint32  zeroX = 0;
        sint32  zeroY = 0;
        float64 grad  = 1.0;
        float64 power = 1.0;

        if (table = allocTable(buffer)) {
            sint32 i=0;

            printf("Define curve %s\n", buffer);
            fscanf(file, "%s{", buffer);

            if (readWord(file, buffer)) {
                sscanf(buffer, "%ld", &min);
            }
            if (readWord(file, buffer)) {
                sscanf(buffer, "%ld", &max);
            }
            if (readWord(file, buffer)) {
                sscanf(buffer, "%ld", &zeroX);
            }
            if (readWord(file, buffer)) {
                sscanf(buffer, "%ld", &zeroY);
            }
            if (readWord(file, buffer)) {
                sscanf(buffer, "%lf", &grad);
            }
            if (readWord(file, buffer)) {
                sscanf(buffer, "%lf", &power);
            }
            fscanf(file, "%s}", buffer);
            printf(
                "\tmin   %ld\n"
                "\tmax   %ld\n"
                "\tzeroX %ld\n"
                "\tzeroY %ld\n"
                "\tgrad  %f\n"
                "\tpower %f\n"
                "\tid    0x%08X\n",
                min, max, zeroX, zeroY, grad, power,
                (unsigned)table->idHash
            );
            {
                float64 yScale = (max-min);
                grad /= MIDI_TABLE_SIZE;
                for ( i =0; i < MIDI_TABLE_SIZE; i++) {
                    sint32 x = i - zeroX;
                    sint32 y;
                    if (x < 0) {
                        y = (sint32)(( -yScale * (pow(grad * (-x), power)))) + zeroY;
                    }
                    else {
                        y = (sint32)((yScale * (pow(grad * x, power)))) + zeroY;
                    }
                    table->data[i] = y > max ? max : y < min ? min : y;
                }
            }
            addTable(table);
        }
    }
}

/**************************************************************************/

void parseChannel(FILE* file, char* buffer, sint32 in) {
    sint32 out;
    if (fscanf(file, "%ld %ld {", &in, &out)==2) {
        uint32 d;
        channels[in - 1].output = out - 1;
        printf("Define channel %ld -> %ld\n", in, out);
        while (buffer[0] != '}' && !feof(file)) {
            if (readWord(file, buffer)) {
                d = hashString(buffer);
                handleDirective(d, file, buffer, in);
            }
        }
    }
}

/**************************************************************************/

void parseProgram(FILE* file, char* buffer, sint32 in) {
    Table* table=0;
    if (readWord(file, buffer)) {
        if (table = findTable(buffer)) {
            channels[in - 1].programMap = table->data;
            return;
        }
    }
    printf("channel %ld - failed to set program table\n", in);
}

/**************************************************************************/

void parseProgBankMSB(FILE* file, char* buffer, sint32 in) {
    Table* table=0;
    if (readWord(file, buffer)) {
        if (table = findTable(buffer)) {
            channels[in - 1].progBankMSBMap = table->data;
            return;
        }
    }
    printf("channel %ld - failed to set progbankmsb table\n", in);
}

/**************************************************************************/

void parseProgBankLSB(FILE* file, char* buffer, sint32 in) {
    Table* table=0;
    if (readWord(file, buffer)) {
        if (table = findTable(buffer)) {
            channels[in - 1].progBankLSBMap = table->data;
            return;
        }
    }
    printf("channel %ld - failed to set progbanklsb table\n", in);
}

/**************************************************************************/

void parseProgTranspose(FILE* file, char* buffer, sint32 in) {
    Table* table=0;
    if (readWord(file, buffer)) {
        if (table = findTable(buffer)) {
            channels[in - 1].progTransMap = table->data;
            return;
        }
    }
    printf("channel %ld - failed to set progtranspose table\n", in);
}

/**************************************************************************/

void parseNotemap(FILE* file, char* buffer, sint32 in) {
    Table* table    = 0;
    sint32 progNum1 = 0;
    sint32 progNum2 = 127;
    bool   range = false;
    readWord(file, buffer);
    if (sscanf(buffer, "%ld", &progNum1) == 1) {
        readWord(file, buffer);
        if (sscanf(buffer, "%ld", &progNum2) == 1) {
            range = true;
            readWord(file, buffer);
        }
        if ( (table = findTable(buffer)) ) {
            if (range) {
                sint32 i;
                for (i = progNum1; i <= progNum2; i++) {
                    channels[in - 1].noteMap[i] = table->data;
                }
                return;
            }
            else {
                channels[in - 1].noteMap[progNum1] = table->data;
                return;
            }
        }
        else {
            printf("channel %ld - failed to set keymap for %ld...%ld\n", in, progNum1, progNum2);
        }
    }
    else {
        printf("channel %ld - failed to set keymap for %ld\n", in, progNum1);
    }
}

/**************************************************************************/

void parseVelocity(FILE* file, char* buffer, sint32 in) {
    Table* table = 0;
    if (readWord(file, buffer)) {
        if (table = findTable(buffer)) {
            channels[in - 1].velocityMap = table->data;
            return;
        }
    }
    printf("channel %ld - failed to set velocity table\n", in);
}

/**************************************************************************/

void parseController(FILE* file, char* buffer, sint32 in) {
    Table* table = 0;
    if (readWord(file, buffer)) {
        if (table = findTable(buffer)) {
            channels[in - 1].controlMap = table->data;
            return;
        }
    }
    printf("channel %ld - failed to set controller table\n", in);
}

/**************************************************************************/

void parseCtrlRange(FILE* file, char* buffer, sint32 in) {
    Table* table   = 0;
    sint32 ctrlNum = 0;
    if (readWord(file, buffer)) {
        if (sscanf(buffer, "%ld", &ctrlNum)==1) {
            if (readWord(file, buffer)) {
                if (table = findTable(buffer)) {
                    channels[in - 1].controlRangeMap[ctrlNum] = table->data;
                    return;
                }
            }
        }
    }
    printf("channel %ld - failed to set controller range table\n", in);
}

/**************************************************************************/

void parseCtrlInit(FILE* file, char* buffer, sint32 in) {
    Table* table = 0;
    sint32 ctrlNum;
    sint32 ctrlVal;
    if (readWord(file, buffer)) {
        if (sscanf(buffer, "%ld", &ctrlNum) == 1) {
            /* controller number specified. try to get value  */
            if (readWord(file, buffer)) {
                if (sscanf(buffer, "%ld", &ctrlVal) == 1) {
                    /* if no table exists, attempt allocate it now */
                    if (!(channels[in - 1].controlInit)) {
                        if ( (table = allocTable(0)) ) {
                            int j;
                            for (j = 0; j < MIDI_NUM_CONTROLLERS; j++) {
                                table->data[j]=0xFF;
                            }
                            addTable(table);
                            channels[in - 1].controlInit = table->data;
                            channels[in - 1].controlInit[ctrlNum] = ctrlVal;
                            return;
                        }
                    }
                    else {
                        channels[in - 1].controlInit[ctrlNum] = ctrlVal;
                        return;
                    }
                }
            }
        }
        else if ( (table = findTable(buffer)) ) {
            /* an existing table was specified rather than a single controller num */
            channels[in - 1].controlInit = table->data;
            return;
        }
    }
    printf("channel %ld - failed to set initial controller table\n", in);
}

/***************************************************************************/

bool loadSetup(const char* configFile) {
    initDirectives();
    /* allocate the channels */
    if (!(channels = allocChannels(tableList))) {
        return false;
    }
    return parseSetup(configFile);
}

/**************************************************************************/

void freeSetup(void) {
    /* frees the entire list of tables */
    Table* t;
    Table* next;
    int    i = 0;
    puts("\nfreeSetup()...");
    for (t = tableList; t; i++) {
        next = t->next;
        printf("freeing table %d [hash: 0x%08X]\n", i, (unsigned)t->idHash);
        freeTable(t);
        t = next;
    }
    if (channels) {
        freeChannels(channels);
    }
    tableList  = 0;
    channels   = 0;
    directives = 0;
    puts("\ndone");
}

/**************************************************************************/

sint32 remapMIDIData(uint8* dBuf, uint8* sBuf, sint32 sLen) {
    sint32   dLen = sLen;
    sint32   cmd  = (sBuf[0]) & 0xF0;
    sint32   in   = (sBuf[0]) & 0x0F;
    Channel* out  = &channels[in];
    switch (cmd) {
        case MS_NOTEOFF:
        case MS_NOTEON: {
            sint32 key = sBuf[1];
            sint32 vel = sBuf[2];
            if (out->noteMap[out->currProgIn]) {
                /* unique key remap for this program# ? */
                key = (out->noteMap[out->currProgIn])[key];
            }
            else {
                /* defualt transpose for this program# ? */
                key += out->currTrans;
            }
            if (out->velocityMap && vel!=0) {
                /* velocity map for this channel# ? */
                vel = out->velocityMap[vel];
            }
            dBuf[0] = cmd | out->output;
            dBuf[1] = key;
            dBuf[2] = vel;
            dLen = 3;
        }
        break;

        case MS_PROG: {
            sint32 i    = 0;
            sint32 prg = out->currProgIn = sBuf[1];
            if (out->progTransMap) {
                /* defualt transpose for this program# ? */
                out->currTrans = out->progTransMap[prg];
            }
            if (out->progBankMSBMap) {
                /* default bank MSB for this program# ? */
                dBuf[i++] = MS_CTRL | out->output;
                dBuf[i++] = 0;
                dBuf[i++] = out->progBankMSBMap[prg];
/*
                printf(
                    "MSB : 0x%02X 0x%02X 0x%02X : ",
                    (unsigned)(dBuf[i - 3]),
                    (unsigned)(dBuf[i - 2]),
                    (unsigned)(dBuf[i - 1])
                );
*/
            }
            if (out->progBankLSBMap) {
                /* default bank LSB for this program# ? */
                dBuf[i++] = MS_CTRL | out->output;
                dBuf[i++] = 32;
                dBuf[i++] = out->progBankLSBMap[prg];
/*
                printf(
                    "LSB : 0x%02X 0x%02X 0x%02X : ",
                    (unsigned)(dBuf[i - 3]),
                    (unsigned)(dBuf[i - 2]),
                    (unsigned)(dBuf[i - 1])
                );
*/
            }
            if (out->programMap) {
                prg = out->programMap[prg];
            }
            dBuf[i++] = MS_PROG | out->output;
            dBuf[i++] = prg;
/*
            printf(
                "PC : 0x%02X 0x%02X\n",
                (unsigned)(dBuf[i - 2]),
                (unsigned)(dBuf[i - 1])
            );
*/
            dLen = i;
        }
        break;

        case MS_CTRL: {
            sint32 ctl = sBuf[1];
            sint32 val = sBuf[2];
            if (out->controlMap[ctl]) {
                /* control# remap for this channel# ? */
                ctl = out->controlMap[ctl];
            }
            if (out->controlRangeMap[ctl]) {
                /* range map for this (remapped) control# ? */
                val = (out->controlRangeMap[ctl])[val];
            }
            dBuf[0] = MS_CTRL | out->output;
            dBuf[1] = ctl;
            dBuf[2] = val;
            dLen    = 3;
        }
        break;

        default: {
          /* copy message */
          while (sLen--) {
            *dBuf++ = *sBuf++;
          }
        }
        break;
    }
    return dLen; /* bytes written */
}
