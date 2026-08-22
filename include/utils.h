#ifndef UTILITIES_H
#define UTILITIES_H
#include <raylib.h>
#include <stdint.h>




double lerp(double a, double b, double t);
double trigInterpolation(double a, double b, double t);

Color blendColors(Color a, Color b, double t);

Vector2 lerpVector2(Vector2 a, Vector2 b, double t);
Vector2 lerpVector2_vec(Vector2 a, Vector2 b, Vector2 t);

double doubleMax(double a, double b);
double doubleMin(double a, double b);
float floatMax(float a, float b);
float floatMin(float a, float b);
uint32_t uint32Min(uint32_t a, uint32_t b);
float floatClip(float val, float min, float max);
int intClip(int val, int min, int max);

char* concatenateStrings(const char* s1, const char* s2);
char* stringToFileName(const char* str, int max);


#endif