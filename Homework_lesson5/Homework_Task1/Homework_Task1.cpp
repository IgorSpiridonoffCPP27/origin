
#include <iostream>
#include "Matematic.h"
using namespace std;

int main()
{
    setlocale(LC_ALL,"ru");
    int a, b,c;
    cout << "Введите первое число: ";
    cin >> a;
    cout << endl << "Введите первое число: ";
    cin>> b;
    cout << endl << "Выберите операцию (1 - сложение, 2 вычитание, 3 - умножение, 4 - деление, 5 - возведение в степень):";
    cin >> c;

    switch (c) {
    case 1: {
        cout<<sloshenie(a, b);
        break;
    }
    case 2: {
        cout << vishitanie(a, b);
        break;
    }
    case 3: {
        cout << ymnoshit(a, b);
        break;
    }
    case 4: {
        cout << razdelit(a, b);
        break;
    }
    case 5: {
        cout << stepen(a, b);
        break;
    }
    
    }

}
