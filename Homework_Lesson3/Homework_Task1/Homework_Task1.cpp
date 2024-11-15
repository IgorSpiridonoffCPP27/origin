#include <iostream>
#include <fstream>
#include <string>

using namespace std;

class Adress {
    string city;
    string ylica;
    int number_home;
    int number_kv;
public:
    Adress(string city="", string ylica="", int number_home=1, int number_kv=1) {
        this->city = city;
        this->ylica = ylica;
        this->number_home = number_home;
        this->number_kv = number_kv;
    }
    string get_output_address() {
        return city+", " + ylica + ", " + to_string(number_home) + ", " + to_string(number_kv) + '\n';
    }
};

int main()
{
    setlocale(LC_ALL, "Russian");

    ifstream file ("in.txt");
    ofstream file2("out.txt");
    if (file.is_open()) {
        int N;
        file >> N;
        string city;
        string ylica;
        int number_home;
        int number_kv;
        Adress* obj = new Adress[N];
        for (int i = 0;i < N;i++) {
            file >> city;
            file >> ylica;
            file >> number_home;
            file >> number_kv;
            obj[i] = Adress(city, ylica, number_home, number_kv);
        }
        file2 << N << endl;
        for (int i = N-1;i >=0;i--) {
            
            file2<< obj[i].get_output_address();
        }
        delete[] obj;
    }
    else {
        cout << "Ошибка";
    }
    file.close();
}

