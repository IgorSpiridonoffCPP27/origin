#include "Parallelogram.h"
Parallelogram::Parallelogram(int a, int b, int c, int d, int A, int B, int C, int D) :Chetirehygolnic(a, a, b, b, A, A, B, B) {
    if (a != c && b != d || A != C && B != D) {
        cout << "Ошибка в равенстве углов A и C, B и D. Ошибка в равенстве сторон а != c и b!=d. Углы изменены. Стороны изменены" << endl;
    }
    name = "Параллелограмм";
}