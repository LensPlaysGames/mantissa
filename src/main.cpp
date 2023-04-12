#include <iostream>
#include <mantissa.h>

int main() {
    {
        binary32 foo{2.1f};
        binary32 bar{2.1f};
        std::cout << foo.ascii_scientific() << '\n';
        std::cout << bar.ascii_scientific() << '\n';
        binary32 sum = foo + bar;
        std::cout << "sum: " << sum.ascii_scientific() << '\n';

        float fsum = sum;
        std:: cout << "sum: " << fsum << '\n';
    }
    {
        binary32 foo{2.1f};
        binary32 bar{8.1f};
        std::cout << foo.ascii_scientific() << '\n';
        std::cout << bar.ascii_scientific() << '\n';
        binary32 sum = foo + bar;
        std::cout << "sum: " << sum.ascii_scientific() << '\n';

        float fsum = sum;
        std:: cout << "sum: " << fsum << '\n';
    }
    {
        binary32 foo{-2.4f};
        binary32 bar{-2.6f};
        std::cout << foo.ascii_scientific() << '\n';
        std::cout << bar.ascii_scientific() << '\n';
        binary32 sum = foo + bar;
        std::cout << "sum: " << sum.ascii_scientific() << '\n';

        float fsum = sum;
        std::cout << "sum: " << fsum << '\n';
    }
    return 0;
}
