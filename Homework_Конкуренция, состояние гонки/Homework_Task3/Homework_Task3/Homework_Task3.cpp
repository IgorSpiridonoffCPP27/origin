#include<chrono>
#include<iostream>
#include<vector>
#include<mutex>

#define Reales 2 //1 - lock ; 2 - unique_lock ; 3 - scoped_lock

using namespace std;

class Data {
int value;
public:
    
    Data(int val) {
        a = vector<int>(5, val);
    }
    vector<int> a ;

    mutex mtx;



};

void Swap(Data& Obj1,Data& Obj2) {


#if Reales == 1
    lock(Obj1.mtx, Obj2.mtx);
    lock_guard<mutex>(Obj1.mtx, adopt_lock);
    lock_guard<mutex>(Obj2.mtx, adopt_lock);
    
    
#elif Reales ==2
    
    unique_lock<mutex>lk1{ Obj1.mtx, defer_lock };
    unique_lock<mutex>lk2{ Obj2.mtx, defer_lock };
    lock(lk1, lk2);
#else
    scoped_lock lks{ Obj1.mtx , Obj2.mtx };
#endif

    Obj1.a.swap(Obj2.a);
    
}

void el() {

}

int main()
{
    setlocale(LC_ALL, "ru");
    Data obj1(6);
    Data obj2(7);

    cout << "Вектор 1: ";
    for (auto i : obj1.a) {
        cout << i << ' ';
    }
    cout << "Вектор 2: ";
    for (auto i : obj2.a) {
        cout << i << ' ';
    }
    cout << endl;
    thread t1 (Swap,ref(obj1), ref(obj2));
    t1.join();
    cout << "Поменял 1 раз" << endl;
    cout << "Вектор 1: ";
    for (auto i : obj1.a) {
        cout << i << ' ';
    }
    cout << "Вектор 2: ";
    for (auto i : obj2.a) {
        cout << i << ' ';
    }
    cout << endl;
    cout << "Поменял 2 раз" << endl;
    thread t2(Swap, ref(obj1), ref(obj2));
    t2.join();
    cout << "Вектор 1: ";
    for (auto i : obj1.a) {
        cout << i << ' ';
    }
    cout << "Вектор 2: ";
    for (auto i : obj2.a) {
        cout << i << ' ';
    }
    

}

