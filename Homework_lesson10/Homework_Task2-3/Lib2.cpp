#include "Lib2.h"

namespace Lib2 {
	using namespace std;
	std::string Leaver::leave(std::string str) {
		std::string s = "До свидания, " + str;
		return s;
	}
	
}