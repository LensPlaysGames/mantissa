#include <mantissa.h>

int main() {
    const float in = 4.2f;
    binary32 number{in};
    float out = number;
    MANTISSA_VALIDATE(out == in);
}
