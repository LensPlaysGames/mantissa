#include <mantissa.h>
#include <cstring>
#include <cstdint>
#include <iostream>
#include <bit>

int main() {
    float lhs = -4.20f;
    float rhs = -10.0f;
    binary32 number0{lhs};
    binary32 number1{rhs};
    binary32 product = number0 * number1;
    float fproduct = product;
    float expected = lhs * rhs;
    (std::cout << std::bitset<32>(product.representation)) << '\n';
    (std::cout << std::bitset<32>(*reinterpret_cast<uint32_t*>(&expected))) << '\n';
    MANTISSA_VALIDATE(fproduct == (lhs * rhs));
}
