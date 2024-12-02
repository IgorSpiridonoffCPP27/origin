

#include <iostream>

using namespace std;

class Figure {
protected:
    int info=0;
    string name = "Фигура";
public:
    virtual void get_info() {
        cout << name << ":" << info << endl;
    }


};

class Trinangle : public Figure {
public:
    Trinangle(int a) {
        info = a;
        name = "Треугольник";
    }
};

class Chetirehegolnic : public Figure {
public:
    Chetirehegolnic(int a) {
        info = a;
        name = "Четырехугольник";
    }
};

int main()
{
    setlocale(LC_ALL, "ru");
    Figure f;
    Trinangle T(3);
    Chetirehegolnic C(4);
    f.get_info();
    T.get_info();
    C.get_info();
}
