#include <stdio.h>
#include <chords.h>
#include <stdlib.h>



static void testIntensity(float intensity[12]) {
    struct chord result = getChordFromIntensityArray(intensity);
    char* str = getChordCompactName(result.type, result.base);
    printf("[Result] chord: %s, score: %.5f\n", str, result.score);

    free(str);
}


static void test1() {
    float intensity[12] = {0, 0.6, 0.1, 0.05, 0.9, 0, 0.1, 0.1, 0.0, 0.9, 0.0, 0.1};       // A Major
    testIntensity(intensity);
}

static void test2() {
    float intensity[12] = {0.6, 0.1, 0.02, 0.9, 0.0, 0.1, 0.0, 0.1, 0.8, 0.0, 0.0, 0.0};       // G# Major
    testIntensity(intensity);
}


static void test3() {
    float intensity[12] = {0.6, 0.1, 0.02, 0.9, 0.0, 0.1, 0.0, 0.7, 0.1, 0.0, 0.0, 0.0};       // C Minor
    testIntensity(intensity);
}

static void test4() {
    float intensity[12] = {0.08, 0.1, 0.75, 0.0, 0.05, 0.8, 0.0, 0.12, 0.04, 0.0, 0.0, 0.8};       // B Diminished
    testIntensity(intensity);
}

static void test5() {
    float intensity[12] = {0.08, 0.75, 0.0, 0.1, 0.79, 0.08, 0.1, 0.81, 0.04, 0.08, 0.74, 0.02};       // C# Fully Diminished
    testIntensity(intensity);
}

static void test6() {
    float intensity[12] = {0.08, 0.74, 0.02, 0.08, 0.75, 0.0, 0.1, 0.79, 0.08, 0.1, 0.81, 0.04};       // E Fully Diminished
    testIntensity(intensity);
}

static void test7() {
    float intensity[12] = {0.08, 0.8, 0.02, 0.0, 0.08, 0.0, 0.78, 0.0, 0.74, 0.0, 0.1, 0.04};       // C# Suspended 4
    testIntensity(intensity);
}


int main() {

    test1();
    test2();
    test3();
    test4();
    test5();
    test6();
    test7();

    return 0;
}