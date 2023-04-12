#ifndef MANTISSA_MAIN_H
#define MANTISSA_MAIN_H

#include <bit>
#include <cstdint>
#include <iostream>
#include <string>
#include <version>

using u32 = uint32_t;
using s32 = int32_t;
using s8 = int8_t;

template<
    typename Repr,
    Repr sign_mask,
    Repr exponent_mask,
    Repr mantissa_mask,
    Repr sign_bit,
    Repr exponent_bit,
    Repr exponent_bias>
struct FloatImpl {
    using SignedRepr = std::make_signed_t<Repr>;
    static constexpr Repr mantissa_bit = 0;

    Repr representation{};

    constexpr FloatImpl() {}
    constexpr FloatImpl(Repr repr) : representation(repr) {}
#if __has_cpp_attribute(__cpp_lib_bit_cast)
    constexpr FloatImpl(float f) : representation(std::bit_cast<Repr>(f)) {}
    constexpr float() {
        return std::bit_cast<float>(representation);
    }
#else
    FloatImpl(float f) : representation(*reinterpret_cast<Repr*>(&f)) {}
    constexpr operator float() {
        return *reinterpret_cast<float*>(&representation);
    }
#endif
    constexpr FloatImpl(bool isNegative, SignedRepr exponent, Repr mantissa) {
        set(isNegative, exponent, mantissa);
    }

    constexpr bool negative() const {
        return representation & sign_mask;
    }

    constexpr void set_negative(bool isNegative) {
        representation &= ~sign_mask;
        if (isNegative) representation |= 1 << sign_bit;
    }

    constexpr SignedRepr exponent() const {
        return ((representation & exponent_mask) >> exponent_bit) - exponent_bias;
    }

    constexpr void set_exponent(SignedRepr exponent) {
        representation &= ~exponent_mask;
        representation |= ((Repr(exponent) + exponent_bias) << exponent_bit) & exponent_mask;
    }

    constexpr Repr mantissa_no_leading() const {
        return representation & mantissa_mask;
    }

    constexpr Repr mantissa() const {
        return mantissa_no_leading() | (1 << exponent_bit);
    }

    constexpr void set_mantissa(Repr mantissa) {
        representation &= ~mantissa_mask;
        representation |= mantissa & mantissa_mask;
    }

    constexpr void set(bool isNegative, SignedRepr exponent, Repr mantissa) {
        set_negative(isNegative);
        set_exponent(exponent);
        set_mantissa(mantissa);
    }

    std::string mantissa_string(Repr base = 10) const {
        Repr mtsa = mantissa_no_leading();
        std::string out;
        if (mtsa)
            while (mtsa) {
                mtsa *= base;
                out += '0' + ((mtsa & ~mantissa_mask) >> exponent_bit);
                mtsa &= mantissa_mask;
            }
        else return "0";
        return out;
    }

    std::string ascii_scientific() const {
        std::string out;
        if (negative()) out += '-';
        if ((representation & ~sign_mask) == 0) {
            out += '0';
            return out;
        }
        out += "1.";
        out += mantissa_string();
        out += "x2^";
        out += std::to_string(exponent());
        return out;
    }

    void add(FloatImpl rhs) {
        bool lhs_leading = true;
        auto get_left_mantissa = [&]() {
            if (lhs_leading) return mantissa();
            else return mantissa_no_leading();
        };
        bool rhs_leading = true;
        auto get_right_mantissa = [&]() {
            if (rhs_leading) return rhs.mantissa();
            else return rhs.mantissa_no_leading();
        };
        while (exponent() < rhs.exponent()) {;
            set_exponent(exponent() + 1);
            set_mantissa(get_left_mantissa() >> 1);
            lhs_leading = false;
        }
        while (rhs.exponent() < exponent()) {
            rhs.set_exponent(rhs.exponent() + 1);
            rhs.set_mantissa(get_right_mantissa() >> 1);
            rhs_leading = false;
        }

        bool isNegative = false;
        SignedRepr new_exponent = exponent();

        SignedRepr left_mantissa = get_left_mantissa();
        if (negative()) left_mantissa *= -1;

        SignedRepr right_mantissa = get_right_mantissa();
        if (rhs.negative()) right_mantissa *= -1;

        SignedRepr tmp_mantissa = left_mantissa + right_mantissa;
        if (tmp_mantissa == 0) set(false, 0, 0);
        if (tmp_mantissa < 0) {
            isNegative = true;
            tmp_mantissa *= -1;
        }
        Repr new_mantissa = tmp_mantissa;
        while (((new_mantissa & ~mantissa_mask) >> exponent_bit) > 1) {
            ++new_exponent;
            new_mantissa >>= 1;
        }

        set(isNegative, new_exponent, new_mantissa);
    }

    FloatImpl operator+(FloatImpl rhs) {
        rhs.add(*this);
        return rhs;
    }
};

using binary32 = FloatImpl<u32,
                           0b1000'0000'0000'0000'0000'0000'0000'0000,
                           0b0111'1111'1000'0000'0000'0000'0000'0000,
                           0b0000'0000'0111'1111'1111'1111'1111'1111,
                           31, 23, 127>;

/// Call this macro with the test condition you'd like to ensure from a
/// test's `main`.
#define MANTISSA_VALIDATE(cond) if (!(cond)) return -1; return 0

#endif // MANTISSA_MAIN_H
