#include "RBTrinangle.h"
#include "EXception.h"
RBTrinangle::RBTrinangle(int a, int b, int c, int A, int B, int C) :Trinangle(a, b, a, A, B, C) {
    if (a != c) {
        throw RB();
    }
    name = "Равнобедренный треугольник";
}