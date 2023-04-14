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
    {
        binary32 foo{4.2f};
        binary32 bar{-2.1f};
        std::cout << foo.ascii_scientific() << '\n';
        std::cout << bar.ascii_scientific() << '\n';
        binary32 sum = foo + bar;
        std::cout << "sum: " << sum.ascii_scientific() << '\n';

        float fsum = sum;
        std::cout << "sum: " << fsum << '\n';
    }
    /*
    {
        binary32 foo{4.2f};
        binary32 bar{2.1f};
        std::cout << foo.ascii_scientific() << '\n';
        std::cout << bar.ascii_scientific() << '\n';
        binary32 diff = foo - bar;
        std::cout << "diff: " << diff.ascii_scientific() << '\n';

        float fdiff = diff;
        std::cout << "diff: " << fdiff << '\n';
    }
    {
        binary32 foo{2.1f};
        binary32 bar{-2.1f};
        std::cout << foo.ascii_scientific() << '\n';
        std::cout << bar.ascii_scientific() << '\n';
        binary32 diff = foo - bar;
        std::cout << "diff: " << diff.ascii_scientific() << '\n';

        float fdiff = diff;
        std::cout << "diff: " << fdiff << '\n';
    }
    {
        binary32 foo{-2.1f};
        binary32 bar{2.1f};
        std::cout << foo.ascii_scientific() << '\n';
        std::cout << bar.ascii_scientific() << '\n';
        binary32 diff = foo - bar;
        std::cout << "diff: " << diff.ascii_scientific() << '\n';

        float fdiff = diff;
        std::cout << "diff: " << fdiff << '\n';
    }
    {
        binary32 foo{-4.2f};
        binary32 bar{-2.1f};
        std::cout << foo.ascii_scientific() << '\n';
        std::cout << bar.ascii_scientific() << '\n';
        binary32 diff = foo - bar;
        std::cout << "diff: " << diff.ascii_scientific() << '\n';

        float fdiff = diff;
        std::cout << "diff: " << fdiff << '\n';
    }
    */
    /*
    {
        binary32 foo{42.0f};
        binary32 bar{10.0f};
        std::cout << foo.ascii_scientific() << '\n';
        std::cout << bar.ascii_scientific() << '\n';
        binary32 product = foo * bar;
        std::cout << "product: " << product.ascii_scientific() << '\n';

        float fproduct = product;
        std::cout << "product: " << fproduct << '\n';
    }
    {
        binary32 foo{-42.0f};
        binary32 bar{10.0f};
        std::cout << foo.ascii_scientific() << '\n';
        std::cout << bar.ascii_scientific() << '\n';
        binary32 product = foo * bar;
        std::cout << "product: " << product.ascii_scientific() << '\n';

        float fproduct = product;
        std::cout << "product: " << fproduct << '\n';
    }
    {
        binary32 foo{42.0f};
        binary32 bar{-10.0f};
        std::cout << foo.ascii_scientific() << '\n';
        std::cout << bar.ascii_scientific() << '\n';
        binary32 product = foo * bar;
        std::cout << "product: " << product.ascii_scientific() << '\n';

        float fproduct = product;
        std::cout << "product: " << fproduct << '\n';
    }
    {
        binary32 foo{-42.0f};
        binary32 bar{-10.0f};
        std::cout << foo.ascii_scientific() << '\n';
        std::cout << bar.ascii_scientific() << '\n';
        binary32 product = foo * bar;
        std::cout << "product: " << product.ascii_scientific() << '\n';

        float fproduct = product;
        std::cout << "product: " << fproduct << '\n';
    }
    */
    return 0;
}
