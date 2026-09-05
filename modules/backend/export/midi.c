#include "export_internal.h"
#include <utils.h>

#define DIVISION 480


void writeVLQforUint8(FILE* fptr, uint8_t data) {
    if (data<128) fputc((int)data, fptr);
    else {
        uint8_t t1=(data>>7)|0x80, t2=data&0x7F;
        fputc((int)t1, fptr);
        fputc((int)t2, fptr);
    }
}

void writeVLQforUint16(FILE* fptr, uint16_t data) {
    if (data<128) fputc((int)data, fptr);
    else if (data<16384) {
        uint8_t t1=(data>>7)|0x80, t2=data&0x7F;
        fputc((int)t1, fptr);
        fputc((int)t2, fptr);
    } else {
        uint8_t t1=(data>>14)|0x80, t2=(data>>7)|0x80, t3=data&0x7F;
        fputc((int)t1, fptr);
        fputc((int)t2, fptr);
        fputc((int)t3, fptr);
    }
}

void writeVLQforUint32(FILE* fptr, uint32_t data) {
    if (data<128) fputc((int)data, fptr);
    else if (data<16384) {
        uint8_t t1=((data>>7)&0x7F)|0x80, t2=data&0x7F;
        fputc((int)t1, fptr);
        fputc((int)t2, fptr);
    } else if (data<2097152) {
        uint8_t t1=((data>>14)&0x7F)|0x80, t2=((data>>7)&0x7F)|0x80, t3=data&0x7F;
        fputc((int)t1, fptr);
        fputc((int)t2, fptr);
        fputc((int)t3, fptr);
    } else if (data<268435456) {
        uint8_t t1=((data>>21)&0x7F)|0x80, t2=((data>>14)&0x7F)|0x80, t3=((data>>7)&0x7F)|0x80, t4=data&0x7F;
        fputc((int)t1, fptr);
        fputc((int)t2, fptr);
        fputc((int)t3, fptr);
        fputc((int)t4, fptr);
    } else {
        fprintf(stderr, "Invalid VLQ value range (val: %u, maxVLQ: %u)\n", data, 268435456);
    }
}

void writeUint32BigEndian(FILE* fptr, uint32_t data) {
    for (int i=0; i<4; i++) {
        int t = ((data >> ((3-i)<<3)) & 0xFF);
        fputc(t, fptr);
    }
}

void writeUint16BigEndian(FILE* fptr, uint16_t data) {
    for (int i=0; i<2; i++) {
        int t = ((data >> ((1-i)<<3)) & 0xFF);
        fputc(t, fptr);
    }
}

int getNumberOfBytesForVLQ(uint32_t data) {
    if (data<128) return 1;
    else if (data<16384) return 2;
    else if (data<2097152) return 3;
    else if (data<268435456) return 4;
    else return 0;
}


static int writeMidiHeader(FILE* fptr) {
    if (!fptr) return 1;

    char* chunkType = "MThd";
    uint32_t length = 6;
    uint16_t format = (globalProject->tracksNum>0);
    uint16_t ntracks = 1+globalProject->tracksNum;
    uint16_t division = DIVISION;

    writeString(fptr, chunkType);
    writeUint32BigEndian(fptr, length);
    writeUint16BigEndian(fptr, format);
    writeUint16BigEndian(fptr, ntracks);
    writeUint16BigEndian(fptr, division);

    return 0;
}

static int writeMidiHeaderForCompactConversion(FILE* fptr, uint8_t tracks) {
    if (!fptr) return 1;

    char* chunkType = "MThd";
    uint32_t length = 6;
    uint16_t format = (globalProject->tracksNum>0);
    uint16_t ntracks = 1+tracks;
    uint16_t division = DIVISION;

    writeString(fptr, chunkType);
    writeUint32BigEndian(fptr, length);
    writeUint16BigEndian(fptr, format);
    writeUint16BigEndian(fptr, ntracks);
    writeUint16BigEndian(fptr, division);

    return 0;
}

static int writeMidiTrackTitleThroughMetaEvent(FILE* fptr, const char* title) {
    if (!fptr) return 1;
    if (!title) return 0;
    writeVLQforUint8(fptr, 0);              // v_time
    fputc(0xFF, fptr);                      // meta_event
    fputc(MIDI_META_TRACK_NAME, fptr);      // meta_type
    int len = strlen(title);
    writeVLQforUint32(fptr, (uint32_t)len);
    writeString(fptr, title);
    return 0;
}

static int writeMidiEndOfTrack(FILE* fptr) {
    if (!fptr) return 1;
    writeVLQforUint8(fptr, 0);              // v_time
    fputc(0xFF, fptr);                      // meta_event
    fputc(MIDI_META_END_OF_TRACK, fptr);    // meta_type
    writeVLQforUint8(fptr, 0);
    return 0;
}

static int _getDDfromTimeSignature(struct time_signature ts) {
    switch (ts.denominator) {
        case 1: return 0;
        case 2: return 1;
        case 4: return 2;
        case 8: return 3;
        default: return 2;
    }
}

static int writeMidiTimeSignature(FILE* fptr, struct time_signature ts) {
    if (!fptr) return 1;
    writeVLQforUint8(fptr, 0);                          // v_time
    fputc(0xFF, fptr);                                  // meta_event
    fputc(MIDI_META_TIME_SIGNATURE, fptr);              // meta_type
    writeVLQforUint8(fptr, 4);                          // v_length
    fputc(ts.numerator, fptr);                          // nn
    fputc(_getDDfromTimeSignature(ts), fptr);           // dd
    uint8_t cc = (ts.denominator == 8 && ts.numerator % 3 == 0) ? 36 : 24;
    fputc(cc, fptr);      // cc
    fputc(8, fptr);                                     // bb

    return 0;
}

static int writeMidiTempo(FILE* fptr, uint16_t tempo) {
    if (!fptr) return 1;
    writeVLQforUint8(fptr, 0);                          // v_time
    fputc(0xFF, fptr);                                  // meta_event
    fputc(MIDI_META_TEMPO_SETTING, fptr);               // meta_type
    writeVLQforUint8(fptr, 3);                          // v_length
    
    uint32_t mpqn = 60000000/(uint32_t)tempo;
    uint8_t t1=((mpqn>>16)&0xFF), t2=((mpqn>>8)&0xFF),t3=(mpqn&0xFF);
    
    fputc(t1, fptr);
    fputc(t2, fptr);
    fputc(t3, fptr);

    return 0;
}

static uint8_t _getPanningForUint8(float panning) {
    return (uint8_t)(127.0*floatClip(panning, 0, 1));
}

static uint8_t _getVolumeForUint8(float volume) {
    return (uint8_t)(127.0*floatClip(volume, 0, 1));
}

static int writeMidiPanning(FILE* fptr, float panning, uint8_t channel, uint8_t* runningStatus) {
    if (!fptr || channel>=16 || panning<0 || panning>1) return 1;
    
    writeVLQforUint8(fptr, 0);                              // v_time
    uint8_t status = (MIDI_CHANNEL_CONTROL_CHANGE|channel)&0xFF;
    if (*runningStatus!=status) {
        fputc(status, fptr);                                // status_byte
        *runningStatus = status;
    }
    fputc(MIDI_CONTROLLER_MSB_PAN, fptr);                   // controller_id
    fputc(_getPanningForUint8(panning), fptr);

    return 0;
}

static int writeMidiChannelVolume(FILE* fptr, float volume, uint8_t channel) {
    if (!fptr || channel>=16 || volume<0 || volume>1) return 1;   
    
    writeVLQforUint8(fptr, 0);                              // v_time
    fputc(MIDI_CHANNEL_CONTROL_CHANGE|channel, fptr);       // status_byte
    fputc(MIDI_CONTROLLER_MSB_CHANNEL_VOLUME, fptr);        // controller_id
    fputc(_getVolumeForUint8(volume), fptr);

    return 0;
}

static int writeMidiProgramChange(FILE* fptr, uint8_t program, uint8_t channel, uint8_t* runningStatus) {
    if (!fptr || channel>=16 || program>127) return 1;   
    
    writeVLQforUint8(fptr, 0);                              // v_time
    uint8_t status = (MIDI_CHANNEL_PROGRAM_CHANGE|channel)&0xFF;
    if (*runningStatus!=status) {
        fputc(status, fptr);                                // status_byte
        *runningStatus = status;
    }
    fputc(program, fptr);

    return 0;
}

static int writeMidiMetaTrack(FILE* fptr) {
    if (!fptr || !globalProject) return 1;

    char* chunkType = "MTrk";
    uint32_t length = 0;
    
    writeString(fptr, chunkType);
    long lengthSeek = ftell(fptr);
    writeUint32BigEndian(fptr, length);

    writeMidiTrackTitleThroughMetaEvent(fptr, "Control Track");
    writeMidiTimeSignature(fptr, globalProject->timeSignature);
    writeMidiTempo(fptr, globalProject->tempo);
    writeMidiEndOfTrack(fptr);

    long trackEndSeek = ftell(fptr);
    length = trackEndSeek-lengthSeek-4;

    fseek(fptr, lengthSeek, SEEK_SET);
    writeUint32BigEndian(fptr, length);
    fseek(fptr, trackEndSeek, SEEK_SET);

    return 0;
}

static int _noteEventCompare(const void* a, const void* b) {
    if (!a && !b) return 0;
    if (!a || !b) return a?-1:1;
    
    struct note_on_data na = *(struct note_on_data*)a;
    struct note_on_data nb = *(struct note_on_data*)b;

    if (na.timestamp>nb.timestamp) return 1;
    if (na.timestamp<nb.timestamp) return -1;
    if (na.velocity>nb.velocity) return 1;
    if (na.velocity<nb.velocity) return -1;
    return 0;
}

static struct note_on_data* _setupTrackNoteEvents(Track track, uint8_t channel) {
    if (!track || channel>=16 || !(track->numElements)) return NULL;

    uint32_t n=track->numElements;
    uint32_t m=n<<1;

    struct note_on_data* arr = malloc(m*sizeof(struct note_on_data));
    uint16_t trackIdx = track-globalProject->tracks;

    for (uint32_t i=0; i<n; i++) {
        struct note_data nt = *((track->notes)[i]);
        arr[i<<1] = (struct note_on_data){.key=nt.key, .velocity=nt.velocity, .channel=channel, .trackIdx=trackIdx, .timestamp=nt.timestamp};
        arr[(i<<1)+1] = (struct note_on_data){.key=nt.key, .velocity=0, .channel=channel, .trackIdx=trackIdx, .timestamp=nt.timestamp+nt.duration};
    }

    qsort(arr, m, sizeof(struct note_on_data), _noteEventCompare);
    double beatTimestamp = trackPiecesInBeat();

    for (uint32_t i=m-1; i; i--) {
        arr[i].timestamp -= arr[i-1].timestamp;
        arr[i].timestamp = (uint32_t)(arr[i].timestamp/beatTimestamp*DIVISION);    //(uint32_t)((beatDuration*DIVISION*arr[i].timestamp)/beatTimestamp);
    }
    arr[0].timestamp = (uint32_t)(arr[0].timestamp/beatTimestamp*DIVISION);
    return arr;
}

static struct note_on_data* _setupTrackNoteEventsForCompactConversion(struct tracks_in_channel* trInCh, uint64_t* bufferSize) {
    if (!trInCh || !(trInCh->isUsed)) return NULL;

    uint8_t channel = trInCh->channel;
    uint16_t tracksNum = trInCh->size;
    uint64_t notesNum=0, eventsNum=0;
    
    for (uint16_t i=0; i<tracksNum; i++) {
        notesNum += (globalProject->tracks)[(trInCh->buffer)[i]].numElements;
    }
    eventsNum = (notesNum<<1);
    if (!notesNum) return NULL;

    struct note_on_data* arr = malloc(eventsNum*sizeof(struct note_on_data));
    if (!arr) return NULL;

    uint64_t idx=0;
    for (uint16_t i=0; i<tracksNum; i++) {
        uint16_t trackIdx = (trInCh->buffer)[i];
        Track track = globalProject->tracks+trackIdx;
        float trackVelocity = track->velocity;
        uint64_t startIdx=idx;
        uint32_t curNumElements=track->numElements;
        uint64_t endIdx=idx+curNumElements;

        for (; idx<endIdx; idx++) {
            struct note_data nt = *((track->notes)[idx-startIdx]);
            arr[idx<<1] = (struct note_on_data){.key=nt.key, .velocity=(uint8_t)(nt.velocity*trackVelocity), .channel=channel, .trackIdx=trackIdx, .timestamp=nt.timestamp};
            arr[(idx<<1)+1] = (struct note_on_data){.key=nt.key, .velocity=0, .channel=channel, .trackIdx=trackIdx, .timestamp=nt.timestamp+nt.duration};
        }
    }

    qsort(arr, eventsNum, sizeof(struct note_on_data), _noteEventCompare);
    double beatTimestamp = trackPiecesInBeat();

    for (uint32_t i=eventsNum-1; i; i--) {
        arr[i].timestamp -= arr[i-1].timestamp;
        arr[i].timestamp = (uint32_t)(arr[i].timestamp/beatTimestamp*DIVISION);
    }
    arr[0].timestamp = (uint32_t)(arr[0].timestamp/beatTimestamp*DIVISION);
    *bufferSize = eventsNum;
    return arr;
}

static int writeMidiNoteOnOffEvent(FILE* fptr, struct note_on_data event, uint8_t* runningStatus) {
    if (!fptr || event.key>127 || event.velocity>127 || event.channel>15) return 1;

    writeVLQforUint32(fptr, event.timestamp);
    uint8_t status = (((event.velocity>0)?MIDI_CHANNEL_NOTE_ON:MIDI_CHANNEL_NOTE_OFF)|event.channel)&0xFF;
    if (*runningStatus!=status) {
        fputc(status, fptr);
        *runningStatus = status;
    }
    fputc(event.key, fptr);
    fputc(event.velocity, fptr);

    return 0;
}


static int writeMidiTrack(FILE* fptr, Track track, uint8_t channel) {
    if (!fptr || !track) return 1;
    
    uint8_t runningStatus = 0;
    int ret = 0;
    char* chunkType = "MTrk";
    uint32_t length = 0;
    
    writeString(fptr, chunkType);
    long lengthSeek = ftell(fptr);
    writeUint32BigEndian(fptr, length);

    ret += writeMidiTrackTitleThroughMetaEvent(fptr, track->title);
    ret += writeMidiChannelVolume(fptr, track->velocity, channel);
    ret += writeMidiPanning(fptr, track->panning, channel, &runningStatus);
    if (channel!=9) ret += writeMidiProgramChange(fptr, track->program, channel, &runningStatus);
    

    uint32_t n=(track->numElements<<1);
    struct note_on_data* arr = _setupTrackNoteEvents(track, channel);

    
    if (arr) {
        for (uint32_t i=0; i<n; i++) ret += writeMidiNoteOnOffEvent(fptr, arr[i], &runningStatus);
        free(arr);
    }
    
    writeMidiEndOfTrack(fptr);

    long trackEndSeek = ftell(fptr);
    length = trackEndSeek-lengthSeek-4;

    fseek(fptr, lengthSeek, SEEK_SET);
    writeUint32BigEndian(fptr, length);
    fseek(fptr, trackEndSeek, SEEK_SET);

    return ret;
}

static const char* _getTrackTitleForCompactConversion(uint8_t channel) {
    if (channel>15) return NULL;
    static const char* titles[16] = {
        "Piano",
        "Chromatic Percussion",
        "Organ",
        "Guitar",
        "Bass",
        "Strings",
        "Ensemble",
        "Brass",
        "Reed",
        "Drums",
        "Pipe",
        "Synth Lead",
        "Synth Pad",
        "Synth Effects",
        "Ethnic",
        "Percussive & Sound Effects"
    };
    return titles[channel];
}

static int writeMidiTrackForCompactConversion(FILE* fptr, struct tracks_in_channel* trInCh) {
    if (!fptr || !trInCh || !(trInCh->buffer)) return 1;
    
    int ret = 0;
    uint8_t channel = trInCh->channel, runningStatus=0;
    char* chunkType = "MTrk";
    uint32_t length = 0;
    
    writeString(fptr, chunkType);
    long lengthSeek = ftell(fptr);
    writeUint32BigEndian(fptr, length);

    ret += writeMidiTrackTitleThroughMetaEvent(fptr, _getTrackTitleForCompactConversion(channel));
    int updatePan=1, updateProgram=(channel!=9), lastProgram=255;
    float lastPan=0.5;

    uint64_t bufferSize = 0;
    struct note_on_data* arr = _setupTrackNoteEventsForCompactConversion(trInCh, &bufferSize);
    if (arr) {
        for (uint64_t i=0; i<bufferSize; i++) {
            Track track = globalProject->tracks+arr[i].trackIdx;
            if (updateProgram && lastProgram!=track->program) {
                lastProgram = track->program;
                ret += writeMidiProgramChange(fptr, lastProgram, channel, &runningStatus);
            }
            if (updatePan && lastPan!=track->panning) {
                lastPan = track->panning;
                writeMidiPanning(fptr, lastPan, channel, &runningStatus);
            }
            ret += writeMidiNoteOnOffEvent(fptr, arr[i], &runningStatus);
        }
        free(arr);
    }
    
    writeMidiEndOfTrack(fptr);

    long trackEndSeek = ftell(fptr);
    length = trackEndSeek-lengthSeek-4;

    fseek(fptr, lengthSeek, SEEK_SET);
    writeUint32BigEndian(fptr, length);
    fseek(fptr, trackEndSeek, SEEK_SET);

    return ret;
}


int canBeDirectlyConvertedToMidi() {
    if (!globalProject) return 0;
    uint16_t n=globalProject->tracksNum;
    //printf("canBeDirectlyConvertedToMidi: total tracks = %u\n", n);
    if (n>16) return 0;
    for (uint16_t i=0; i<n-1; i++) {
        for (uint16_t j=i+1; j<n; j++) {
            //printf("Checking Tracks #%u and #%u, with prorgams %u and %u respectively\n", i, j, (globalProject->tracks)[i].program, (globalProject->tracks)[j].program);
            if ((globalProject->tracks)[i].program==(globalProject->tracks)[j].program) return 0;
        }
    }
    return 1;
}



int writeRegularMidi(FILE* fptr) {
    if (!fptr || !globalProject) return 1;

    int ret=0;
    ret += writeMidiHeader(fptr);
    ret += writeMidiMetaTrack(fptr);

    uint16_t n=globalProject->tracksNum;
    uint8_t channels[16]={0,};
    uint8_t cur=0;

    for (uint16_t i=0; i<n; i++) {
        if (midiGetProgramType((globalProject->tracks)[i].program)==MPT_DRUMS) channels[i]=9;
        else {
            channels[i]=cur++;
            if (cur==9) cur++;
        }
    }

    for (uint16_t i=0; i<n; i++) {
        ret += writeMidiTrack(fptr, globalProject->tracks+i, channels[i]);
    }
    return ret;
}


static struct tracks_in_channel _setupTracksInChannel(uint8_t channel) {
    return (struct tracks_in_channel){.channel=channel, .isUsed=0, .capacity=0, .size=0, .buffer=NULL};
}

static void _addTrackInChannel(struct tracks_in_channel* trInCh, uint16_t trackIdx) {
    if (!trInCh) return;
    if (trInCh->capacity && trInCh->buffer) {
        if (trInCh->capacity<=trInCh->size) {
            uint16_t newCap = (trInCh->capacity<<1);
            uint16_t* arr = realloc(trInCh->buffer, newCap*sizeof(uint16_t));
            if (!arr) return;

            trInCh->buffer = arr;
            trInCh->capacity = newCap;
        }
    } else {
        trInCh->capacity = 0;
        if (trInCh->buffer) free(trInCh->buffer);

        uint16_t cap = 4;
        trInCh->buffer = malloc(cap*sizeof(uint16_t));
        if (!(trInCh->buffer)) return;
        trInCh->capacity = cap;
    }
    (trInCh->buffer)[(trInCh->size)++] = trackIdx;
    trInCh->isUsed = 1;
}

static void _freeTracksInChannel(struct tracks_in_channel* trInCh) {
    if (!trInCh) return;
    if (trInCh->buffer) free(trInCh->buffer);
}

static void _freeMidiChannelExport(struct midi_channel_export* ptr) {
    if (!ptr) return;
    for (uint8_t i=0; i<16; i++) {
        _freeTracksInChannel(&((ptr->channels)[i]));
    }
}

static uint8_t _getChannelForProgram(uint8_t program) {
    enum midi_program_type mpt = midiGetProgramType(program);
    switch (mpt) {
        case MPT_PIANO: return 0;
        case MPT_CHROMATIC_PERCUSSION: return 1;
        case MPT_ORGAN: return 2;
        case MPT_GUITAR: return 3;
        case MPT_BASS: return 4;
        case MPT_STRINGS: return 5;
        case MPT_ENSEMBLE: return 6;
        case MPT_BRASS: return 7;
        case MPT_REED: return 8;
        case MPT_DRUMS: return 9;
        case MPT_PIPE: return 10;
        case MPT_SYNTH_LEAD: return 11;
        case MPT_SYNTH_PAD: return 12;
        case MPT_SYNTH_EFFECTS: return 13;
        case MPT_ETHNIC: return 14;
        case MPT_PERCUSSIVE:
        case MPT_SOUND_EFFECTS: return 15;
        case MPT_END:  
        default: return 16;     // wrong value intended
    }
}

static struct midi_channel_export _setupCompactMidiChannels() {
    struct midi_channel_export ret = {0,};
    for (uint8_t i=0; i<16; i++) ret.channels[i] = _setupTracksInChannel(i);

    if (!globalProject) return ret;

    uint16_t n=globalProject->tracksNum;
    for (uint16_t i=0; i<n; i++) {
        Track track = globalProject->tracks+i;
        if (track->numElements) {
            uint8_t ch = _getChannelForProgram(track->program);
            if (ch<16) _addTrackInChannel(&((ret.channels)[ch]), i);
        }
    }

    for (uint8_t i=0; i<16; i++) ret.channelsUsed += (ret.channels)[i].isUsed;

    return ret;
}

int writeCompactMidi(FILE* fptr) {
    if (!fptr || !globalProject) return 1;

    printf("Compact Midi:\n");

    int ret=0;
    struct midi_channel_export channelSettings = _setupCompactMidiChannels();

    ret += writeMidiHeaderForCompactConversion(fptr, channelSettings.channelsUsed);
    ret += writeMidiMetaTrack(fptr);

    for (uint8_t i=0; i<16; i++) {
        if (channelSettings.channels[i].isUsed) {
            printf("Channel #%u:\n", i);
            for (uint16_t j=0; j<channelSettings.channels[i].size; j++) {
                printf("- Track %u\n", channelSettings.channels[i].buffer[j]);
            }
            ret += writeMidiTrackForCompactConversion(fptr, &(channelSettings.channels[i]));
        
        }
    }

    _freeMidiChannelExport(&channelSettings);

    return ret;
}

int exportProjectAsMidi(const char* filename) {
    if (!filename) return 1;

    FILE* fptr = fopen(filename, "wb");
    if (!fptr) return 1;    // Couldn't open file

    int directMidi= canBeDirectlyConvertedToMidi();
    int ret = 0;
    if (directMidi) ret += writeRegularMidi(fptr);
    else ret += writeCompactMidi(fptr);
    fclose(fptr);

    return ret;
}