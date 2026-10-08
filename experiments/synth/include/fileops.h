#ifndef FILEOPS_H
#define FILEOPS_H

#include <stdio.h>
#include <stdint.h>


void writeUint32(FILE* fptr, uint32_t data);
void writeUint16(FILE* fptr, uint16_t data);
void writeUint8(FILE* fptr, uint8_t data);
int readUint32(FILE* fptr, uint32_t* ret);
int readUint16(FILE* fptr, uint16_t* ret);
int readUint8(FILE* fptr, uint8_t* ret);
int readString(FILE* fptr, char* buffer, int len);
void writeString(FILE* fptr, const char* string);


#endif