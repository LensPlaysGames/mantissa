#ifndef MANTISSA_MAIN_H
#define MANTISSA_MAIN_H

#include <bit>
#include <bitset>
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

    /// Return true iff the exponent has every single bit cleared.
    constexpr bool exponent_zeroes() const {
        return (representation & exponent_mask) == 0;
    }

    /// Return true iff the exponent has every single bit set.
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

    /// Set the float representation to signify a quiet NaN, either
    /// positive or negative.
    constexpr void set_not_a_number(bool isNegative = false) {
        // Set all bits in the exponent.
        representation = exponent_mask;
        // Make mantissa a non-zero value to indicate NaN vs infinity.
        representation |= mantissa_mask;
        // Clear top bit of mantissa as this is not a signalling NaN.
        representation &= ~(1 << (exponent_bit - 1));
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
    constexpr bool is_not_a_number() const {
        return exponent_ones() && mantissa();
    }
    constexpr bool is_zero() const {
        return exponent_zeroes() && !mantissa();
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
        if (exponent_zeroes()) {
            // Zero
            if (!mantissa()) {
                out += '0';
                return out;
            }
            // Subnormal
            out += "0.";
            out += mantissa_string();
            out += "x2^-126";
            return out;
        }
        // Every bit set in the exponent means infinity or NaN.
        else if (exponent_ones()) {
            // Infinity
            if (!mantissa()) {
                out += "inf";
                return out;
            }
            // Not a Number (NaN)
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
        /// X + 0 is still X.
        if (rhs.is_zero()) return;
        /// NaN + anything is still NaN.
        if (is_not_a_number()) return;
        /// infinity + anything is still infinity.
        if (is_infinity()) return;
        /// Anything + NaN is still NaN.
        /// Anything + infinity is still infinity.
        if (rhs.is_not_a_number() || rhs.is_infinity()) {
            *this = rhs;
            return;
        }

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
        /// X - 0 is still X.
        if (rhs.is_zero()) return;
        /// NaN - anything is still NaN.
        if (is_not_a_number()) return;
        /// infinity - anything is still infinity.
        if (is_infinity()) return;
        /// Anything - NaN is still NaN.
        /// Anything - infinity is still infinity.
        if (rhs.is_not_a_number() || rhs.is_infinity()) {
            *this = rhs;
            return;
        }

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

    void mul(FloatImpl rhs) {
        // LaTeX:
        // s_A.m_A.2^{{e}_A} \times s_A.m_A.2^{{e}_A} = (s_A \oplus s_B).m_A \times m_B.2^{(e_A + e_B) + n_{bias}}

        Repr left_mantissa = mantissa();
        Repr right_mantissa = rhs.mantissa();

        // The sign of the product is equal to the exclusive logical disjunction of both operand's signs.
        set_negative(negative() ^ rhs.negative());

        // The exponent of the product is equal to the sum of both operand's exponents.
        set_exponent(exponent() + rhs.exponent());

        // The mantissa of the product is equal to the multiplication of both operand's mantissas.

        // When multiplying, the product requires twice as much storage as the
        // operands; for a 24-bit mantissa, this would require 48 bits for the
        // product. Because things can get kind of weird here with bit numbers,
        // we just use two of the underlying representation for each half of
        // the bits. This is a software version of a very dumbed-down
        // Karatsuba-Urdhva multiplier, afaik.

        // Amount of mantissa bits over two (half)
        static constexpr Repr shift_amount = (1 + exponent_bit) / 2 + ((1 + exponent_bit) % 2);
        static constexpr Repr shift_mask = (Repr(1) << shift_amount) - 1;
        // Calculate product of lower half of mantissa.
        Repr low_mantissa = (left_mantissa & shift_mask) * (right_mantissa & shift_mask);
        // Calculate product of higher half of mantissa.
        Repr high_mantissa = (left_mantissa >> shift_amount) * (right_mantissa >> shift_amount);
        // All of the top mantissa bits are set from the low bits of the high mantissa;
        // the bottom bit would be set by the MSB of the low mantissa,
        // but we are doing ties-to-even rounding which means the
        // bottom bit of the mantissa will never be set, afaik.
        Repr new_mantissa = (high_mantissa << 1);
        set_mantissa_normalised(new_mantissa);
    }

    constexpr FloatImpl operator*(FloatImpl rhs) const {
        FloatImpl lhs = *this;
        lhs.mul(rhs);
        return lhs;
    }

    //constexpr FloatImpl operator/(FloatImpl rhs) const {
    //    FloatImpl lhs = *this;
    //    lhs.div(rhs);
    //    return lhs;
    //}
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
