#include <math.h>




float lerpFloat(float a, float b, float t) {
    if (t<=0.0) return a;
    if (t>=1.0) return b;
    return a+(b-a)*t;
}

float trigonometricInterpolationFloat(float a, float b, float t) {
    if (t<=0.0) return a;
    if (t>=1.0) return b;
    t = sinf(1.570796327*t);
    return a+(b-a)*t*t;
}