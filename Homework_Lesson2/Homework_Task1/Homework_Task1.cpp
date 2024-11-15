
#include <iostream>

using namespace std;

class Calculator {
    double num1, num2;
public:
    double add() {
        return num1 + num2;
    }
    double multiply() {
        return num1 * num2;
    }
    double subtract_1_2() {
        return num2- num1;
    }
    double subtract_2_1() {
        return num1 - num2;
    }
    double divide_1_2() {
        return num1 / num2;
    }
    double divide_2_1() {
        return num2 / num1;
    }
    bool set_num1(double num1) {
        this->num1 = num1;
        if (num1 != 0){
            return true;
        }
        else {
            return false;
        }
        
    }
    bool set_num2(double num2) {
        this->num2 = num2;
        if (num2 != 0) {
            return true;
        }
        else {
            return false;
        }
    }
};

int main()
{
    setlocale(LC_ALL, "Russian");
    while (true) {
        Calculator Object;
        cout << "Введите num1: ";
        int num1;
        cin >> num1;
        if (Object.set_num1(num1)) {
            repeat:
            cout << "Введите num2: ";
            int num2;
            cin >> num2;
            if (Object.set_num2(num2)) {
                cout << "num1 + num2 = " << Object.add() << endl;
                cout << "num1 - num2 = " << Object.subtract_2_1() << endl;
                cout << "num2 - num1 = " << Object.subtract_1_2() << endl;
                cout << "num1 * num2 = " << Object.multiply() << endl;
                cout << "num1 / num2 = " << Object.divide_1_2() << endl;
                cout << "num2 / num1 = " << Object.divide_2_1() << endl;
            }
            else {
                cout << "Неверный ввод!" << endl;
                goto repeat;
            }
        }
        else {
            cout << "Неверный ввод!" << endl;
        }
    }
}

