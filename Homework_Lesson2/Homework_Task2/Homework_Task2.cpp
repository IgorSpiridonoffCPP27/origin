
#include <iostream>
#include <string>
using namespace std;


class Counter {
    int count;
    
public:
    Counter(int count = 1) {
        this->count = count;
    }
    void add_count() {
        count++;
    }
    void minus_count() {
        count--;
    }
    void print_count() {
        cout << count << endl;
    }


};

int main()
{
    setlocale(LC_ALL, "Russian");

    string otvet;
    cout << "Вы хотите указать начальное значение счётчика? Введите yes или no: ";
    cin >> otvet;
    if (otvet == "yes") {
        cout << "Введите начальное значение счётчика: ";
        int start;
        cin >> start;
        Counter object(start);
        string comand;
        cout << "Введите команду ('+', '-', '=' или 'x'): ";
        cin >> comand;
        while (comand != "x") {
            if (comand == "+") {
                object.add_count();
            }
            if (comand == "-") {
                object.minus_count();
            }
            if (comand == "=") {
                object.print_count();
            }

            cout << "Введите команду ('+', '-', '=' или 'x'): ";
            cin >> comand;
        }
        cout << "До свидания!";
        
    }
    else {
        Counter object;
        string comand;
        cout << "Введите команду ('+', '-', '=' или 'x'): ";
        cin >> comand;
        while (comand != "x") {
            if (comand == "+") {
                object.add_count();
            }
            if (comand == "-") {
                object.minus_count();
            }
            if (comand == "=") {
                object.print_count();
            }
            cout << "Введите команду ('+', '-', '=' или 'x'): ";
            cin >> comand;
        }
        cout << "До свидания!";
    }
    
    

}
