#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace hwlib::algorithms
{

// Bit-by-bit implementations: no lookup tables, so nothing is spent on flash
// for a table a device may use once at boot. All three are constexpr and can be
// evaluated at compile time.
//
// Parameters are named after the algorithm catalogue, so a reader never has to
// guess the polynomial, init value or reflection from a call site.

/// CRC-8/NRSC-5: poly 0x31, init 0xFF, no reflection, no final xor.
/// Used by Sensirion sensors (SHT4x and relatives) to guard each data word.
/// Check value: "123456789" -> 0xF7.
[[nodiscard]] constexpr std::uint8_t Crc8Nrsc5(std::span<const std::uint8_t> data) noexcept
{
    constexpr std::uint8_t POLY = 0x31U;
    constexpr std::uint8_t INIT = 0xFFU;
    constexpr std::uint8_t MSB  = 0x80U;

    std::uint8_t crc = INIT;
    for (const std::uint8_t byte : data)
    {
        crc ^= byte;
        for (unsigned bit = 0U; bit < 8U; ++bit)
        {
            crc = (crc & MSB) != 0U ? static_cast<std::uint8_t>((crc << 1U) ^ POLY)
                                    : static_cast<std::uint8_t>(crc << 1U);
        }
    }
    return crc;
}

/// CRC-16/CCITT-FALSE: poly 0x1021, init 0xFFFF, no reflection, no final xor.
/// Check value: "123456789" -> 0x29B1.
[[nodiscard]] constexpr std::uint16_t Crc16Ccitt(std::span<const std::uint8_t> data) noexcept
{
    constexpr std::uint16_t POLY = 0x1021U;
    constexpr std::uint16_t INIT = 0xFFFFU;
    constexpr std::uint16_t MSB  = 0x8000U;

    std::uint16_t crc = INIT;
    for (const std::uint8_t byte : data)
    {
        crc = static_cast<std::uint16_t>(crc ^ (static_cast<std::uint16_t>(byte) << 8U));
        for (unsigned bit = 0U; bit < 8U; ++bit)
        {
            crc = (crc & MSB) != 0U ? static_cast<std::uint16_t>((crc << 1U) ^ POLY)
                                    : static_cast<std::uint16_t>(crc << 1U);
        }
    }
    return crc;
}

/// CRC-32/ISO-HDLC, the one zlib, Ethernet and PNG use: reflected poly
/// 0xEDB88320, init 0xFFFFFFFF, final xor 0xFFFFFFFF.
/// Check value: "123456789" -> 0xCBF43926.
[[nodiscard]] constexpr std::uint32_t Crc32IsoHdlc(std::span<const std::uint8_t> data) noexcept;

/// Incremental CRC-32/ISO-HDLC, for data that does not arrive in one piece —
/// a firmware image verified page by page, for instance. Feeding everything
/// through one stream gives the same value as a single Crc32IsoHdlc() call.
class Crc32Stream
{
public:
    constexpr Crc32Stream() = default;

    constexpr void Update(std::span<const std::uint8_t> data) noexcept
    {
        constexpr std::uint32_t POLY = 0xEDB88320U;

        for (const std::uint8_t byte : data)
        {
            m_crc ^= byte;
            for (unsigned bit = 0U; bit < 8U; ++bit)
            {
                m_crc = (m_crc & 1U) != 0U ? (m_crc >> 1U) ^ POLY : m_crc >> 1U;
            }
        }
    }

    /// Current value with the final xor applied. Does not end the stream:
    /// further Update() calls keep extending the same CRC.
    [[nodiscard]] constexpr std::uint32_t Value() const noexcept
    {
        return m_crc ^ XOR_OUT;
    }

    constexpr void Reset() noexcept
    {
        m_crc = INIT;
    }

private:
    static constexpr std::uint32_t INIT    = 0xFFFFFFFFU;
    static constexpr std::uint32_t XOR_OUT = 0xFFFFFFFFU;

    std::uint32_t m_crc{INIT};
};

[[nodiscard]] constexpr std::uint32_t Crc32IsoHdlc(std::span<const std::uint8_t> data) noexcept
{
    Crc32Stream stream;
    stream.Update(data);
    return stream.Value();
}

} // namespace hwlib::algorithms
