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
    constexpr FloatImpl(float f) : representation(*reinterpret_cast<Repr*>(&f)) {}
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

    /// Return true iff the exponent has every single one bit set.
    constexpr bool exponent_ones() const {
        return (representation & exponent_mask) == exponent_mask;
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

    constexpr void set_zero(bool isNegative = false) {
        representation = 0;
        set_negative(isNegative);
    }

    /// Will update both exponent and mantissa to ensure that there
    /// *is* an implicit leading one, if needed.
    constexpr void set_mantissa_normalised(Repr new_mantissa) {
        SignedRepr new_exponent = exponent();
        // If there are bits set above the implicit one in the given
        // mantissa, shift it to the right until the highest bit is the
        // implicit leading one that won't end up being stored.
        while (((new_mantissa & ~mantissa_mask) >> exponent_bit) > 1) {
            ++new_exponent;
            new_mantissa >>= 1;
        }
        // If the new mantissa is non-zero and...
        if (new_mantissa) {
            // If the implicit leading one *isn't* set, shift it to the
            // left until the leading one is set.
            while (!(new_mantissa & ~mantissa_mask)) {
                --new_exponent;
                new_mantissa <<= 1;
            }
        }
        set_exponent(new_exponent);
        set_mantissa(new_mantissa);
    }

    constexpr bool is_infinity() const {
        return exponent_ones() && !mantissa();
    }
    constexpr bool is_nan() const {
        return exponent_ones() && mantissa();
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
        if (!exponent()) {
            if (!mantissa()) {
                out += '0';
                return out;
            }
            // TODO: handle subnormal numbers (0.mantissa * 2^-126)
            out += "handle subnormal numbers";
            return out;
        }
        // Every bit set in the exponent means infinity or NaN.
        else if (exponent_ones()) {
            if (!mantissa()) {
                out += "inf";
                return out;
            }
            out += "NaN";
            return out;
        }
        out += "1.";
        out += mantissa_string();
        out += "x2^";
        out += std::to_string(exponent());
        return out;
    }

    constexpr void add(FloatImpl rhs) {
        // a + -b  =  a - b
        if (rhs.negative()) {
            sub({false, rhs.exponent(), rhs.mantissa()});
            return;
        }

        // From this point on, RHS is *not* negative.

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

        SignedRepr left_mantissa = get_left_mantissa();
        if (negative()) left_mantissa *= -1;
        SignedRepr right_mantissa = get_right_mantissa();
        SignedRepr new_mantissa = left_mantissa + right_mantissa;
        if (new_mantissa == 0) {
            set_zero();
            return;
        }
        if (new_mantissa < 0) {
            isNegative = true;
            new_mantissa *= -1;
        }
        set_negative(isNegative);
        set_mantissa_normalised(new_mantissa);
    }

    constexpr FloatImpl operator+(FloatImpl rhs) const {
        rhs.add(*this);
        return rhs;
    }

    constexpr void sub(FloatImpl rhs) {
        // a - -b  =  a + b
        if (rhs.negative())
            return add({false, rhs.exponent(), rhs.mantissa()});

        // From this point on, RHS is *not* negative.

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

        SignedRepr left_mantissa = get_left_mantissa();
        if (negative()) left_mantissa *= -1;
        SignedRepr right_mantissa = get_right_mantissa();
        SignedRepr new_mantissa = left_mantissa - right_mantissa;
        if ((new_mantissa & mantissa_mask) == 0) {
            set_zero();
            return;
        }
        bool isNegative = false;
        if (new_mantissa < 0) {
            isNegative = true;
            new_mantissa *= -1;
        }
        set_negative(isNegative);
        set_mantissa_normalised(new_mantissa);
    }

    constexpr FloatImpl operator-(FloatImpl rhs) const {
        FloatImpl lhs = *this;
        lhs.sub(rhs);
        return lhs;
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
