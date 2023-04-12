#include <mantissa.h>

int main() {
    binary32 lhs{-4.2f};
    binary32 rhs{-2.1f};
    float out = float(lhs - rhs);
    MANTISSA_VALIDATE(out == -2.1f);
}
