#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include <mutex>
#include <ctime>
#include <cstdlib>
#include <iomanip>
#include <windows.h>
using namespace std;



mutex mtx;

void MOVE(int number) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD pos = { 0, static_cast<SHORT>(number + 2) };
    SetConsoleCursorPosition(hConsole, pos);
}

void PRINT(int number,string symbol,int progress, thread::id id, double realtime = 0.0) {
    
    

    lock_guard<mutex> lock(mtx);
    MOVE(number);
    cout << number << " " << id <<" " << setw(2);
    
    for (int i = -1;i < progress;i++) {
        cout << symbol;
    }
    if (realtime > 0) {
        this_thread::sleep_for(50ms);
        cout << " Время : " << realtime;
    }
    else {
        for (int i = 0;i < 3;i++) {
            cout << ".";
            this_thread::sleep_for(50ms);
        }
    }
   
}

void Work(int NUM) {
    vector<string> a(7, "#");
    auto id = this_thread::get_id();
    auto start = chrono::high_resolution_clock::now();
    for (int i = 0;i < 7;i++) {
        PRINT(NUM, a[i],i,id);
    }
    auto end = chrono::high_resolution_clock::now();
    double time = chrono::duration<double>(end - start).count();
    PRINT(NUM, a[6], 6, id, time);
   
}


int main()
{
    
    setlocale(LC_ALL, "ru");
    int N;
    cout << "Введите количество потоков: ";
    cin >> N;
    vector<thread> TV;
    cout << "№ " << " id " << " progress bar " << "Time";

    for (int i = 0; i < N;i++) {
        TV.push_back(thread(Work,i));
    }

    for (int i = 0; i < N;i++) {
        TV[i].join();
    }

    
    MOVE(N + 2);
    

    return 0;
}