#include <mantissa.h>
#include <cstring>
#include <cstdint>
#include <iostream>
#include <bit>

int main() {
    binary32 number0{-4.20f};
    binary32 number1{-10.0f};
    binary32 product = number0 * number1;
    float fproduct = product;
    float expected = -4.20f * -10.0f;
    (std::cout << std::bitset<32>(product.representation)) << '\n';
    (std::cout << std::bitset<32>(*reinterpret_cast<uint32_t*>(&expected))) << '\n';
    MANTISSA_VALIDATE(fproduct == (-4.20f * -10.0f));
}
