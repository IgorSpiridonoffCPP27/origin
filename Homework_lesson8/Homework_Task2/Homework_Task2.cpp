#include <iostream>
using namespace std;
class Fraction
{
private:
	int numerator_;
	int denominator_;

public:
	Fraction(int numerator, int denominator)
	{
		numerator_ = numerator;
		denominator_ = denominator;
	}
	Fraction operator +(Fraction second) {
		if (denominator_ == second.denominator_) {
			return Fraction(numerator_+ second.numerator_, denominator_);
		}
		else {
			return Fraction(numerator_ * second.denominator_ + second.numerator_ * denominator_, denominator_ * second.denominator_);
		}
	}
	Fraction operator -(Fraction second) {
		if (denominator_ == second.denominator_) {
	
			return Fraction(numerator_ - second.numerator_, denominator_);
		}
		else {
			
			return Fraction(numerator_ * second.denominator_ - second.numerator_ * denominator_, denominator_ * second.denominator_);
		}
		
	}
	Fraction operator *(Fraction second) {
		
		return Fraction(numerator_ * second.numerator_, denominator_* second.denominator_);
	}
	Fraction operator /(Fraction second) {
		int c,b;
		c = second.denominator_;
		b = second.numerator_;
		
		return Fraction(numerator_ * c, denominator_ * b);
	}
	Fraction operator ++() {
		numerator_= numerator_+ denominator_;
		return *this;
	}
	Fraction operator --() {
		numerator_= numerator_ - denominator_;

		return *this;
	}
	Fraction operator ++(int) {
		Fraction temp = *this;
		++(*this);

		return temp;
	}
	Fraction operator --(int) {
		Fraction temp = *this;
		--(*this);

		return temp;
	}
	int getnumerator() {
		return numerator_;
	}
	int getdenominator() {
		return denominator_;
	}

};

int main()
{
	setlocale(LC_ALL, "ru");
	int chis1, chis2, znam1, znam2;
	cout << "Введите числитель дроби 1:";
	cin >> chis1;
	cout << endl;
	cout << "Введите знаменатель дроби 1:";
	cin >> znam1;
	cout << endl;
	cout << "Введите числитель дроби 2:";
	cin >> chis2;
	cout << endl;
	cout << "Введите знаменатель дроби 2:";
	cin >> znam2;
	cout << endl;
	Fraction f1(chis1, znam1);
	Fraction f2(chis2, znam2);
	Fraction f3 = (f1 + f2);
	cout << chis1 << "/" << znam1 << "+" << chis2 << "/" << znam2 << "=" << f3.getnumerator()<<"/"<< f3.getdenominator()<<endl;
	f3 = (f1 - f2);
	cout << chis1 << "/" << znam1 << "-" << chis2 << "/" << znam2 << "=" << f3.getnumerator() << "/" << f3.getdenominator() << endl;
	f3 = (f1 * f2);
	cout << chis1 << "/" << znam1 << "*" << chis2 << "/" << znam2 << "=" << f3.getnumerator() << "/" << f3.getdenominator() << endl;
	 f3 = (f1 / f2);
	cout << chis1 << "/" << znam1 << "/" << chis2 << "/" << znam2 << "=" << f3.getnumerator() << "/" << f3.getdenominator() << endl;
	f1 = (++f1);
	 f3 = (f1 * f2);
	cout << "++"<<chis1 << "/" << znam1 << "*" << chis2 << "/" << znam2 << "=" << f3.getnumerator() << "/" << f3.getdenominator() << endl;
	cout << "Значение дроби 1 = " << f1.getnumerator() << "/" << f1.getdenominator() << endl;
	 
	 f3 = ((f1--) * f2);
	cout << chis1 << "/" << znam1<<"--" << "*" << chis2 << "/" << znam2 << "=" << f3.getnumerator() << "/" << f3.getdenominator() << endl;
	cout << "Значение дроби 1 = " << f1.getnumerator() << "/" << f1.getdenominator() << endl;

	
	return 0;
}