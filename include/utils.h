#ifndef UTILITIES_H
#define UTILITIES_H
#include <raylib.h>

double lerp(double a, double b, double t);
double trigInterpolation(double a, double b, double t);

Color blendColors(Color a, Color b, double t);

Vector2 lerpVector2(Vector2 a, Vector2 b, double t);
Vector2 lerpVector2_vec(Vector2 a, Vector2 b, Vector2 t);

double doubleMax(double a, double b);
double doubleMin(double a, double b);
float floatMax(float a, float b);
float floatMin(float a, float b);

#endif