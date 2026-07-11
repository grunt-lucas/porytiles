#pragma once

#include <array>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <istream>
#include <sstream>
#include <string>
#include <vector>

namespace porytiles {

/// @brief Computes the MD5 digest of an input stream.
class StreamDigest {
  public:
    /// @brief Computes the MD5 digest of an input stream.
    ///
    /// @details
    /// Reads the given input stream and computes the MD5 hash of its contents. The stream is read from its current
    /// position until the end. The stream's position is not restored after reading. The MD5 algorithm processes
    /// the data in 512-bit blocks and produces a 128-bit hash value represented as a 32-character hexadecimal string.
    ///
    /// @param stream The input stream to digest.
    /// @return A string representing the MD5 digest in lowercase hexadecimal format.
    [[nodiscard]] std::string digest(std::istream &stream) const
    {
        // Initialize MD5 state
        uint32_t a = 0x67452301;
        uint32_t b = 0xEFCDAB89;
        uint32_t c = 0x98BADCFE;
        uint32_t d = 0x10325476;

        // Read stream into buffer
        std::vector<uint8_t> data;
        char ch;
        while (stream.get(ch)) {
            data.push_back(static_cast<uint8_t>(ch));
        }

        // Apply padding
        const uint64_t original_bit_length = data.size() * 8;
        data.push_back(0x80); // Append '1' bit (plus zeros)

        // Pad with zeros until length ≡ 448 (mod 512)
        while ((data.size() % 64) != 56) {
            data.push_back(0x00);
        }

        // Append original length as 64-bit little-endian
        for (int i = 0; i < 8; ++i) {
            data.push_back((original_bit_length >> (i * 8)) & 0xFF);
        }

        // Process each 512-bit block
        for (size_t offset = 0; offset < data.size(); offset += 64) {
            process_block(data.data() + offset, a, b, c, d);
        }

        // Convert to hex string
        return to_hex_string(a, b, c, d);
    }

  private:
    // MD5 auxiliary functions
    static uint32_t f(uint32_t x, uint32_t y, uint32_t z)
    {
        return (x & y) | (~x & z);
    }
    static uint32_t g(uint32_t x, uint32_t y, uint32_t z)
    {
        return (x & z) | (y & ~z);
    }
    static uint32_t h(uint32_t x, uint32_t y, uint32_t z)
    {
        return x ^ y ^ z;
    }
    static uint32_t i(uint32_t x, uint32_t y, uint32_t z)
    {
        return y ^ (x | ~z);
    }

    // Left rotate
    static uint32_t rotate_left(uint32_t value, uint32_t shift)
    {
        return (value << shift) | (value >> (32 - shift));
    }

    // Process a single 512-bit block
    static void process_block(const uint8_t *block, uint32_t &a, uint32_t &b, uint32_t &c, uint32_t &d)
    {
        // Convert block to 32-bit words (little-endian)
        uint32_t x[16];
        for (int j = 0; j < 16; ++j) {
            x[j] = block[j * 4] | (block[j * 4 + 1] << 8) | (block[j * 4 + 2] << 16) | (block[j * 4 + 3] << 24);
        }

        // Save original state
        uint32_t aa = a;
        uint32_t bb = b;
        uint32_t cc = c;
        uint32_t dd = d;

        // Round 1
        for (int j = 0; j < 16; ++j) {
            uint32_t k_val = static_cast<uint32_t>(std::floor(std::abs(std::sin(j + 1)) * 4294967296.0));
            uint32_t f_val = f(b, c, d);
            uint32_t temp = a + f_val + x[j] + k_val;
            temp = rotate_left(temp, s_[j]);
            a = d;
            d = c;
            c = b;
            b = b + temp;
        }

        // Round 2
        for (int j = 0; j < 16; ++j) {
            uint32_t k_val = static_cast<uint32_t>(std::floor(std::abs(std::sin(j + 17)) * 4294967296.0));
            uint32_t g_val = g(b, c, d);
            uint32_t x_index = (1 + 5 * j) % 16;
            uint32_t temp = a + g_val + x[x_index] + k_val;
            temp = rotate_left(temp, s_[16 + j]);
            a = d;
            d = c;
            c = b;
            b = b + temp;
        }

        // Round 3
        for (int j = 0; j < 16; ++j) {
            uint32_t k_val = static_cast<uint32_t>(std::floor(std::abs(std::sin(j + 33)) * 4294967296.0));
            uint32_t h_val = h(b, c, d);
            uint32_t x_index = (5 + 3 * j) % 16;
            uint32_t temp = a + h_val + x[x_index] + k_val;
            temp = rotate_left(temp, s_[32 + j]);
            a = d;
            d = c;
            c = b;
            b = b + temp;
        }

        // Round 4
        for (int j = 0; j < 16; ++j) {
            uint32_t k_val = static_cast<uint32_t>(std::floor(std::abs(std::sin(j + 49)) * 4294967296.0));
            uint32_t i_val = i(b, c, d);
            uint32_t x_index = (7 * j) % 16;
            uint32_t temp = a + i_val + x[x_index] + k_val;
            temp = rotate_left(temp, s_[48 + j]);
            a = d;
            d = c;
            c = b;
            b = b + temp;
        }

        // Add this block's hash to result
        a += aa;
        b += bb;
        c += cc;
        d += dd;
    }

    // Convert final state to hex string
    [[nodiscard]] static std::string to_hex_string(uint32_t a, uint32_t b, uint32_t c, uint32_t d)
    {
        std::stringstream ss;
        ss << std::hex << std::setfill('0');

        // Output in little-endian byte order
        auto append_word = [&ss](uint32_t word) {
            for (int i = 0; i < 4; ++i) {
                ss << std::setw(2) << ((word >> (i * 8)) & 0xFF);
            }
        };

        append_word(a);
        append_word(b);
        append_word(c);
        append_word(d);

        return ss.str();
    }

    // Shift amounts for each round
    static constexpr std::array<uint32_t, 64> s_ = {
        // Round 1
        7,
        12,
        17,
        22,
        7,
        12,
        17,
        22,
        7,
        12,
        17,
        22,
        7,
        12,
        17,
        22,
        // Round 2
        5,
        9,
        14,
        20,
        5,
        9,
        14,
        20,
        5,
        9,
        14,
        20,
        5,
        9,
        14,
        20,
        // Round 3
        4,
        11,
        16,
        23,
        4,
        11,
        16,
        23,
        4,
        11,
        16,
        23,
        4,
        11,
        16,
        23,
        // Round 4
        6,
        10,
        15,
        21,
        6,
        10,
        15,
        21,
        6,
        10,
        15,
        21,
        6,
        10,
        15,
        21};
};

} // namespace porytiles
