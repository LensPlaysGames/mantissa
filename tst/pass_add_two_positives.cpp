#include <mantissa.h>

int main() {
    binary32 number0{2.1f};
    binary32 number1{2.1f};
    binary32 sum = number0 + number1;
    float fsum = sum;
    MANTISSA_VALIDATE(fsum == 4.2f);
}
