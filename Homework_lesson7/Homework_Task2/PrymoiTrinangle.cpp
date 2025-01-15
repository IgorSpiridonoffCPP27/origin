#include "PrymoiTrinangle.h"
#include "EXception.h"
PrymoiTrinangle::PrymoiTrinangle(int a, int b, int c, int A, int B, int C) :Trinangle(a, b, c, A, B, 90) {
    if (C != 90) {
        throw AngleC90();
    }
    name = "Прямоугольный треугольник";
}