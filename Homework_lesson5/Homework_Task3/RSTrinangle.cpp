#include "RSTrinangle.h"
RSTrinangle::RSTrinangle(int a, int b, int c, int A, int B, int C) :Trinangle(a, a, a, 60, 60, 60) {
    if (a != c && b != c || A != 60 && B != 60 && C != 60) {
        cout << "Ошибка в равенстве сторон a b c. Ошибка в равенстве углов A B C - 60 градусам. Изменение в равенстве " << endl;
    }
    name = "Равностороний треугольник";
}