#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <hwlib/algorithms/crc.hpp>
#include <span>

namespace
{

// The catalogue check value: the CRC of the ASCII string "123456789".
constexpr std::array<std::uint8_t, 9> CHECK_INPUT{'1', '2', '3', '4', '5', '6', '7', '8', '9'};

TEST(CrcTest, Crc8Nrsc5MatchesTheCatalogueCheckValue)
{
    EXPECT_EQ(hwlib::algorithms::Crc8Nrsc5(CHECK_INPUT), 0xF7U);
}

// The example from the Sensirion SHT4x datasheet.
TEST(CrcTest, Crc8Nrsc5MatchesTheSensirionDatasheetExample)
{
    constexpr std::array<std::uint8_t, 2> DATA{0xBEU, 0xEFU};
    EXPECT_EQ(hwlib::algorithms::Crc8Nrsc5(DATA), 0x92U);
}

TEST(CrcTest, Crc16CcittMatchesTheCatalogueCheckValue)
{
    EXPECT_EQ(hwlib::algorithms::Crc16Ccitt(CHECK_INPUT), 0x29B1U);
}

// Cross-checked against zlib.crc32("123456789").
TEST(CrcTest, Crc32IsoHdlcMatchesTheCatalogueCheckValue)
{
    EXPECT_EQ(hwlib::algorithms::Crc32IsoHdlc(CHECK_INPUT), 0xCBF43926U);
}

TEST(CrcTest, EmptyInputYieldsTheInitialValues)
{
    const std::span<const std::uint8_t> empty{};
    EXPECT_EQ(hwlib::algorithms::Crc8Nrsc5(empty), 0xFFU);
    EXPECT_EQ(hwlib::algorithms::Crc16Ccitt(empty), 0xFFFFU);
    EXPECT_EQ(hwlib::algorithms::Crc32IsoHdlc(empty), 0x00000000U);
}

TEST(CrcTest, DetectsASingleFlippedBit)
{
    std::array<std::uint8_t, 9> corrupted  = CHECK_INPUT;
    corrupted[4]                          ^= 0x01U;

    EXPECT_NE(hwlib::algorithms::Crc8Nrsc5(corrupted), hwlib::algorithms::Crc8Nrsc5(CHECK_INPUT));
    EXPECT_NE(hwlib::algorithms::Crc16Ccitt(corrupted), hwlib::algorithms::Crc16Ccitt(CHECK_INPUT));
    EXPECT_NE(hwlib::algorithms::Crc32IsoHdlc(corrupted), hwlib::algorithms::Crc32IsoHdlc(CHECK_INPUT));
}

TEST(CrcTest, DetectsReorderedBytes)
{
    constexpr std::array<std::uint8_t, 2> FORWARD{0x01U, 0x02U};
    constexpr std::array<std::uint8_t, 2> SWAPPED{0x02U, 0x01U};

    EXPECT_NE(hwlib::algorithms::Crc16Ccitt(FORWARD), hwlib::algorithms::Crc16Ccitt(SWAPPED));
    EXPECT_NE(hwlib::algorithms::Crc32IsoHdlc(FORWARD), hwlib::algorithms::Crc32IsoHdlc(SWAPPED));
}

TEST(CrcTest, AllThreeAreUsableAtCompileTime)
{
    static_assert(hwlib::algorithms::Crc8Nrsc5(CHECK_INPUT) == 0xF7U);
    static_assert(hwlib::algorithms::Crc16Ccitt(CHECK_INPUT) == 0x29B1U);
    static_assert(hwlib::algorithms::Crc32IsoHdlc(CHECK_INPUT) == 0xCBF43926U);
    SUCCEED();
}

TEST(Crc32StreamTest, ChunkedUpdatesMatchTheOneShotCall)
{
    hwlib::algorithms::Crc32Stream stream;
    stream.Update(std::span{CHECK_INPUT.data(), 4U});
    stream.Update(std::span{CHECK_INPUT.data() + 4U, 5U});

    EXPECT_EQ(stream.Value(), hwlib::algorithms::Crc32IsoHdlc(CHECK_INPUT));
}

TEST(Crc32StreamTest, ByteAtATimeMatchesTheOneShotCall)
{
    hwlib::algorithms::Crc32Stream stream;
    for (const std::uint8_t byte : CHECK_INPUT)
    {
        stream.Update(std::span{&byte, 1U});
    }

    EXPECT_EQ(stream.Value(), 0xCBF43926U);
}

TEST(Crc32StreamTest, ValueDoesNotEndTheStream)
{
    hwlib::algorithms::Crc32Stream stream;
    stream.Update(std::span{CHECK_INPUT.data(), 4U});
    const std::uint32_t partial = stream.Value();

    stream.Update(std::span{CHECK_INPUT.data() + 4U, 5U});
    EXPECT_NE(stream.Value(), partial);
    EXPECT_EQ(stream.Value(), 0xCBF43926U);
}

TEST(Crc32StreamTest, AFreshStreamYieldsTheEmptyValue)
{
    const hwlib::algorithms::Crc32Stream stream;
    EXPECT_EQ(stream.Value(), 0x00000000U);
}

TEST(Crc32StreamTest, ResetStartsOver)
{
    hwlib::algorithms::Crc32Stream stream;
    stream.Update(CHECK_INPUT);
    stream.Reset();
    stream.Update(CHECK_INPUT);

    EXPECT_EQ(stream.Value(), 0xCBF43926U);
}

} // namespace
