#include <mantissa.h>

int main() {
    binary32 number0{-42.0f};
    binary32 number1{10.0f};
    binary32 product = number0 * number1;
    float fproduct = product;
    MANTISSA_VALIDATE(fproduct == -420.0f);
}
