#include "core/model/PlannerModel.hpp"

#include "support/ScopedEnvironment.hpp"

#include "stapik/app/AppContext.hpp"

#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
#include <memory>
#include <set>
#include <string>

namespace
{
    namespace fs = std::filesystem;

    class PlannerModelTest : public testing::Test
    {
    protected:
        void SetUp() override
        {
            auto pattern = (fs::temp_directory_path() / "stapik-planner-model-XXXXXX").string();
            ASSERT_NE(mkdtemp(pattern.data()), nullptr);
            m_directory = pattern;
            setenv("XDG_DATA_HOME", m_directory.c_str(), 1);

            stapik::app::AppInfo appInfo;
            appInfo.internalName = "stapikplannermodeltest";
            stapik::app::AppContext::resetForTests();
            stapik::app::AppContext::initialize(appInfo);

            m_model = std::make_unique<PlannerModel>();
        }

        void TearDown() override
        {
            m_model.reset();
            stapik::app::AppContext::resetForTests();
            std::error_code errorCode;
            fs::remove_all(m_directory, errorCode);
        }

        // Plans the activity at the given catalog position in the first slot of the given weekday.
        void planInFirstSlot(const Weekday weekday, const std::size_t activityIndex) const
        {
            DayPlan day = m_model->weekPlan()[static_cast<std::size_t>(weekday)];
            day.slots[0] = m_model->activities()[activityIndex];
            m_model->updateDay(weekday, day);
        }

        [[nodiscard]] const std::optional<Activity>& firstSlotOf(const Weekday weekday) const
        {
            return m_model->weekPlan()[static_cast<std::size_t>(weekday)].slots[0];
        }

        stapik::test::ScopedEnvironment m_dataHome{ "XDG_DATA_HOME" };
        fs::path m_directory;
        std::unique_ptr<PlannerModel> m_model;
    };
}

TEST_F(PlannerModelTest, RenamingAnActivityUpdatesItsSlotsInThePlan)
{
    m_model->addActivity(Activity{ .name = "Gym", .difficulty = Difficulty::HARD });
    planInFirstSlot(Weekday::MONDAY, 0);

    m_model->updateActivity(0, Activity{ .name = "Strength", .difficulty = Difficulty::HARD });

    ASSERT_TRUE(firstSlotOf(Weekday::MONDAY).has_value());
    EXPECT_EQ(firstSlotOf(Weekday::MONDAY)->name, "Strength");
}

TEST_F(PlannerModelTest, ChangingTheDifficultyOfAnActivityUpdatesItsSlotsInThePlan)
{
    m_model->addActivity(Activity{ .name = "Gym", .difficulty = Difficulty::LIGHT });
    planInFirstSlot(Weekday::MONDAY, 0);

    m_model->updateActivity(0, Activity{ .name = "Gym", .difficulty = Difficulty::HARD });

    ASSERT_TRUE(firstSlotOf(Weekday::MONDAY).has_value());
    EXPECT_EQ(firstSlotOf(Weekday::MONDAY)->difficulty, Difficulty::HARD);
    EXPECT_EQ(m_model->weekPlan()[0].totalLoad(), 50);
}

TEST_F(PlannerModelTest, RemovingAnActivityClearsItsSlotsAndKeepsTheOthers)
{
    m_model->addActivity(Activity{ .name = "Gym", .difficulty = Difficulty::HARD });
    m_model->addActivity(Activity{ .name = "Reading", .difficulty = Difficulty::LIGHT });
    planInFirstSlot(Weekday::MONDAY, 0);
    planInFirstSlot(Weekday::TUESDAY, 1);

    m_model->removeActivity(0);

    EXPECT_FALSE(firstSlotOf(Weekday::MONDAY).has_value());
    ASSERT_TRUE(firstSlotOf(Weekday::TUESDAY).has_value());
    EXPECT_EQ(firstSlotOf(Weekday::TUESDAY)->name, "Reading");
}

TEST_F(PlannerModelTest, RenamedActivityIsStillPlannedAfterTheApplicationIsRestarted)
{
    m_model->addActivity(Activity{ .name = "Gym", .difficulty = Difficulty::HARD });
    planInFirstSlot(Weekday::MONDAY, 0);
    m_model->updateActivity(0, Activity{ .name = "Strength", .difficulty = Difficulty::HARD });

    const PlannerModel reloaded;

    ASSERT_TRUE(reloaded.weekPlan()[0].slots[0].has_value());
    EXPECT_EQ(reloaded.weekPlan()[0].slots[0]->name, "Strength");
}

TEST_F(PlannerModelTest, AddedActivitiesGetUniqueIds)
{
    m_model->addActivity(Activity{ .name = "Gym", .difficulty = Difficulty::HARD });
    m_model->addActivity(Activity{ .id = "chosen-by-the-caller", .name = "Gym", .difficulty = Difficulty::HARD });

    const auto& activities = m_model->activities();
    ASSERT_EQ(activities.size(), 2U);
    EXPECT_FALSE(activities[0].id.empty());
    EXPECT_FALSE(activities[1].id.empty());
    EXPECT_NE(activities[0].id, activities[1].id);
    EXPECT_NE(activities[1].id, "chosen-by-the-caller");
}

TEST_F(PlannerModelTest, UpdatingAnActivityKeepsItsId)
{
    m_model->addActivity(Activity{ .name = "Gym", .difficulty = Difficulty::HARD });
    const auto originalId = m_model->activities()[0].id;

    m_model->updateActivity(0, Activity{ .id = "ignored", .name = "Strength", .difficulty = Difficulty::LIGHT });

    EXPECT_EQ(m_model->activities()[0].id, originalId);
    EXPECT_EQ(m_model->activities()[0].name, "Strength");
}

TEST_F(PlannerModelTest, RemovingAnActivityDoesNotTouchSlotsOfAnotherActivityWithTheSameName)
{
    m_model->addActivity(Activity{ .name = "Run", .difficulty = Difficulty::LIGHT });
    m_model->addActivity(Activity{ .name = "Run", .difficulty = Difficulty::LIGHT });
    planInFirstSlot(Weekday::MONDAY, 0);
    planInFirstSlot(Weekday::TUESDAY, 1);
    const auto survivingId = m_model->activities()[1].id;

    m_model->removeActivity(0);

    EXPECT_FALSE(firstSlotOf(Weekday::MONDAY).has_value());
    ASSERT_TRUE(firstSlotOf(Weekday::TUESDAY).has_value());
    EXPECT_EQ(firstSlotOf(Weekday::TUESDAY)->id, survivingId);
}

TEST_F(PlannerModelTest, RenamingOneOfTwoActivitiesWithTheSameNameOnlyChangesItsOwnSlots)
{
    m_model->addActivity(Activity{ .name = "Run", .difficulty = Difficulty::LIGHT });
    m_model->addActivity(Activity{ .name = "Run", .difficulty = Difficulty::LIGHT });
    planInFirstSlot(Weekday::MONDAY, 0);
    planInFirstSlot(Weekday::TUESDAY, 1);

    m_model->updateActivity(1, Activity{ .name = "Sprint", .difficulty = Difficulty::LIGHT });

    EXPECT_EQ(firstSlotOf(Weekday::MONDAY)->name, "Run");
    EXPECT_EQ(firstSlotOf(Weekday::TUESDAY)->name, "Sprint");
}

TEST_F(PlannerModelTest, WeekPlanChangeIsAnnouncedOnlyWhenSlotsWereAffected)
{
    m_model->addActivity(Activity{ .name = "Gym", .difficulty = Difficulty::HARD });
    m_model->addActivity(Activity{ .name = "Reading", .difficulty = Difficulty::LIGHT });
    planInFirstSlot(Weekday::MONDAY, 0);
    int weekPlanChanges = 0;
    m_model->signalWeekPlanChanged().connect([&weekPlanChanges] { ++weekPlanChanges; });

    m_model->updateActivity(1, Activity{ .name = "Novel", .difficulty = Difficulty::LIGHT });
    m_model->removeActivity(1);
    EXPECT_EQ(weekPlanChanges, 0);

    m_model->updateActivity(0, Activity{ .name = "Strength", .difficulty = Difficulty::HARD });
    EXPECT_EQ(weekPlanChanges, 1);

    m_model->removeActivity(0);
    EXPECT_EQ(weekPlanChanges, 2);
}

TEST_F(PlannerModelTest, ActivityIdsSurviveARestart)
{
    m_model->addActivity(Activity{ .name = "Gym", .difficulty = Difficulty::HARD });
    planInFirstSlot(Weekday::MONDAY, 0);
    const auto id = m_model->activities()[0].id;

    const PlannerModel reloaded;

    ASSERT_EQ(reloaded.activities().size(), 1U);
    EXPECT_EQ(reloaded.activities()[0].id, id);
    EXPECT_EQ(reloaded.weekPlan()[0].slots[0]->id, id);
}
