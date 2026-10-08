#include "core/model/PlannerDocument.hpp"

#include "support/PlannerTestDocuments.hpp"

#include "stapik/sync/SyncCoordinator.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <optional>
#include <stdexcept>
#include <tuple>
#include <string>

namespace
{
    using planner::test::at;
    using planner::test::makeSampleDocument;

    static_assert(stapik::document::PersistableDocument<PlannerDocument>);
    static_assert(stapik::sync::SyncableDocument<PlannerDocument>);

    nlohmann::json roundTrip(const PlannerDocument& document)
    {
        return PlannerDocument::fromJson(document.toJson()).toJson();
    }
}

TEST(PlannerDocumentTest, DefaultDocumentHasEmptyWeekPlanWithDefaultSlotCount)
{
    const auto document = PlannerDocument::createDefault();

    EXPECT_EQ(document.settings.slots, Settings::DEFAULT_SLOTS);
    EXPECT_TRUE(document.activities.empty());
    EXPECT_EQ(document.lastUpdate(), PlannerDocument::TimePoint{});
    EXPECT_FALSE(document.lastKnownCloudUpdate().has_value());

    for (std::size_t dayIndex = 0; dayIndex < document.weekPlan.size(); ++dayIndex)
    {
        EXPECT_EQ(static_cast<std::size_t>(document.weekPlan[dayIndex].weekday), dayIndex);
        EXPECT_EQ(document.weekPlan[dayIndex].slots.size(), static_cast<std::size_t>(Settings::DEFAULT_SLOTS));
        EXPECT_EQ(document.weekPlan[dayIndex].totalLoad(), 0);
    }
}

TEST(PlannerDocumentTest, JsonRoundTripPreservesAllContent)
{
    const auto original = makeSampleDocument(120);

    const auto restored = PlannerDocument::fromJson(original.toJson());

    EXPECT_EQ(restored.toJson(), original.toJson());
    EXPECT_EQ(restored.settings.slots, original.settings.slots);
    ASSERT_EQ(restored.activities.size(), 2U);
    EXPECT_EQ(restored.activities[0].name, "Gym");
    EXPECT_EQ(restored.activities[0].difficulty, Difficulty::HARD);
    ASSERT_TRUE(restored.weekPlan[0].slots[0].has_value());
    EXPECT_EQ(restored.weekPlan[0].slots[0]->name, "Gym");
    EXPECT_FALSE(restored.weekPlan[0].slots[1].has_value());
    ASSERT_TRUE(restored.weekPlan[4].slots[1].has_value());
    EXPECT_EQ(restored.weekPlan[4].slots[1]->difficulty, Difficulty::LIGHT);
}

TEST(PlannerDocumentTest, LastUpdateSurvivesTheRoundTripWithSecondPrecision)
{
    auto document = makeSampleDocument(0);
    document.updatedAt = at(300) + std::chrono::milliseconds{ 750 };

    const auto restored = PlannerDocument::fromJson(document.toJson());

    EXPECT_EQ(restored.lastUpdate(), at(300));
}

TEST(PlannerDocumentTest, CloudBaselineIsLocalBookkeepingAndNotSerialized)
{
    const auto document = makeSampleDocument(10).withLastKnownCloudUpdate(at(500));

    EXPECT_EQ(document.toJson(), makeSampleDocument(10).toJson());
    EXPECT_FALSE(PlannerDocument::fromJson(document.toJson()).lastKnownCloudUpdate().has_value());
}

TEST(PlannerDocumentTest, WithLastKnownCloudUpdateKeepsTheContent)
{
    const auto original = makeSampleDocument(10);

    const auto updated = original.withLastKnownCloudUpdate(at(500));

    EXPECT_EQ(updated.lastKnownCloudUpdate(), std::optional<PlannerDocument::TimePoint>{ at(500) });
    EXPECT_EQ(updated.lastUpdate(), original.lastUpdate());
    EXPECT_EQ(updated.toJson(), original.toJson());
    EXPECT_FALSE(original.lastKnownCloudUpdate().has_value());
}

TEST(PlannerDocumentTest, MissingSectionsFallBackToDefaults)
{
    const auto document = PlannerDocument::fromJson(nlohmann::json::object());

    EXPECT_EQ(document.toJson(), PlannerDocument::createDefault().toJson());
}

TEST(PlannerDocumentTest, SlotCountIsClampedToTheSupportedRange)
{
    const auto tooFew = PlannerDocument::fromJson({ { "settings", { { "slots", 0 } } } });
    const auto tooMany = PlannerDocument::fromJson({ { "settings", { { "slots", 99 } } } });

    EXPECT_EQ(tooFew.settings.slots, Settings::MIN_SLOTS);
    EXPECT_EQ(tooFew.weekPlan[0].slots.size(), static_cast<std::size_t>(Settings::MIN_SLOTS));
    EXPECT_EQ(tooMany.settings.slots, Settings::MAX_SLOTS);
    EXPECT_EQ(tooMany.weekPlan[6].slots.size(), static_cast<std::size_t>(Settings::MAX_SLOTS));
}

TEST(PlannerDocumentTest, SlotsAreTruncatedOrPaddedToTheSlotCount)
{
    const nlohmann::json json = {
        { "settings", { { "slots", 3 } } },
        { "weekPlan", nlohmann::json::array({
            { { "weekday", "monday" }, { "slots", nlohmann::json::array({ nullptr, nullptr, nullptr, { { "name", "Lost" }, { "difficulty", "hard" } } }) } },
            { { "weekday", "tuesday" }, { "slots", nlohmann::json::array({ { { "name", "Run" }, { "difficulty", "medium" } } }) } }
        }) }
    };

    const auto document = PlannerDocument::fromJson(json);

    EXPECT_EQ(document.weekPlan[0].slots.size(), 3U);
    EXPECT_EQ(document.weekPlan[0].totalLoad(), 0);
    ASSERT_EQ(document.weekPlan[1].slots.size(), 3U);
    ASSERT_TRUE(document.weekPlan[1].slots[0].has_value());
    EXPECT_FALSE(document.weekPlan[1].slots[1].has_value());
    EXPECT_FALSE(document.weekPlan[1].slots[2].has_value());
}

TEST(PlannerDocumentTest, UnknownDifficultyFallsBackToNormal)
{
    const nlohmann::json json = {
        { "activities", nlohmann::json::array({ { { "name", "Odd" }, { "difficulty", "extreme" } } }) }
    };

    const auto document = PlannerDocument::fromJson(json);

    ASSERT_EQ(document.activities.size(), 1U);
    EXPECT_EQ(document.activities[0].difficulty, Difficulty::NORMAL);
}

TEST(PlannerDocumentTest, UnparsableLastUpdateFallsBackToTheEpochInsteadOfFailing)
{
    const auto document = PlannerDocument::fromJson({ { "lastUpdate", "yesterday" } });

    EXPECT_EQ(document.lastUpdate(), PlannerDocument::TimePoint{});
}

TEST(PlannerDocumentTest, NonObjectJsonIsRejected)
{
    EXPECT_THROW(std::ignore = PlannerDocument::fromJson(nlohmann::json::array()), std::invalid_argument);
    EXPECT_THROW(std::ignore = PlannerDocument::fromJson(nlohmann::json(nullptr)), std::invalid_argument);
    EXPECT_THROW(std::ignore = PlannerDocument::fromJson(nlohmann::json("text")), std::invalid_argument);
}

TEST(PlannerDocumentTest, WrongTypesAreRejected)
{
    EXPECT_THROW(std::ignore = PlannerDocument::fromJson({ { "settings", { { "slots", "many" } } } }), std::exception);
    EXPECT_THROW(std::ignore = PlannerDocument::fromJson({ { "settings", nlohmann::json::object() } }), std::exception);
    EXPECT_THROW(std::ignore = PlannerDocument::fromJson({ { "activities", nlohmann::json::array({ { { "name", 5 } } }) } }), std::exception);
    EXPECT_THROW(std::ignore = PlannerDocument::fromJson({ { "lastUpdate", 12 } }), std::exception);
}

// Older versions read the payload directly, so its shape must stay what they expect.
TEST(PlannerDocumentTest, PayloadKeepsTheShapeReadByEarlierVersions)
{
    const auto json = makeSampleDocument(10).toJson();

    ASSERT_TRUE(json.contains("settings"));
    EXPECT_EQ(json.at("settings").at("slots"), Settings::DEFAULT_SLOTS);

    ASSERT_EQ(json.at("activities").size(), 2U);
    EXPECT_EQ(json.at("activities")[0], (nlohmann::json{ { "name", "Gym" }, { "difficulty", "hard" } }));

    ASSERT_EQ(json.at("weekPlan").size(), 7U);
    const auto& monday = json.at("weekPlan")[0];
    EXPECT_EQ(monday.at("weekday"), "monday");
    ASSERT_EQ(monday.at("slots").size(), 4U);
    EXPECT_EQ(monday.at("slots")[0], (nlohmann::json{ { "name", "Gym" }, { "difficulty", "hard" } }));
    EXPECT_TRUE(monday.at("slots")[1].is_null());
}

TEST(PlannerDocumentTest, RoundTripIsStable)
{
    const auto document = makeSampleDocument(42);

    EXPECT_EQ(roundTrip(document), document.toJson());
}
