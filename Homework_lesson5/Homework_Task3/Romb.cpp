#include "Romb.h"
Romb::Romb(int a, int b, int c, int d, int A, int B, int C, int D) :Chetirehygolnic(a, b, c, d, A, B, C, D) {
    if (a != c && c != b && b != d || A != C && B != D) {
        cout << "Ошибка в равенстве всех сторон. Ошибка в равенстве углов A и C, B и D. Углы изменены. Стороны изменены" << endl;
    }
    name = "Ромб";
}