#include "PrymoiTrinangle.h"
PrymoiTrinangle::PrymoiTrinangle(int a, int b, int c, int A, int B, int C) :Trinangle(a, b, c, A, B, 90) {
    if (C != 90) {
        cout << "Ошибка в величине угла С. Угол С изменен на 90" << endl;
    }
    name = "Прямоугольный треугольник";
}