#include <fileops.h>
#include <string.h>



void writeUint32(FILE* fptr, uint32_t data) {
    for (int i=0; i<4; i++) {
        int t = ((data >> (i<<3)) & 0xFF);
        fputc(t, fptr);
    }
}

void writeUint16(FILE* fptr, uint16_t data) {
    for (int i=0; i<2; i++) {
        int t = ((data >> (i<<3)) & 0xFF);
        fputc(t, fptr);
    }
}

void writeUint8(FILE* fptr, uint8_t data) {
    fputc((int)data, fptr);
}

int readUint32(FILE* fptr, uint32_t* ret) {
    uint32_t tmp = 0;
    for (int i=0; i<4; i++) {
        int c = fgetc(fptr);
        if (c==EOF) return 1;
        tmp |= (((uint32_t)c)<<(i<<3));
    }
    *ret = tmp;
    return 0;
}

int readUint16(FILE* fptr, uint16_t* ret) {
    uint16_t tmp = 0;
    for (int i=0; i<2; i++) {
        int c = fgetc(fptr);
        if (c==EOF) return 1;
        tmp |= (((uint16_t)c)<<(i<<3));
    }
    *ret = tmp;
    return 0;
}

int readUint8(FILE* fptr, uint8_t* ret) {
    int c = fgetc(fptr);
    if (c==EOF) return 1;
    *ret = (uint8_t)c;
    return 0;
}

int readString(FILE* fptr, char* buffer, int len) {
    for (int i=0; i<len; i++) {
        int c = fgetc(fptr);
        if (c==EOF) return 1;
        buffer[i] = (uint8_t)c;
    }
    buffer[len]=0;
    return 0;
}


void writeString(FILE* fptr, const char* string) {
    const char* ch = string;
    while (*ch) {
        writeUint8(fptr, (uint8_t)(*ch));
        ch++;
    }
}