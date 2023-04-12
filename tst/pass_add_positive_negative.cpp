#include <mantissa.h>

int main() {
    binary32 number0{2.1f};
    binary32 number1{-4.2f};
    binary32 sum = number0 + number1;
    float fsum = sum;
    std::cout << sum.ascii_scientific() << '\n';
    MANTISSA_VALIDATE(fsum == -2.1f);
}
