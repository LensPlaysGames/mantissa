#include <mantissa.h>

int main() {
    binary32 number{4.2f};
    MANTISSA_VALIDATE(float(number) == 4.2f);
}
