#include <complex.h>
#include <math.h>




Complex complexProduct(Complex a, Complex b) {
    Complex ret = {0,0};
    ret.real = a.real*b.real-a.imaginary*b.imaginary;
    ret.imaginary = a.real*b.imaginary+b.real*a.imaginary;
    return ret;
}


Complex complexScalar(Complex a, float scalar) {
    return (Complex){scalar*a.real, scalar*a.imaginary};
}

Complex complexAddition(Complex a, Complex b) {
    return (Complex){a.real+b.real, a.imaginary+b.imaginary};
}

Complex complexSubtraction(Complex a, Complex b) {
    return (Complex){a.real-b.real, a.imaginary-b.imaginary};
}

float complexMagnitude(Complex a) {
    return (float)sqrt(a.real*a.real+a.imaginary*a.imaginary);
}