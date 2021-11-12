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
        enum CRCTypes {
            crc8, crc8_cdma2000, crc8_darc, crc8_dvb_s2, crc8_ebu, crc8_i_code, crc8_itu, crc8_maxim, crc8_rohc, crc8_wcdma,
            crc16_a, crc16_arc, crc16_aug_ccitt, crc16_buypass, crc16_ccitt_false, crc16_cdma2000, crc16_dds_110, crc16_dect_r, crc16_dect_x, crc16_dnp, crc16_en_13757, crc16_genibus, crc16_kermit, crc16_maxim, crc16_mcrf4xx, crc16_modbus, crc16_riello, crc16_t10_dif, crc16_teledisk, crc16_tms37157, crc16_usb, crc16_x_25, crc16_xmodem,
            crc32, crc32_bzip2, crc32_c, crc32_d, crc32_jamcrc, crc32_mpeg_2, crc32_posix, crc32_q, crc32_xfer,
            crc64_ecma, crc64_iso
        };

        template <const std::size_t size, typename T, std::enable_if_t<std::is_integral_v<T>, bool> = true>
        constexpr T binaryReverse(T _byte) noexcept {
            auto reversed_byte { static_cast<T>(0x00) };
            for (std::size_t bit = 0; bit != size; ++bit, _byte >>= 0x01) {
                reversed_byte = reversed_byte << 0x01 | (_byte & 0x01);
            }

            return reversed_byte;
        }

        template <typename T, std::enable_if_t<std::is_integral_v<T>, bool> = true>
        constexpr auto generateCRCTable(const T _polynomial, const bool _ref_in, const bool _ref_out) noexcept {
            constexpr std::size_t bits  { sizeof(T) * 8 };
            constexpr std::size_t shift { bits - 8 };
            constexpr auto        mask  { static_cast<T>(0x80) << shift };

            std::array<T, 256> crc_table;
            for (const std::size_t byte : std::ranges::iota_view(0, 256)) {
                auto crc { _ref_in ? binaryReverse<bits>(static_cast<T>(byte)) >> shift : static_cast<T>(byte) };
                for (std::size_t bit = 0; bit != bits; ++bit) {
                    if (crc & mask) { crc = (crc << 0x01) ^ _polynomial; }
                    else { crc <<= 0x01; }
                }
                crc_table[byte] = _ref_out ? binaryReverse<bits>(crc) : crc;
            }

            return crc_table;
        }

        template <
            typename V, V polynomial, V init, V xor_out, bool ref_in, bool ref_out, std::enable_if_t<std::is_integral_v<V>, bool> = true
        > auto generateCRCCode(const uint8_t* _data, std::size_t _size) noexcept {
            constexpr std::size_t bits  { sizeof(V) * 8 };
            constexpr std::size_t shift { bits - 8 };

            static constexpr auto crc_table { generateCRCTable<V>(polynomial, ref_in, ref_out) };
            auto                  crc_code  { init };
            do {
                crc_code = (ref_out ? crc_code >> 8 : crc_code << 8) ^ crc_table.at((ref_in ? crc_code & 0xff : crc_code >> shift) ^ *_data++);
            } while (--_size);

            return crc_code ^ xor_out;
        }
    }

    using crc_types = authlib::detail::CRCTypes;

    template <crc_types T, std::enable_if_t<std::is_same_v<decltype(T), crc_types>, bool> = true>
    constexpr auto crc_gen(const uint8_t* _data, const std::size_t _size) noexcept {
        using namespace ubn::authlib::detail;

        if constexpr (T == crc_types::crc8)          { return generateCRCCode<uint8_t, 0x07, 0x00, 0x00, false, false>(_data, _size); }
        if constexpr (T == crc_types::crc8_cdma2000) { return generateCRCCode<uint8_t, 0x9b, 0xff, 0x00, false, false>(_data, _size); }
        if constexpr (T == crc_types::crc8_darc)     { return generateCRCCode<uint8_t, 0x39, 0x00, 0x00, true,  true >(_data, _size); }
        if constexpr (T == crc_types::crc8_dvb_s2)   { return generateCRCCode<uint8_t, 0xd5, 0x00, 0x00, false, false>(_data, _size); }
        if constexpr (T == crc_types::crc8_ebu)      { return generateCRCCode<uint8_t, 0x1d, 0xff, 0x00, true,  true >(_data, _size); }
        if constexpr (T == crc_types::crc8_i_code)   { return generateCRCCode<uint8_t, 0x1d, 0xfd, 0x00, false, false>(_data, _size); }
        if constexpr (T == crc_types::crc8_itu)      { return generateCRCCode<uint8_t, 0x07, 0x00, 0x55, false, false>(_data, _size); }
        if constexpr (T == crc_types::crc8_maxim)    { return generateCRCCode<uint8_t, 0x31, 0x00, 0x00, true,  true >(_data, _size); }
        if constexpr (T == crc_types::crc8_rohc)     { return generateCRCCode<uint8_t, 0x07, 0xff, 0x00, true,  true >(_data, _size); }
        if constexpr (T == crc_types::crc8_wcdma)    { return generateCRCCode<uint8_t, 0x9b, 0x00, 0x00, true,  true >(_data, _size); }

        if constexpr (T == crc_types::crc16_a)           { return generateCRCCode<uint16_t, 0x1021, 0xc6c6, 0x0000, true,  true >(_data, _size); }
        if constexpr (T == crc_types::crc16_arc)         { return generateCRCCode<uint16_t, 0x8005, 0x0000, 0x0000, true,  true >(_data, _size); }
        if constexpr (T == crc_types::crc16_aug_ccitt)   { return generateCRCCode<uint16_t, 0x1021, 0x1d0f, 0x0000, false, false>(_data, _size); }
        if constexpr (T == crc_types::crc16_buypass)     { return generateCRCCode<uint16_t, 0x8005, 0x0000, 0x0000, false, false>(_data, _size); }
        if constexpr (T == crc_types::crc16_cdma2000)    { return generateCRCCode<uint16_t, 0xc867, 0xffff, 0x0000, false, false>(_data, _size); }
        if constexpr (T == crc_types::crc16_ccitt_false) { return generateCRCCode<uint16_t, 0x1021, 0xffff, 0x0000, false, false>(_data, _size); }
        if constexpr (T == crc_types::crc16_dds_110)     { return generateCRCCode<uint16_t, 0x8005, 0x800d, 0x0000, false, false>(_data, _size); }
        if constexpr (T == crc_types::crc16_dect_r)      { return generateCRCCode<uint16_t, 0x0589, 0x0000, 0x0001, false, false>(_data, _size); }
        if constexpr (T == crc_types::crc16_dect_x)      { return generateCRCCode<uint16_t, 0x0589, 0x0000, 0x0000, false, false>(_data, _size); }
        if constexpr (T == crc_types::crc16_dnp)         { return generateCRCCode<uint16_t, 0x3d65, 0x0000, 0xffff, true,  true >(_data, _size); }
        if constexpr (T == crc_types::crc16_en_13757)    { return generateCRCCode<uint16_t, 0x3d65, 0x0000, 0xffff, false, false>(_data, _size); }
        if constexpr (T == crc_types::crc16_genibus)     { return generateCRCCode<uint16_t, 0x1021, 0xffff, 0xffff, false, false>(_data, _size); }
        if constexpr (T == crc_types::crc16_kermit)      { return generateCRCCode<uint16_t, 0x1021, 0x0000, 0x0000, true,  true >(_data, _size); }
        if constexpr (T == crc_types::crc16_maxim)       { return generateCRCCode<uint16_t, 0x8005, 0x0000, 0xffff, true,  true >(_data, _size); }
        if constexpr (T == crc_types::crc16_mcrf4xx)     { return generateCRCCode<uint16_t, 0x1021, 0xffff, 0x0000, true,  true >(_data, _size); }
        if constexpr (T == crc_types::crc16_modbus)      { return generateCRCCode<uint16_t, 0x8005, 0xffff, 0x0000, true,  true >(_data, _size); }
        if constexpr (T == crc_types::crc16_riello)      { return generateCRCCode<uint16_t, 0x1021, 0xb2aa, 0x0000, true,  true >(_data, _size); }
        if constexpr (T == crc_types::crc16_t10_dif)     { return generateCRCCode<uint16_t, 0x8bb7, 0x0000, 0x0000, false, false>(_data, _size); }
        if constexpr (T == crc_types::crc16_teledisk)    { return generateCRCCode<uint16_t, 0xa097, 0x0000, 0x0000, false, false>(_data, _size); }
        if constexpr (T == crc_types::crc16_tms37157)    { return generateCRCCode<uint16_t, 0x1021, 0x89ec, 0x0000, true,  true >(_data, _size); }
        if constexpr (T == crc_types::crc16_usb)         { return generateCRCCode<uint16_t, 0x8005, 0xffff, 0xffff, true,  true >(_data, _size); }
        if constexpr (T == crc_types::crc16_x_25)        { return generateCRCCode<uint16_t, 0x1021, 0xffff, 0xffff, true,  true >(_data, _size); }
        if constexpr (T == crc_types::crc16_xmodem)      { return generateCRCCode<uint16_t, 0x1021, 0x0000, 0x0000, false, false>(_data, _size); }

        if constexpr (T == crc_types::crc32)        { return generateCRCCode<uint32_t, 0x04c11db7, 0xffffffff, 0xffffffff, true,  true >(_data, _size); }
        if constexpr (T == crc_types::crc32_bzip2)  { return generateCRCCode<uint32_t, 0x04c11db7, 0xffffffff, 0xffffffff, false, false>(_data, _size); }
        if constexpr (T == crc_types::crc32_c)      { return generateCRCCode<uint32_t, 0x1edc6f41, 0xffffffff, 0xffffffff, true,  true >(_data, _size); }
        if constexpr (T == crc_types::crc32_d)      { return generateCRCCode<uint32_t, 0xa833982b, 0xffffffff, 0xffffffff, true,  true >(_data, _size); }
        if constexpr (T == crc_types::crc32_jamcrc) { return generateCRCCode<uint32_t, 0x04c11db7, 0xffffffff, 0x00000000, true,  true >(_data, _size); }
        if constexpr (T == crc_types::crc32_mpeg_2) { return generateCRCCode<uint32_t, 0x04c11db7, 0xffffffff, 0x00000000, false, false>(_data, _size); }
        if constexpr (T == crc_types::crc32_posix)  { return generateCRCCode<uint32_t, 0x04c11db7, 0x00000000, 0xffffffff, false, false>(_data, _size); }
        if constexpr (T == crc_types::crc32_q)      { return generateCRCCode<uint32_t, 0x814141ab, 0x00000000, 0x00000000, false, false>(_data, _size); }
        if constexpr (T == crc_types::crc32_xfer)   { return generateCRCCode<uint32_t, 0x000000af, 0x00000000, 0x00000000, false, false>(_data, _size); }

        if constexpr (T == crc_types::crc64_ecma) { return generateCRCCode<uint64_t, 0x42f0e1eba9ea3693, 0xffffffffffffffff, 0xffffffffffffffff, true, true>(_data, _size); }
        if constexpr (T == crc_types::crc64_iso)  { return generateCRCCode<uint64_t, 0x000000000000001b, 0xffffffffffffffff, 0xffffffffffffffff, true, true>(_data, _size); }
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
