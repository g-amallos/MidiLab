#include <raylib.h>
#include <math.h>


double lerp(double a, double b, double t) {
    if (t<=0) return a;
    if (t>=1) return b;
    return a+(b-a)*t;
}

double trigInterpolation(double a, double b, double t) {
    t = sin(PI*0.5*t);
    t*=t;
    return lerp(a, b, t);
}

Color blendColors(Color a, Color b, double t) {
    Color ret;
    ret.r = (unsigned char)(lerp(a.r, b.r, t));
    ret.g = (unsigned char)(lerp(a.g, b.g, t));
    ret.b = (unsigned char)(lerp(a.b, b.b, t));
    ret.a = (unsigned char)(lerp(a.a, b.a, t));
    return ret;
}

Vector2 lerpVector2(Vector2 a, Vector2 b, double t) {
    return (Vector2){.x=lerp(a.x, b.x, t), .y=lerp(a.y, b.y, t)};
}

Vector2 lerpVector2_vec(Vector2 a, Vector2 b, Vector2 t) {
    return (Vector2){.x=lerp(a.x, b.x, t.x), .y=lerp(a.y, b.y, t.y)};
}

double doubleMax(double a, double b) {
    return (a>b)?a:b;
}

double doubleMin(double a, double b) {
    return (a<b)?a:b;
}

float floatMax(float a, float b) {
    return (a>b)?a:b;
}

float floatMin(float a, float b) {
    return (a<b)?a:b;
}