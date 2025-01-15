#include "Romb.h"
#include "EXception.h"
Romb::Romb(int a, int b, int c, int d, int A, int B, int C, int D) :Chetirehygolnic(a, b, c, d, A, B, C, D) {
    if (a != c && c != b && b != d || A != C && B != D) {
        throw Romba();
    }
    name = "Ромб";
}