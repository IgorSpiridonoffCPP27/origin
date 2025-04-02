#include <iostream>
#include <vector>
#include <algorithm>

#include <cstring>
using namespace std;


class big_integer {
	
	char* str_num;
public:
	big_integer(const char* str) {
		size_t len = strlen(str);
		str_num = new char[len + 1];  

		strcpy_s(str_num, len + 1, str);
		
		

		
	}
	~big_integer(){
		delete[] str_num;
	}

	big_integer(const big_integer& other) : big_integer(other.str_num) {

	}
	big_integer(big_integer&& other) noexcept{
		str_num = other.str_num;
		
		other.str_num = nullptr;
	}
	big_integer& operator=(const big_integer& other) {
		if (&other != this) {
			delete[] str_num;
			big_integer(other.str_num);
		}
		return *this;
	}
	big_integer& operator=(big_integer&& other)noexcept {
		if (&other != this) {
			delete[] str_num;
			str_num = other.str_num;
			delete other.str_num;
		}
		return *this;
	}

	void printclass() const{
		for (int i = (strlen(str_num) - 1);i >= 0;i--) {
			cout << str_num[i];
		}
	}
	
	std::string operator+(const big_integer other)const {
		int razryd = 0;
		std::vector<int> num_str;
		
		if (strlen(str_num) > strlen(other.str_num)) {
			int razn_len = strlen(str_num) - 1+ (strlen(other.str_num) - 1);
			for (int i = (strlen(other.str_num) - 1);i >= 0;i--) {
				if (i == 0) {
					razn_len = razn_len - i - 1 ;
				}
				else{
					razn_len = razn_len - i;
				}
				num_str.push_back(((static_cast<int>(str_num[razn_len])-48) + (static_cast<int>(other.str_num[i]) - 48))%10 + razryd);
				razryd = ((static_cast<int>(str_num[razn_len]) - 48) + (static_cast<int>(other.str_num[i]) - 48)) / 10;
			}
			for (int j = razn_len-1; j >= 0;j--) {
				if (razryd != 0) {
					num_str.push_back(((static_cast<int>(str_num[j]) - 48)+ razryd)%10);
					razryd = ((static_cast<int>(str_num[j]) - 48) + razryd) / 10;
				}
				else {
					razryd = 0;
					num_str.push_back((static_cast<int>(str_num[j]) - 48));
				}
			}
			string s="";
			for (auto el = num_str.rbegin(); el != num_str.rend(); el++) {
				s = s + static_cast<char>(*el+48);
			}
			return s;

		}
		else {
			int razn_len = strlen(other.str_num) - 1 + (strlen(str_num) - 1);
			for (int i = (strlen(str_num) - 1);i >= 0;i--) {
				if (i == 0) {
					razn_len = razn_len - i - 1;
				}
				else {
					razn_len = razn_len - i;
				}
				num_str.push_back(((static_cast<int>(other.str_num[razn_len]) - 48) + (static_cast<int>(str_num[i]) - 48)) % 10 + razryd);
				razryd = ((static_cast<int>(other.str_num[razn_len]) - 48) + (static_cast<int>(str_num[i]) - 48)) / 10;
			}
			for (int j = razn_len - 1; j >= 0;j--) {
				if (razryd != 0) {
					num_str.push_back(((static_cast<int>(other.str_num[j]) - 48) + razryd) % 10);
					razryd = ((static_cast<int>(other.str_num[j]) - 48) + razryd) / 10;
				}
				else {
					razryd = 0;
					num_str.push_back((static_cast<int>(other.str_num[j]) - 48));
				}
			}
			string s = "";
			for (auto el = num_str.rbegin(); el != num_str.rend(); el++) {
				s = s + static_cast<char>(*el + 48);
			}
			return s;
		}

	}
	std::string operator*(const big_integer other)const {
		string a = str_num;
		string b = other.str_num;

		int len_a = a.length(), len_b = b.length();
		vector<int> result(len_a + len_b, 0);

		
		for (int i = len_a - 1; i >= 0; --i) {
			for (int j = len_b - 1; j >= 0; --j) {
				int mul = (a[i] - '0') * (b[j] - '0');
				int pos_low = i + j + 1;
				int pos_high = i + j;
				mul += result[pos_low];  

				result[pos_low] = mul % 10;
				result[pos_high] += mul / 10;
			}
		}

		
		string product;
		bool leading_zero = true;
		for (int num : result) {
			if (leading_zero && num == 0) continue;
			leading_zero = false;
			product += (num + '0');
		}

		return product.c_str();

	}
	std::string operator*(int other)const {
		string a = str_num;
		if (other == 0) {
			return "0";
		}

		string b = "";
		while (other != 0) {
			b = b + static_cast<char>(other % 10+48);
			other = other / 10;
		}

		int len_a = a.length(), len_b = b.length();
		vector<int> result(len_a + len_b, 0);


		for (int i = len_a - 1; i >= 0; --i) {
			for (int j = len_b - 1; j >= 0; --j) {
				int mul = (a[i] - '0') * (b[j] - '0');
				int pos_low = i + j + 1;
				int pos_high = i + j;
				mul += result[pos_low];

				result[pos_low] = mul % 10;
				result[pos_high] += mul / 10;
			}
		}


		string product;
		bool leading_zero = true;
		for (int num : result) {
			if (leading_zero && num == 0) continue;
			leading_zero = false;
			product += (num + '0');
		}

		return product.c_str();

	}
	

};



int main()
{

	auto num1 = big_integer("567");
	auto num2 = big_integer("1234567");

	auto num3 = num1 + num2;
	auto num4 = num1 * num2;
	auto num5 = num1 * 2;


	cout << num3 << endl;
	cout << num4 << endl;
	cout << num5 << endl;

	return 0;
}
