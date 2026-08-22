#include <utils.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>



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

uint32_t uint32Min(uint32_t a, uint32_t b) {
    return (a<b)?a:b;
}

float floatClip(float val, float min, float max) {
    if (val>max) return max;
    if (val<min) return min;
    return val;
}

int intClip(int val, int min, int max) {
    if (val>max) return max;
    if (val<min) return min;
    return val;
}

char* concatenateStrings(const char* s1, const char* s2) {
    int l1=strlen(s1), l2=strlen(s2);
    char* str = malloc((l1+l2+1)*sizeof(char));
    if (!str) return NULL;
    memcpy(str, s1, l1);
    memcpy(str + l1, s2, l2);
    str[l1+l2] = 0;
    return str;
}

static int allowedCharsInFileName(char c) {
    if (c>='A' && c<='Z') return 1;
    if (c>='a' && c<='z') return 1;
    if (c>='0' && c<='9') return 1;
    if (c=='-' || c=='_' || c=='+' || c=='=' || c=='.' || c==',') return 1;
    return 0;
}

char* stringToFileName(const char* str, int max) {
    int len = strlen(str);
    int chars=0;
    for (int i=0; i<len; i++) chars+=allowedCharsInFileName(str[i]);

    if (chars>max) chars=max;

    char* ret = malloc((chars+1)*sizeof(char));
    if (!ret) return NULL;

    int i=0, j=0;
    while (j<chars) {
        if (allowedCharsInFileName(str[i])) ret[j++]=str[i];
        i++;
    }
    ret[chars]=0;
    return ret;
}