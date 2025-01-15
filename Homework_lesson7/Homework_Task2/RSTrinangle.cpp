#include "RSTrinangle.h"
#include "EXception.h"
RSTrinangle::RSTrinangle(int a, int b, int c, int A, int B, int C) :Trinangle(a, a, a, 60, 60, 60) {
    if (a != c && b != c || A != 60 && B != 60 && C != 60) {
        throw RS();
    }
    name = "Равностороний треугольник";
}