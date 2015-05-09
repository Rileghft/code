#include <iostream>
#include <string>
#include <cstdlib>
#include <stdexcept>
#include <exception>
 
int main()
{
    std::string str1 = "45";
    std::string str2 = "3.14159";
    std::string str3 = "31337 with words";
    std::string str4 = "words and 2";
 
    int myint1 = std::stoi(str1);
    int myint2 = std::stoi(str2);
    int myint3 = std::stoi(str3);
    int myint4 = 0;
    // error: 'std::invalid_argument'
    try {
        myint4 = std::stoi(str4);
    }
    catch(std::invalid_argument& e) {
        std::cout << "standard exception: " << e.what() << std::endl;
    }
                                    
    std::cout << "std::stoi(\"" << str1 << "\") is " << myint1 << '\n';
    std::cout << "std::stoi(\"" << str2 << "\") is " << myint2 << '\n';
    std::cout << "std::stoi(\"" << str3 << "\") is " << myint3 << '\n';
    std::cout << "std::stoi(\"" << str4 << "\") is " << myint4 << '\n';
}
