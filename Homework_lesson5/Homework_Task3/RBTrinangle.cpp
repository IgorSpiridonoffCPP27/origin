#include "RBTrinangle.h"
RBTrinangle::RBTrinangle(int a, int b, int c, int A, int B, int C) :Trinangle(a, b, a, A, B, C) {
    if (a != c) {
        cout << "Ошибка в равенстве сторон a и c. Изменение сторона c стала равной a " << endl;
    }
    name = "Равнобедренный треугольник";
}