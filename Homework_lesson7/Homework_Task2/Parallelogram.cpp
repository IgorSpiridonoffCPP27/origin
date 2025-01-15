#include "Parallelogram.h"
#include "EXception.h"
Parallelogram::Parallelogram(int a, int b, int c, int d, int A, int B, int C, int D) :Chetirehygolnic(a, a, b, b, A, A, B, B) {
    if (a != c && b != d || A != C && B != D) {
        throw Parall();
    }
    name = "ֿאנאככוכמדנאלל";
}