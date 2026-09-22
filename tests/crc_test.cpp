#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <integra/crc.hpp>
#include <span>

namespace
{

// The catalogue check value: the CRC of the ASCII string "123456789".
constexpr std::array<std::uint8_t, 9> CHECK_INPUT{'1', '2', '3', '4', '5', '6', '7', '8', '9'};

TEST(CrcTest, Crc8Nrsc5MatchesTheCatalogueCheckValue)
{
    EXPECT_EQ(integra::Crc8Nrsc5(CHECK_INPUT), 0xF7U);
}

// The example from the Sensirion SHT4x datasheet.
TEST(CrcTest, Crc8Nrsc5MatchesTheSensirionDatasheetExample)
{
    constexpr std::array<std::uint8_t, 2> DATA{0xBEU, 0xEFU};
    EXPECT_EQ(integra::Crc8Nrsc5(DATA), 0x92U);
}

TEST(CrcTest, Crc16CcittMatchesTheCatalogueCheckValue)
{
    EXPECT_EQ(integra::Crc16Ccitt(CHECK_INPUT), 0x29B1U);
}

// Cross-checked against zlib.crc32("123456789").
TEST(CrcTest, Crc32IsoHdlcMatchesTheCatalogueCheckValue)
{
    EXPECT_EQ(integra::Crc32IsoHdlc(CHECK_INPUT), 0xCBF43926U);
}

TEST(CrcTest, EmptyInputYieldsTheInitialValues)
{
    const std::span<const std::uint8_t> empty{};
    EXPECT_EQ(integra::Crc8Nrsc5(empty), 0xFFU);
    EXPECT_EQ(integra::Crc16Ccitt(empty), 0xFFFFU);
    EXPECT_EQ(integra::Crc32IsoHdlc(empty), 0x00000000U);
}

TEST(CrcTest, DetectsASingleFlippedBit)
{
    std::array<std::uint8_t, 9> corrupted  = CHECK_INPUT;
    corrupted[4]                          ^= 0x01U;

    EXPECT_NE(integra::Crc8Nrsc5(corrupted), integra::Crc8Nrsc5(CHECK_INPUT));
    EXPECT_NE(integra::Crc16Ccitt(corrupted), integra::Crc16Ccitt(CHECK_INPUT));
    EXPECT_NE(integra::Crc32IsoHdlc(corrupted), integra::Crc32IsoHdlc(CHECK_INPUT));
}

TEST(CrcTest, DetectsReorderedBytes)
{
    constexpr std::array<std::uint8_t, 2> FORWARD{0x01U, 0x02U};
    constexpr std::array<std::uint8_t, 2> SWAPPED{0x02U, 0x01U};

    EXPECT_NE(integra::Crc16Ccitt(FORWARD), integra::Crc16Ccitt(SWAPPED));
    EXPECT_NE(integra::Crc32IsoHdlc(FORWARD), integra::Crc32IsoHdlc(SWAPPED));
}

TEST(CrcTest, AllThreeAreUsableAtCompileTime)
{
    static_assert(integra::Crc8Nrsc5(CHECK_INPUT) == 0xF7U);
    static_assert(integra::Crc16Ccitt(CHECK_INPUT) == 0x29B1U);
    static_assert(integra::Crc32IsoHdlc(CHECK_INPUT) == 0xCBF43926U);
    SUCCEED();
}

TEST(Crc32StreamTest, ChunkedUpdatesMatchTheOneShotCall)
{
    integra::Crc32Stream stream;
    stream.Update(std::span{CHECK_INPUT.data(), 4U});
    stream.Update(std::span{CHECK_INPUT.data() + 4U, 5U});

    EXPECT_EQ(stream.Value(), integra::Crc32IsoHdlc(CHECK_INPUT));
}

TEST(Crc32StreamTest, ByteAtATimeMatchesTheOneShotCall)
{
    integra::Crc32Stream stream;
    for (const std::uint8_t byte : CHECK_INPUT)
    {
        stream.Update(std::span{&byte, 1U});
    }

    EXPECT_EQ(stream.Value(), 0xCBF43926U);
}

TEST(Crc32StreamTest, ValueDoesNotEndTheStream)
{
    integra::Crc32Stream stream;
    stream.Update(std::span{CHECK_INPUT.data(), 4U});
    const std::uint32_t partial = stream.Value();

    stream.Update(std::span{CHECK_INPUT.data() + 4U, 5U});
    EXPECT_NE(stream.Value(), partial);
    EXPECT_EQ(stream.Value(), 0xCBF43926U);
}

TEST(Crc32StreamTest, AFreshStreamYieldsTheEmptyValue)
{
    const integra::Crc32Stream stream;
    EXPECT_EQ(stream.Value(), 0x00000000U);
}

TEST(Crc32StreamTest, ResetStartsOver)
{
    integra::Crc32Stream stream;
    stream.Update(CHECK_INPUT);
    stream.Reset();
    stream.Update(CHECK_INPUT);

    EXPECT_EQ(stream.Value(), 0xCBF43926U);
}

} // namespace
