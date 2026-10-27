#include "core/model/ActivityName.hpp"

#include <gtest/gtest.h>

#include <string>
#include <vector>

TEST(ActivityNameTest, SurroundingWhitespaceIsRemoved)
{
    EXPECT_EQ(ActivityName::trimmed("  Gym \t"), "Gym");
    EXPECT_EQ(ActivityName::trimmed("Morning run"), "Morning run");
}

TEST(ActivityNameTest, ANameMadeOfWhitespaceIsEmpty)
{
    EXPECT_TRUE(ActivityName::trimmed("").empty());
    EXPECT_TRUE(ActivityName::trimmed("   \t\n").empty());
}

TEST(ActivityNameTest, ANameIsTakenIgnoringCaseAndSurroundingWhitespace)
{
    const std::vector<std::string> others = { "Gym", "Łódź trip" };

    EXPECT_TRUE(ActivityName::isTaken("gym", others));
    EXPECT_TRUE(ActivityName::isTaken("  GYM ", others));
    EXPECT_TRUE(ActivityName::isTaken("ŁÓDŹ TRIP", others));
}

TEST(ActivityNameTest, ADifferentNameIsNotTaken)
{
    const std::vector<std::string> others = { "Gym" };

    EXPECT_FALSE(ActivityName::isTaken("Gym 2", others));
    EXPECT_FALSE(ActivityName::isTaken("Gym", {}));
}
