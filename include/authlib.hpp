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
        enum crc8_types { ccitt, itu, rohc, ebu, i_code, maxim, darc, cdma2000, wcdma, dvb_s2 };

        template <const std::size_t size, typename T>
        constexpr T binaryReverse(T _byte) noexcept {
            T reversed_byte { 0x00 };

            for (std::size_t bit = 0; bit != size; ++bit, _byte >>= 0x01) {
                reversed_byte = reversed_byte << 0x01 | (_byte & 0x01);
            }

            return reversed_byte;
        }

        constexpr auto generateCRC8Table(const unsigned char _polynomial, const bool _ref_in, const bool _ref_out) noexcept {
            std::array<unsigned char, 256> crc_table;

            for (const uint8_t byte : std::ranges::iota_view(1, 256)) {
                auto crc { _ref_in ? binaryReverse<8>(byte) : byte };

                for (std::size_t bit = 0; bit != 8; ++bit) {
                    if (static_cast<bool>(crc & 0x80)) { crc = (crc << 0x01) ^ _polynomial; }
                    else { crc <<= 0x01; }
                }
                crc_table[byte] = _ref_out ? binaryReverse<8>(crc) : crc;
            }

            return crc_table;
        }

        template<crc8_types T, std::enable_if_t<std::is_same_v<decltype(T), crc8_types>, bool> = true>
        constexpr auto initCRC8Table() noexcept {
            if constexpr (T == crc8_types::ccitt)    { return generateCRC8Table(0x07, false, false); }
            if constexpr (T == crc8_types::itu)      { return generateCRC8Table(0x07, false, false); }
            if constexpr (T == crc8_types::rohc)     { return generateCRC8Table(0x07, true,  true); }
            if constexpr (T == crc8_types::ebu)      { return generateCRC8Table(0x1d, true,  true); }
            if constexpr (T == crc8_types::i_code)   { return generateCRC8Table(0x1d, false, false); }
            if constexpr (T == crc8_types::maxim)    { return generateCRC8Table(0x31, true,  true); }
            if constexpr (T == crc8_types::darc)     { return generateCRC8Table(0x39, true,  true); }
            if constexpr (T == crc8_types::cdma2000) { return generateCRC8Table(0x9b, false, false); }
            if constexpr (T == crc8_types::wcdma)    { return generateCRC8Table(0x9b, true,  true); }
            if constexpr (T == crc8_types::dvb_s2)   { return generateCRC8Table(0xd5, false, false); }
        }

        template <crc8_types T, std::enable_if_t<std::is_same_v<decltype(T), crc8_types>, bool> = true>
        auto getCRC8Code(const unsigned char* _data, std::size_t _size) noexcept {
            static constexpr auto crc8_table { initCRC8Table<T>() };

            auto crc8_code { 0x00 };
            if constexpr (T == crc8_types::rohc || T == crc8_types::ebu || T == crc8_types::cdma2000) {
                crc8_code = 0xff;
            } else if constexpr (T == crc8_types::i_code) {
                crc8_code = 0xfd;
            }

            do {
                crc8_code = crc8_table.at(crc8_code ^ *_data++);
            } while (--_size);
            if constexpr (T == crc8_types::itu) { crc8_code ^= 0x55; }

            return crc8_code;
        }
    }

    using authlib::detail::crc8_types;

    template <crc8_types T, std::enable_if_t<std::is_same_v<decltype(T), crc8_types>, bool> = true>
    constexpr auto crc8_gen(const unsigned char* _data, const std::size_t _size) noexcept {
        return authlib::detail::getCRC8Code<T>(_data, _size);
    }

    template <crc8_types T, typename V, std::enable_if_t<std::is_same_v<decltype(T), crc8_types>, bool> = true>
    constexpr auto crc8_gen(const V& _str) noexcept {
        return crc8_gen<T>(
            reinterpret_cast<const unsigned char*>(_str.data()),
            _str.size()
        );
    }

    template <crc8_types T, std::enable_if_t<std::is_same_v<decltype(T), crc8_types>, bool> = true>
    constexpr auto crc8_gen(const char* _str) noexcept {
        return crc8_gen<T>(
            reinterpret_cast<const unsigned char*>(_str),
            std::strlen(_str)
        );
    }
}
