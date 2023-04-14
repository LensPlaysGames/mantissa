#include <mantissa.h>

int main() {
    binary32 foo{-42.0f};
    binary32 bar{-10.0f};
    std::cout << foo.ascii_scientific() << '\n';
    std::cout << bar.ascii_scientific() << '\n';
    binary32 product = foo * bar;
    std::cout << "product: " << product.ascii_scientific() << '\n';

    float fproduct = product;
    std::cout << "product: " << fproduct << '\n';
}
