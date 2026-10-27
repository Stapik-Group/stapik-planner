#include "core/model/PlannerDocument.hpp"

#include "support/PlannerTestDocuments.hpp"

#include "stapik/sync/SyncCoordinator.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <optional>
#include <set>
#include <stdexcept>
#include <tuple>
#include <string>

namespace
{
    using planner::test::at;
    using planner::test::makeSampleDocument;

    static_assert(stapik::document::PersistableDocument<PlannerDocument>);
    static_assert(stapik::sync::SyncableDocument<PlannerDocument>);

    // A document as written before the activities had ids: only names and difficulties.
    nlohmann::json legacyDocumentJson()
    {
        return {
            { "activities", nlohmann::json::array({
                { { "name", "Gym" }, { "difficulty", "hard" } },
                { { "name", "Reading" }, { "difficulty", "light" } }
            }) },
            { "weekPlan", nlohmann::json::array({
                { { "weekday", "monday" }, { "slots", nlohmann::json::array({ nullptr, nullptr, { { "name", "Reading" }, { "difficulty", "light" } } }) } }
            }) }
        };
    }

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
        { "activities", nlohmann::json::array({
            { { "name", "Run" }, { "difficulty", "medium" } },
            { { "name", "Lost" }, { "difficulty", "hard" } }
        }) },
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

TEST(PlannerDocumentTest, UnknownDifficultyMakesTheDocumentUnreadableInsteadOfDowngradingIt)
{
    const nlohmann::json json = {
        { "activities", nlohmann::json::array({ { { "name", "Odd" }, { "difficulty", "extreme" } } }) }
    };

    EXPECT_THROW(std::ignore = PlannerDocument::fromJson(json), std::invalid_argument);
}

TEST(PlannerDocumentTest, UnknownWeekdayMakesTheDocumentUnreadableInsteadOfOverwritingMonday)
{
    const nlohmann::json json = {
        { "weekPlan", nlohmann::json::array({
            { { "weekday", "funday" }, { "slots", nlohmann::json::array() } }
        }) }
    };

    EXPECT_THROW(std::ignore = PlannerDocument::fromJson(json), std::invalid_argument);
}

TEST(PlannerDocumentTest, EveryDifficultyAndWeekdayWrittenByTheDocumentIsReadBack)
{
    for (const auto difficulty : { Difficulty::LIGHT, Difficulty::NORMAL, Difficulty::MEDIUM, Difficulty::HARD })
        EXPECT_EQ(Activity::deserializeDifficulty(Activity::serializeDifficulty(difficulty)), difficulty);

    for (const auto weekday : { Weekday::MONDAY, Weekday::TUESDAY, Weekday::WEDNESDAY, Weekday::THURSDAY, Weekday::FRIDAY, Weekday::SATURDAY, Weekday::SUNDAY })
        EXPECT_EQ(DayPlan::deserializeWeekday(DayPlan::serializeWeekday(weekday)), weekday);
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
    EXPECT_EQ(json.at("activities")[0].at("name"), "Gym");
    EXPECT_EQ(json.at("activities")[0].at("difficulty"), "hard");

    ASSERT_EQ(json.at("weekPlan").size(), 7U);
    const auto& monday = json.at("weekPlan")[0];
    EXPECT_EQ(monday.at("weekday"), "monday");
    ASSERT_EQ(monday.at("slots").size(), 4U);
    EXPECT_EQ(monday.at("slots")[0].at("name"), "Gym");
    EXPECT_EQ(monday.at("slots")[0].at("difficulty"), "hard");
    EXPECT_TRUE(monday.at("slots")[1].is_null());
}

TEST(PlannerDocumentTest, ActivitiesAndTheirSlotsAreWrittenWithTheIds)
{
    const auto json = makeSampleDocument(10).toJson();

    EXPECT_EQ(json.at("activities")[0].at("id"), "gym");
    EXPECT_EQ(json.at("activities")[1].at("id"), "reading");
    EXPECT_EQ(json.at("weekPlan")[0].at("slots")[0].at("id"), "gym");
    EXPECT_EQ(json.at("weekPlan")[0].at("slots")[2].at("id"), "reading");
}

TEST(PlannerDocumentTest, DocumentWithoutIdsGetsUniqueIdsAndItsSlotsAreLinkedToTheCatalog)
{
    const auto legacy = legacyDocumentJson();

    const auto document = PlannerDocument::fromJson(legacy);

    ASSERT_EQ(document.activities.size(), 2U);
    EXPECT_FALSE(document.activities[0].id.empty());
    EXPECT_FALSE(document.activities[1].id.empty());
    EXPECT_NE(document.activities[0].id, document.activities[1].id);
    ASSERT_TRUE(document.weekPlan[0].slots[2].has_value());
    EXPECT_EQ(document.weekPlan[0].slots[2]->id, document.activities[1].id);
    EXPECT_EQ(document.weekPlan[0].slots[2]->name, "Reading");
}

TEST(PlannerDocumentTest, GeneratedIdsStayStableOnceTheDocumentIsLoaded)
{
    const auto loaded = PlannerDocument::fromJson(legacyDocumentJson());

    EXPECT_EQ(PlannerDocument::fromJson(loaded.toJson()).toJson(), loaded.toJson());
}

TEST(PlannerDocumentTest, SlotWithoutIdPrefersTheActivityWithTheSameNameAndDifficulty)
{
    const nlohmann::json json = {
        { "activities", nlohmann::json::array({
            { { "name", "Run" }, { "difficulty", "light" } },
            { { "name", "Run" }, { "difficulty", "hard" } }
        }) },
        { "weekPlan", nlohmann::json::array({
            { { "weekday", "monday" }, { "slots", nlohmann::json::array({ { { "name", "Run" }, { "difficulty", "hard" } } }) } }
        }) }
    };

    const auto document = PlannerDocument::fromJson(json);

    ASSERT_TRUE(document.weekPlan[0].slots[0].has_value());
    EXPECT_EQ(document.weekPlan[0].slots[0]->id, document.activities[1].id);
}

TEST(PlannerDocumentTest, SlotWithoutIdFallsBackToTheFirstActivityWithTheSameNameAndTakesItsDifficulty)
{
    const nlohmann::json json = {
        { "activities", nlohmann::json::array({
            { { "name", "Run" }, { "difficulty", "light" } },
            { { "name", "Run" }, { "difficulty", "hard" } }
        }) },
        { "weekPlan", nlohmann::json::array({
            { { "weekday", "monday" }, { "slots", nlohmann::json::array({ { { "name", "Run" }, { "difficulty", "medium" } } }) } }
        }) }
    };

    const auto document = PlannerDocument::fromJson(json);

    ASSERT_TRUE(document.weekPlan[0].slots[0].has_value());
    EXPECT_EQ(document.weekPlan[0].slots[0]->id, document.activities[0].id);
    EXPECT_EQ(document.weekPlan[0].slots[0]->difficulty, Difficulty::LIGHT);
}

TEST(PlannerDocumentTest, SlotsOfActivitiesMissingFromTheCatalogAreDropped)
{
    const nlohmann::json json = {
        { "activities", nlohmann::json::array({ { { "id", "gym" }, { "name", "Gym" }, { "difficulty", "hard" } } }) },
        { "weekPlan", nlohmann::json::array({
            { { "weekday", "monday" }, { "slots", nlohmann::json::array({
                { { "name", "Renamed long ago" }, { "difficulty", "hard" } },
                { { "id", "removed" }, { "name", "Gym" }, { "difficulty", "hard" } },
                { { "id", "gym" }, { "name", "Gym" }, { "difficulty", "hard" } }
            }) } }
        }) }
    };

    const auto document = PlannerDocument::fromJson(json);

    EXPECT_FALSE(document.weekPlan[0].slots[0].has_value());
    EXPECT_FALSE(document.weekPlan[0].slots[1].has_value());
    ASSERT_TRUE(document.weekPlan[0].slots[2].has_value());
    EXPECT_EQ(document.weekPlan[0].slots[2]->id, "gym");
}

TEST(PlannerDocumentTest, SlotTakesTheCurrentNameAndDifficultyOfItsActivity)
{
    const nlohmann::json json = {
        { "activities", nlohmann::json::array({ { { "id", "gym" }, { "name", "Strength" }, { "difficulty", "hard" } } }) },
        { "weekPlan", nlohmann::json::array({
            { { "weekday", "monday" }, { "slots", nlohmann::json::array({ { { "id", "gym" }, { "name", "Gym" }, { "difficulty", "light" } } }) } }
        }) }
    };

    const auto document = PlannerDocument::fromJson(json);

    ASSERT_TRUE(document.weekPlan[0].slots[0].has_value());
    EXPECT_EQ(document.weekPlan[0].slots[0]->name, "Strength");
    EXPECT_EQ(document.weekPlan[0].slots[0]->difficulty, Difficulty::HARD);
    EXPECT_EQ(document.weekPlan[0].totalLoad(), 50);
}

TEST(PlannerDocumentTest, MissingAndRepeatedIdsAreReplacedKeepingTheFirstOne)
{
    const nlohmann::json json = {
        { "activities", nlohmann::json::array({
            { { "id", "same" }, { "name", "First" }, { "difficulty", "light" } },
            { { "id", "same" }, { "name", "Second" }, { "difficulty", "light" } },
            { { "id", "" }, { "name", "Third" }, { "difficulty", "light" } }
        }) },
        { "weekPlan", nlohmann::json::array({
            { { "weekday", "monday" }, { "slots", nlohmann::json::array({ { { "id", "same" }, { "name", "First" }, { "difficulty", "light" } } }) } }
        }) }
    };

    const auto document = PlannerDocument::fromJson(json);

    ASSERT_EQ(document.activities.size(), 3U);
    EXPECT_EQ(document.activities[0].id, "same");
    EXPECT_FALSE(document.activities[1].id.empty());
    EXPECT_NE(document.activities[1].id, "same");
    EXPECT_FALSE(document.activities[2].id.empty());
    EXPECT_NE(document.activities[1].id, document.activities[2].id);
    ASSERT_TRUE(document.weekPlan[0].slots[0].has_value());
    EXPECT_EQ(document.weekPlan[0].slots[0]->name, "First");
}

TEST(PlannerDocumentTest, RoundTripIsStable)
{
    const auto document = makeSampleDocument(42);

    EXPECT_EQ(roundTrip(document), document.toJson());
}

TEST(PlannerDocumentTest, ADayIsEmptyOnlyWhenNoSlotIsPlanned)
{
    auto document = makeSampleDocument(10);

    EXPECT_FALSE(document.weekPlan[0].isEmpty());
    EXPECT_TRUE(document.weekPlan[1].isEmpty());
    document.weekPlan[1].slots[2] = document.activities[0];
    EXPECT_FALSE(document.weekPlan[1].isEmpty());
}

TEST(PlannerDocumentTest, DocumentWithoutPlanSectionsHasTheDefaultPlanOnly)
{
    const auto document = PlannerDocument::fromJson(nlohmann::json::object());

    EXPECT_EQ(document.activePlanId, PlannerDocument::DEFAULT_PLAN_ID);
    EXPECT_TRUE(document.activePlanName.empty());
    EXPECT_TRUE(document.savedPlans.empty());
}

TEST(PlannerDocumentTest, SavedPlansSurviveTheRoundTrip)
{
    auto document = makeSampleDocument(10);
    document.activePlanName = "Work";
    document.savedPlans.push_back(SavedPlan{ .id = "holiday", .name = "Holiday", .weekPlan = PlannerDocument::emptyWeekPlan(document.settings.slots) });
    document.savedPlans[0].weekPlan[2].slots[1] = document.activities[1];

    const auto reloaded = PlannerDocument::fromJson(document.toJson());

    EXPECT_EQ(reloaded.activePlanName, "Work");
    ASSERT_EQ(reloaded.savedPlans.size(), 1U);
    EXPECT_EQ(reloaded.savedPlans[0].id, "holiday");
    EXPECT_EQ(reloaded.savedPlans[0].name, "Holiday");
    EXPECT_EQ(reloaded.savedPlans[0].weekPlan, document.savedPlans[0].weekPlan);
    EXPECT_EQ(reloaded.toJson(), document.toJson());
}

TEST(PlannerDocumentTest, SlotsOfSavedPlansTakeTheCurrentActivityFromTheCatalog)
{
    const nlohmann::json json = {
        { "activities", nlohmann::json::array({ { { "id", "gym" }, { "name", "Strength" }, { "difficulty", "hard" } } }) },
        { "savedPlans", nlohmann::json::array({
            { { "id", "holiday" }, { "name", "Holiday" }, { "weekPlan", nlohmann::json::array({
                { { "weekday", "monday" }, { "slots", nlohmann::json::array({ { { "id", "gym" }, { "name", "Gym" }, { "difficulty", "light" } } }) } }
            }) } }
        }) }
    };

    const auto document = PlannerDocument::fromJson(json);

    ASSERT_EQ(document.savedPlans.size(), 1U);
    ASSERT_TRUE(document.savedPlans[0].weekPlan[0].slots[0].has_value());
    EXPECT_EQ(document.savedPlans[0].weekPlan[0].slots[0]->name, "Strength");
    EXPECT_EQ(document.savedPlans[0].weekPlan[0].slots[0]->difficulty, Difficulty::HARD);
}

TEST(PlannerDocumentTest, MissingAndRepeatedPlanIdsAreReplacedKeepingTheActiveOne)
{
    const nlohmann::json json = {
        { "plan", { { "id", "work" }, { "name", "Work" } } },
        { "savedPlans", nlohmann::json::array({
            { { "id", "work" }, { "name", "Same id as the active plan" } },
            { { "name", "No id" } },
            { { "id", "holiday" }, { "name", "Holiday" } }
        }) }
    };

    const auto document = PlannerDocument::fromJson(json);

    EXPECT_EQ(document.activePlanId, "work");
    ASSERT_EQ(document.savedPlans.size(), 3U);
    std::set<std::string> ids = { document.activePlanId };
    for (const auto& plan : document.savedPlans)
    {
        EXPECT_FALSE(plan.id.empty());
        ids.insert(plan.id);
    }
    EXPECT_EQ(ids.size(), 4U);
    EXPECT_EQ(document.savedPlans[2].id, "holiday");
}

TEST(PlannerDocumentTest, WrongPlanTypesAreRejected)
{
    EXPECT_THROW(std::ignore = PlannerDocument::fromJson({ { "plan", 5 } }), std::exception);
    EXPECT_THROW(std::ignore = PlannerDocument::fromJson({ { "savedPlans", nlohmann::json::array({ 7 }) } }), std::exception);
}
