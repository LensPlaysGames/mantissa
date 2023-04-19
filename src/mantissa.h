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
    Repr sign_bit,
    Repr exponent_bit,
    Repr exponent_bias>
struct FloatImpl {
    static constexpr size_t representation_bits = sizeof(Repr) * 8;
    static_assert(sign_bit < representation_bits, "Sign bit must fit within underlying representation.");
    static_assert(sign_bit > exponent_bit, "Sign bit must be a more significant bit than the exponent bit.");
    static_assert(exponent_bit, "Exponent bit must leave room for at least 1 LSB dedicated to mantissa.");
    static_assert(exponent_bit < representation_bits, "Exponent bit must fit within underlying representation.");

    using SignedRepr = std::make_signed_t<Repr>;
    static constexpr Repr sign_mask = (Repr(1) << sign_bit);
    static constexpr Repr exponent_mask = ((Repr(1) << (sign_bit - exponent_bit)) - 1) << exponent_bit;
    static constexpr Repr mantissa_bit = 0;
    static constexpr Repr mantissa_mask = (Repr(1) << exponent_bit) - 1;

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
        if (new_mantissa & mantissa_mask) {
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
        if (mtsa) {
            while (mtsa) {
                mtsa *= base;
                out += '0' + ((mtsa & ~mantissa_mask) >> exponent_bit);
                mtsa &= mantissa_mask;
            }
            return out;
        }
        return "0";
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
        /// Zero * anything is still zero.
        /// NaN * anything is still NaN.
        /// Infinity * anything is still infinity.
        if (is_zero() || is_not_a_number() || is_infinity()) return;
        /// Anything * zero is still zero.
        /// Anything * NaN is still NaN.
        /// Anything * infinity is still infinity.
        if (rhs.is_zero() || rhs.is_not_a_number() || rhs.is_infinity()) {
            *this = rhs;
            return;
        }

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

        // NOTE:
        // 4.20f * 10.0f == 42.0f but it actually equals 41.9922 with the following code...
        // 4.20f == 0b01000000100001100110011001100110
        //            = not signed
        //             ======== exponent of +2
        //                     ======================= mantissa of 00_0011_ repeating
        // 10.0f == 0b01000001001000000000000000000000
        //            = not signed
        //             ======== exponent of +3
        //                     ======================= mantissa of 01
        // 42.0f == 0b01000010001010000000000000000000
        //            = not signed
        //             ======== exponent of +5
        //                     ======================= mantissa of 0101
        // I am quite confusion.
        // 4.20f mantissa is 0b1.00001100110011001100110
        // 10.0f mantissa is 0b1.01000000000000000000000
        // When you multiply these binary numbers (including the implicit
        // leading one), you get 1.01001111111 (in binary still). How are we
        // meant to convert that into 1.01010000000? If there are a certain
        // amount of 1s in a row do we just turn them higher? I *must* be
        // missing something...

        // exponent_bit stores the first index after the mantissa, which can
        // also be thought of as the amount of mantissa bits stored in the
        // representation. The leading, hidden one + this number is equal to
        // the bit precision of this float format.
        static constexpr Repr bits_precision = (1 + exponent_bit);
        // Amount of mantissa bits over two (half)
        static constexpr Repr shift_amount_lo = bits_precision / 2 + (bits_precision % 2);
        static constexpr Repr shift_mask_lo = (Repr(1) << shift_amount_lo) - 1;
        static constexpr Repr shift_amount_hi = bits_precision / 2;
        static constexpr Repr shift_mask_hi = ((Repr(1) << shift_amount_hi) - 1) << shift_amount_lo;
        // Calculate product of lower half of mantissa.
        Repr low_mantissa = (left_mantissa & shift_mask_lo) * (right_mantissa & shift_mask_lo);
        std::cout << "lo * lo:\n"
                  << "              " << std::bitset<shift_amount_lo>(left_mantissa) << '\n'
                  << " *            " << std::bitset<shift_amount_lo>(right_mantissa) << '\n'
                  << " =" << std::bitset<exponent_bit + 1>(low_mantissa) << '\n';

        Repr lowhigh_mantissa = (left_mantissa & shift_mask_lo) * (right_mantissa >> shift_amount_lo);
        std::cout << "lo * hi:\n"
                  << "              " << std::bitset<shift_amount_lo>(left_mantissa) << '\n'
                  << " *            " << std::bitset<shift_amount_hi>(right_mantissa >> shift_amount_lo) << '\n'
                  << " =" << std::bitset<exponent_bit + 1>(lowhigh_mantissa) << '\n';

        Repr highlow_mantissa = (left_mantissa >> (shift_amount_lo - 1)) * (right_mantissa & shift_mask_lo);
        std::cout << "hi * lo:\n"
                  << "              " << std::bitset<shift_amount_hi>(left_mantissa >> shift_amount_lo) << '\n'
                  << " *            " << std::bitset<shift_amount_lo>(right_mantissa) << '\n'
                  << " =" << std::bitset<exponent_bit + 1>(highlow_mantissa) << '\n';

        // Calculate product of higher half of mantissa.
        Repr high_mantissa = (left_mantissa >> (shift_amount_lo - 1)) * (right_mantissa >> shift_amount_lo);
        std::cout << "hi * hi:\n"
                  << "              " << std::bitset<shift_amount_hi>(left_mantissa >> (shift_amount_lo - 1)) << '\n'
                  << " *            " << std::bitset<shift_amount_hi>(right_mantissa >> shift_amount_lo) << '\n'
                  << " =" << std::bitset<exponent_bit + 1>(high_mantissa) << '\n';

        // All of the top mantissa bits are set from the low bits of the high mantissa;
        // the bottom bit is set according to the rounding mode.
        // Currently we only support "round-to-nearest, ties-to-even".
        // That's exactly what it sounds like. If the low bits are less than
        // half of what they could be, than the bit rounds down (nearest). If
        // it's exactly half-way, round down (0 is even and 1 is not, and in
        // binary that means we can ever only round down).
        Repr new_mantissa = high_mantissa + (highlow_mantissa >> (shift_amount_lo - 1));
        if ((low_mantissa + (lowhigh_mantissa >> shift_amount_lo)) >> shift_amount_lo) new_mantissa |= 1;
        else new_mantissa &= ~Repr(1);

        set_mantissa_normalised(new_mantissa);
    }

    constexpr FloatImpl operator*(FloatImpl rhs) const {
        rhs.mul(*this);
        return rhs;
    }
};

using binary32 = FloatImpl<u32, 31, 23, 127>;

/// Call this macro with the test condition you'd like to ensure from a
/// test's `main`.
#define MANTISSA_VALIDATE(cond) if (!(cond)) return -1; return 0

#endif // MANTISSA_MAIN_H
