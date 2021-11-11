#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <array>
#include <string>
#include <string_view>
#include <ranges>
#include <type_traits>

namespace ubn {
    namespace authlib::detail {
        enum crc_types { crc8_ccitt, crc8_itu, crc8_rohc, crc8_ebu, crc8_i_code, crc8_maxim, crc8_darc, crc8_cdma2000, crc8_wcdma, crc8_dvb_s2 };

        template <const std::size_t size, typename T, std::enable_if_t<std::is_integral_v<T>, bool> = true>
        constexpr T binaryReverse(T _byte) noexcept {
            T reversed_byte { 0x00 };

            for (std::size_t bit = 0; bit != size; ++bit, _byte >>= 0x01) {
                reversed_byte = reversed_byte << 0x01 | (_byte & 0x01);
            }

            return reversed_byte;
        }

        template <typename T, std::enable_if_t<std::is_integral_v<T>, bool> = true>
        constexpr auto generateCRCTable(const T _polynomial, const bool _ref_in, const bool _ref_out) noexcept {
            constexpr std::size_t bits  { sizeof(T) * 8 };
            constexpr std::size_t shift { bits - 8 };
            constexpr auto        mask  { 0x80 << shift };

            std::array<T, 256> crc_table;

            for (const std::size_t byte : std::ranges::iota_view(0, 256)) {
                auto crc { _ref_in ? (binaryReverse<bits>(static_cast<T>(byte)) >> shift): static_cast<T>(byte) };

                for (std::size_t bit = 0; bit != bits; ++bit) {
                    if (crc & mask) { crc = (crc << 0x01) ^ _polynomial; }
                    else { crc <<= 0x01; }
                }
                crc_table[byte] = _ref_out ? binaryReverse<bits>(crc) : crc;
            }

            return crc_table;
        }

        template <crc_types T, std::enable_if_t<std::is_same_v<decltype(T), crc_types>, bool> = true>
        constexpr auto initCRCTable() noexcept {
            if constexpr (T == crc_types::crc8_ccitt)    { return generateCRCTable<uint8_t>(0x07, false, false); }
            if constexpr (T == crc_types::crc8_itu)      { return generateCRCTable<uint8_t>(0x07, false, false); }
            if constexpr (T == crc_types::crc8_rohc)     { return generateCRCTable<uint8_t>(0x07, true,  true); }
            if constexpr (T == crc_types::crc8_ebu)      { return generateCRCTable<uint8_t>(0x1d, true,  true); }
            if constexpr (T == crc_types::crc8_i_code)   { return generateCRCTable<uint8_t>(0x1d, false, false); }
            if constexpr (T == crc_types::crc8_maxim)    { return generateCRCTable<uint8_t>(0x31, true,  true); }
            if constexpr (T == crc_types::crc8_darc)     { return generateCRCTable<uint8_t>(0x39, true,  true); }
            if constexpr (T == crc_types::crc8_cdma2000) { return generateCRCTable<uint8_t>(0x9b, false, false); }
            if constexpr (T == crc_types::crc8_wcdma)    { return generateCRCTable<uint8_t>(0x9b, true,  true); }
            if constexpr (T == crc_types::crc8_dvb_s2)   { return generateCRCTable<uint8_t>(0xd5, false, false); }
        }

        template <crc_types T, std::enable_if_t<std::is_same_v<decltype(T), crc_types>, bool> = true>
        auto generateCRCCode(const uint8_t* _data, std::size_t _size) noexcept {
            static constexpr auto crc_table { initCRCTable<T>() };

            auto crc_code { 0x00 };
            if constexpr (T == crc_types::crc8_rohc || T == crc_types::crc8_ebu || T == crc_types::crc8_cdma2000) {
                crc_code = 0xff;
            } else if constexpr (T == crc_types::crc8_i_code) {
                crc_code = 0xfd;
            }

            do {
                crc_code = crc_table.at(crc_code ^ *_data++);
            } while (--_size);
            if constexpr (T == crc_types::crc8_itu) { crc_code ^= 0x55; }

            return crc_code;
        }
    }

    using authlib::detail::crc_types;

    template <crc_types T, std::enable_if_t<std::is_same_v<decltype(T), crc_types>, bool> = true>
    constexpr auto crc_gen(const uint8_t* _data, const std::size_t _size) noexcept {
        return authlib::detail::generateCRCCode<T>(_data, _size);
    }

    template <crc_types T, typename V, std::enable_if_t<std::is_same_v<decltype(T), crc_types>, bool> = true>
    constexpr auto crc_gen(const V& _str) noexcept {
        return crc_gen<T>(
            reinterpret_cast<const uint8_t*>(_str.data()),
            _str.size()
        );
    }

    template <crc_types T, std::enable_if_t<std::is_same_v<decltype(T), crc_types>, bool> = true>
    constexpr auto crc_gen(const char* _str) noexcept {
        return crc_gen<T>(
            reinterpret_cast<const uint8_t*>(_str),
            std::strlen(_str)
        );
    }
}
