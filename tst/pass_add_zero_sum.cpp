#include <mantissa.h>

int main() {
    binary32 lhs{-1.0f};
    binary32 rhs{1.0f};
    float out = float(lhs + rhs);
    MANTISSA_VALIDATE(out == 0.0f);
}
