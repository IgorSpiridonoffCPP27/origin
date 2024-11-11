
#include <iostream>
using namespace std;

enum month {
    январь=1,
    февраль,
    март,
    апрель,
    май,
    июнь,
    июль,
    август,
    сентябрь,
    октябрь,
    ноябрь,
    декабрь
};
int main()
{
    setlocale(LC_ALL, "Russian");
    
    int num;

    cout << "Введите номер месяца: ";
    cin >> num;
    while (num != 0) {
        switch (num)
        {
        case static_cast<int>(month::август):
                cout << "Август" << endl;
                cout << "Введите номер месяца: ";
                cin >> num;
                break;
        case static_cast<int>(month::январь):
            cout << "Январь" << endl;
            cout << "Введите номер месяца: ";
            cin >> num;
            break;
        case static_cast<int>(month::февраль):
            cout << "февраль" << endl;
            cout << "Введите номер месяца: ";
            cin >> num;
            break;
        case static_cast<int>(month::март):
            cout << "март" << endl;
            cout << "Введите номер месяца: ";
            cin >> num;
            break;
        case static_cast<int>(month::апрель):
            cout << "апрель" << endl;
            cout << "Введите номер месяца: ";
            cin >> num;
            break;
        case static_cast<int>(month::май):
            cout << "май" << endl;
            cout << "Введите номер месяца: ";
            cin >> num;
            break;
        case static_cast<int>(month::июнь):
            cout << "июнь" << endl;
            cout << "Введите номер месяца: ";
            cin >> num;
            break;
        case static_cast<int>(month::июль):
            cout << "июль" << endl;
            cout << "Введите номер месяца: ";
            cin >> num;
            break;
        case static_cast<int>(month::сентябрь):
            cout << "сентябрь" << endl;
            cout << "Введите номер месяца: ";
            cin >> num;
            break;
        case static_cast<int>(month::октябрь):
            cout << "октябрь" << endl;
            cout << "Введите номер месяца: ";
            cin >> num;
            break;
        case static_cast<int>(month::ноябрь):
            cout << "ноябрь" << endl;
            cout << "Введите номер месяца: ";
            cin >> num;
            break;
        case static_cast<int>(month::декабрь):
            cout << "декабрь" << endl;
            cout << "Введите номер месяца: ";
            cin >> num;
            break;
        default:
            cout << "Неправильный номер!" << endl;
            cout << "Введите номер месяца: ";
            cin >> num;
            break;
        }
    }
    cout << "До свидания" << endl;
}

