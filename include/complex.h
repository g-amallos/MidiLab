#ifndef COMPLEX_H
#define COMPLEX_H


typedef struct complex_number {
    float real;
    float imaginary;
} Complex;



Complex complexProduct(Complex a, Complex b);
Complex complexScalar(Complex a, float scalar);
Complex complexAddition(Complex a, Complex b);
Complex complexSubtraction(Complex a, Complex b);
float complexMagnitude(Complex a);


#endif