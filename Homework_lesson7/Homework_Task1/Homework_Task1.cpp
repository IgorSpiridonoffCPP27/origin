

#include <iostream>
#include <exception>
#include <string>
using namespace std;

int function(std::string str, int forbidden_length) {
    if (str.length() == forbidden_length) {
        throw exception("Вы ввели слово запретной длины! До свидания");
    }
    return 0;
}

int main()
{   
    setlocale(LC_ALL, "ru");
    int Lenght;
    string word;
    try {
        cout << "Введите запретную длину: ";
        cin >> Lenght;
        while (true) {
            cout << endl << "Введите слово: ";
            cin >> word;
            function(word, Lenght);
            cout << endl << "Длина слова \"" << word << "\" равна " << word.length()<<endl;

        }
    }
    catch (const exception& msg) {
        cout << msg.what();
    }
}

