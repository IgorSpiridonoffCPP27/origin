#include <iostream>
#include <vector>
#include <algorithm>

class big_integer {
private:
    std::vector<int> digits; 

public:
    
    big_integer(const std::string& str) {
        for (auto it = str.rbegin(); it != str.rend(); ++it) {
            digits.push_back(*it - '0'); 
        }
    }

    
    big_integer() {}

    
    big_integer(big_integer&& other) noexcept : digits(std::move(other.digits)) {}

    
    big_integer& operator=(big_integer&& other) noexcept {
        if (this != &other) {
            digits = std::move(other.digits);
        }
        return *this;
    }

    
    big_integer operator+(const big_integer& other) const {
        big_integer result;
        result.digits.clear();

        int carry = 0;
        size_t maxSize = std::max(digits.size(), other.digits.size());

        for (size_t i = 0; i < maxSize || carry; ++i) {
            int sum = carry;
            if (i < digits.size()) sum += digits[i];
            if (i < other.digits.size()) sum += other.digits[i];

            result.digits.push_back(sum % 10);
            carry = sum / 10;
        }

        return result;
    }

    
    big_integer operator*(int num) const {
        big_integer result;
        result.digits.clear();

        int carry = 0;
        for (size_t i = 0; i < digits.size() || carry; ++i) {
            long long cur = carry + (i < digits.size() ? digits[i] * num : 0);
            result.digits.push_back(cur % 10);
            carry = cur / 10;
        }

        return result;
    }

   
    big_integer operator*(const big_integer& other) const {
        big_integer result("0");
        for (size_t i = 0; i < other.digits.size(); ++i) {
            big_integer temp = (*this * other.digits[i]); 
            temp.digits.insert(temp.digits.begin(), i, 0); 
            result = result + temp;
        }
        return result;
    }

    friend std::ostream& operator<<(std::ostream& os, const big_integer& num) {
        if (num.digits.empty()) {
            os << "0";
        }
        else {
            for (auto it = num.digits.rbegin(); it != num.digits.rend(); ++it) {
                os << *it;
            }
        }
        return os;
    }
};


int main() {
    setlocale(LC_ALL,"ru");
    big_integer number1("114575");
    big_integer number2("78524");

    auto result1 = number1 + number2;
    std::cout << "Сумма: " << result1 << std::endl; 

    auto result2 = number1 * 2;
    std::cout << "Умножение на число: " << result2 << std::endl; 

    auto result3 = number1 * number2;
    std::cout << "Умножение на другой обьект: " << result3 << std::endl; 

    return 0;
}
