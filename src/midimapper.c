/*
    MIDI mapper experiment v0.0
    (c) K Churchill 2004
*/

#include "midimapper.h"

/**************************************************************************/

struct Library* MidiBase = 0;
struct MSource* source   = 0;
struct MDest*   dest     = 0;
struct MRoute*  sRoute   = 0;
struct MRoute*  dRoute   = 0;
const char*     sName    = "MidiOut";
const char*     dName    = "MidiIn";

extern Channel* channels;

typedef uint32 (*FillBuffer)(void);

FillBuffer dummyFill = 0;

/**************************************************************************/

void done(void) {
    fflush(stdout);
    if (dRoute) {
        DeleteMRoute(dRoute);
        dRoute = 0;
    }
    if (sRoute) {
        DeleteMRoute(sRoute);
        sRoute = 0;
    }
    if (dest) {
        DeleteMDest(dest);
        dest = 0;
    }
    if (source) {
        DeleteMSource(source);
        source = 0;
    }
    if (MidiBase) {
        CloseLibrary(MidiBase);
        MidiBase = 0;
    }
}

/**************************************************************************/

bool init(void) {
    if (!(MidiBase = OpenLibrary(MIDINAME, MIDIVERSION))) {
        printf("Couldn't open %s version %ld\n", MIDINAME, MIDIVERSION);
        return false;
    }
    if (!(dest = CreateMDest(0, 0))) {
        printf("Couldn't create MDest\n");
        return false;
    }
    if (!(dRoute = MRouteDest(dName, dest, 0))) {
        printf("Coudln't create dest Route\n");
        return false;
    }
    if (!(source = CreateMSource(0, 0))) {
        printf("Couldn't create MSource\n");
        return false;
    }
    if (!(sRoute = MRouteSource(source, sName, 0))) {
        printf("Coudln't create source Route\n");
        return false;
    }
    return true;
}

/**************************************************************************/

void showPacket(struct MidiPacket* packet) {
    if (packet->Type == MMF_SYSEX) {
        printf("[sysex message, ignoring]\n");
    }
    else {
        int    len = packet->Length;
        uint8* msg = packet->MidiMsg;
        /* ignore active sensing messages*/
        if (len != 1 && msg[0] != 0xFE) {
            while (len--) {
                printf("0x%02X ", *msg++);
            }
            printf("\n");
        }
    }
}

/**************************************************************************/

void processPacket(struct MidiPacket* packet) {
    char outBuffer[256];
    if (packet->Type != MMF_SYSEX) {
        sint32 len = remapMIDIData(outBuffer, packet->MidiMsg, packet->Length);
        PutMidiStream(source, dummyFill, outBuffer, len, len);
    }
}

/**************************************************************************/

void processMessages(void) {
    struct MidiPacket* packet = 0;
    uint32 flags = SIGBREAKF_CTRL_C | (1L << dest->DestPort->mp_SigBit);

    /* change the task priority for message processing */
    sint32 oldPri = SetTaskPri(FindTask(0), 20);
    while (!(Wait(flags) & SIGBREAKF_CTRL_C)) {
        while (packet = GetMidiPacket(dest)) {
            processPacket(packet);
            //showPacket(packet);
            FreeMidiPacket(packet);
        }
    }
    /* Here we must have had a CTRL C, but it is possible there are packets left*/
    while (packet = GetMidiPacket(dest)) {
        processPacket(packet);
        //showPacket(packet);
        FreeMidiPacket(packet);
    }
    /* restore the old priority */
    SetTaskPri(FindTask(0), oldPri);
}

/**************************************************************************/

void initChannels(void) {
    sint32 i, j;
    uint8  initBuffer[4];
    for (i = 0; i < MIDI_NUM_CHANNELS; i++) {
        if (channels[i].controlInit) {
            for (j = 0; j<MIDI_NUM_CONTROLLERS; j++) {
                if (channels[i].controlInit[j] < 0x80) {
                    initBuffer[0] = MS_CTRL | channels[i].output;
                    initBuffer[1] = j;
                    initBuffer[2] = channels[i].controlInit[j];
                    PutMidiStream(source, dummyFill, initBuffer, 3, 3);
                }
            }
        }
    }
}

/**************************************************************************/

int main(int arg_n, char** arg_v) {
    const char* cfgFile;
    if (init() == true) {
        printf("MIDI ReMapper\n");
        cfgFile = arg_n > 1 ? arg_v[1] : "remap.cfg";
        if (loadSetup(cfgFile)) {
            initChannels();
            printf("\nInitialisation complete: Press CTRL-C to abort\n");
            processMessages();
        }
        freeSetup();
    }
    done();
    return 0;
}
