#include <mantissa.h>

int main() {
    binary32 foo{3.99999976158142089844f};
    binary32 bar{3.99999976158142089844f};
    std::cout << foo.ascii_scientific() << '\n';
    std::cout << bar.ascii_scientific() << '\n';
    binary32 product = foo * bar;
    std::cout << "product: " << product.ascii_scientific() << '\n';

    float fproduct = product;
    std::cout << "product: " << fproduct << '\n';

    float testproduct = 3.99999976158142089844f * 3.99999976158142089844f;
    std::cout << "actual floats: " << testproduct << '\n';

    MANTISSA_VALIDATE(fproduct == 16.0f);
}
