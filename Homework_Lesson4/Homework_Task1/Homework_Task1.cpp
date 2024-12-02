

#include <iostream>

using namespace std;

class Figure {
protected:
    int count;
    string name;
public:
    Figure() {
        count = 0;
        name = "Фигура";
    }
    virtual void get_info() {
        cout << name << ":" << count << endl;
    }


};

class Trinangle : public Figure {
public:
    Trinangle() {
        count = 3;
        name = "Треугольник";
    }
};

class Chetirehegolnic : public Figure {
public:
    Chetirehegolnic() {
        count = 4;
        name = "Четырехугольник";
    }
};

int main()
{
    setlocale(LC_ALL, "ru");
    Figure f;
    Trinangle T;
    Chetirehegolnic C;
    cout << "Количество сторон: "<<endl;
    f.get_info();
    T.get_info();
    C.get_info();
}
