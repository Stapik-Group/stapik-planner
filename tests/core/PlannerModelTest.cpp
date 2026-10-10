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

TEST_F(PlannerModelTest, AddingAnActivityCanBeUndoneAndRedoneWithTheSameId)
{
    m_model->addActivity(Activity{ .name = "Gym", .difficulty = Difficulty::HARD });
    const auto id = m_model->activities()[0].id;

    EXPECT_TRUE(m_model->undoStack().undo());
    EXPECT_TRUE(m_model->activities().empty());

    EXPECT_TRUE(m_model->undoStack().redo());
    ASSERT_EQ(m_model->activities().size(), 1U);
    EXPECT_EQ(m_model->activities()[0].id, id);
    EXPECT_EQ(m_model->activities()[0].name, "Gym");
}

TEST_F(PlannerModelTest, EditingAnActivityCanBeUndoneTogetherWithTheCopiesInThePlan)
{
    m_model->addActivity(Activity{ .name = "Gym", .difficulty = Difficulty::HARD });
    planInFirstSlot(Weekday::MONDAY, 0);

    m_model->updateActivity(0, Activity{ .name = "Strength", .difficulty = Difficulty::LIGHT });
    ASSERT_EQ(firstSlotOf(Weekday::MONDAY)->name, "Strength");

    EXPECT_TRUE(m_model->undoStack().undo());
    EXPECT_EQ(m_model->activities()[0].name, "Gym");
    EXPECT_EQ(m_model->activities()[0].difficulty, Difficulty::HARD);
    EXPECT_EQ(firstSlotOf(Weekday::MONDAY)->name, "Gym");
    EXPECT_EQ(firstSlotOf(Weekday::MONDAY)->difficulty, Difficulty::HARD);

    EXPECT_TRUE(m_model->undoStack().redo());
    EXPECT_EQ(m_model->activities()[0].name, "Strength");
    EXPECT_EQ(firstSlotOf(Weekday::MONDAY)->name, "Strength");
}

TEST_F(PlannerModelTest, RemovingAnActivityCanBeUndoneAtItsPositionTogetherWithItsSlots)
{
    m_model->addActivity(Activity{ .name = "Gym", .difficulty = Difficulty::HARD });
    m_model->addActivity(Activity{ .name = "Reading", .difficulty = Difficulty::LIGHT });
    m_model->addActivity(Activity{ .name = "Running", .difficulty = Difficulty::MEDIUM });
    planInFirstSlot(Weekday::MONDAY, 1);
    planInFirstSlot(Weekday::TUESDAY, 2);
    const auto readingId = m_model->activities()[1].id;

    m_model->removeActivity(1);
    ASSERT_EQ(m_model->activities().size(), 2U);
    ASSERT_FALSE(firstSlotOf(Weekday::MONDAY).has_value());

    EXPECT_TRUE(m_model->undoStack().undo());

    ASSERT_EQ(m_model->activities().size(), 3U);
    EXPECT_EQ(m_model->activities()[0].name, "Gym");
    EXPECT_EQ(m_model->activities()[1].name, "Reading");
    EXPECT_EQ(m_model->activities()[1].id, readingId);
    EXPECT_EQ(m_model->activities()[2].name, "Running");
    ASSERT_TRUE(firstSlotOf(Weekday::MONDAY).has_value());
    EXPECT_EQ(firstSlotOf(Weekday::MONDAY)->id, readingId);
    EXPECT_EQ(firstSlotOf(Weekday::TUESDAY)->name, "Running");
}

TEST_F(PlannerModelTest, ChangingASlotCanBeUndoneAndRedone)
{
    m_model->addActivity(Activity{ .name = "Gym", .difficulty = Difficulty::HARD });
    planInFirstSlot(Weekday::MONDAY, 0);

    EXPECT_TRUE(m_model->undoStack().undo());
    EXPECT_FALSE(firstSlotOf(Weekday::MONDAY).has_value());
    EXPECT_EQ(m_model->activities().size(), 1U) << "only the slot change is undone";

    EXPECT_TRUE(m_model->undoStack().redo());
    ASSERT_TRUE(firstSlotOf(Weekday::MONDAY).has_value());
    EXPECT_EQ(firstSlotOf(Weekday::MONDAY)->name, "Gym");
}

TEST_F(PlannerModelTest, ReducingTheSlotCountCanBeUndoneAndBringsTheDroppedSlotsBack)
{
    m_model->addActivity(Activity{ .name = "Gym", .difficulty = Difficulty::HARD });
    DayPlan monday = m_model->weekPlan()[0];
    monday.slots[3] = m_model->activities()[0];
    m_model->updateDay(Weekday::MONDAY, monday);
    ASSERT_EQ(m_model->settings().slots, 4);

    EXPECT_TRUE(m_model->updateSlotCount(3));
    ASSERT_EQ(m_model->weekPlan()[0].slots.size(), 3U);

    EXPECT_TRUE(m_model->undoStack().undo());

    EXPECT_EQ(m_model->settings().slots, 4);
    ASSERT_EQ(m_model->weekPlan()[0].slots.size(), 4U);
    ASSERT_TRUE(m_model->weekPlan()[0].slots[3].has_value());
    EXPECT_EQ(m_model->weekPlan()[0].slots[3]->name, "Gym");
}

TEST_F(PlannerModelTest, ANewChangeDropsTheStepsThatCouldBeRedone)
{
    m_model->addActivity(Activity{ .name = "Gym", .difficulty = Difficulty::HARD });
    EXPECT_TRUE(m_model->undoStack().undo());
    ASSERT_TRUE(m_model->undoStack().canRedo());

    m_model->addActivity(Activity{ .name = "Reading", .difficulty = Difficulty::LIGHT });

    EXPECT_FALSE(m_model->undoStack().canRedo());
}

TEST_F(PlannerModelTest, ChangesThatLeaveTheContentAsItIsTakeNoStepOfTheHistory)
{
    m_model->addActivity(Activity{ .name = "Gym", .difficulty = Difficulty::HARD });
    planInFirstSlot(Weekday::MONDAY, 0);
    const auto depth = m_model->undoStack().undoDepth();

    m_model->updateActivity(0, Activity{ .name = "Gym", .difficulty = Difficulty::HARD });
    m_model->updateDay(Weekday::MONDAY, m_model->weekPlan()[0]);
    EXPECT_FALSE(m_model->updateSlotCount(m_model->settings().slots));
    m_model->updateActivity(5, Activity{ .name = "Nothing", .difficulty = Difficulty::HARD });
    m_model->removeActivity(5);

    EXPECT_EQ(m_model->undoStack().undoDepth(), depth);
}

TEST_F(PlannerModelTest, UndoAndRedoAreSavedLikeAnyOtherChange)
{
    m_model->addActivity(Activity{ .name = "Gym", .difficulty = Difficulty::HARD });

    EXPECT_TRUE(m_model->undoStack().undo());
    EXPECT_TRUE(PlannerModel().activities().empty());

    EXPECT_TRUE(m_model->undoStack().redo());
    const PlannerModel reloaded;
    ASSERT_EQ(reloaded.activities().size(), 1U);
    EXPECT_EQ(reloaded.activities()[0].name, "Gym");
}

TEST_F(PlannerModelTest, ViewsAreToldAboutUndoneAndRedoneSlotChangesButNotAboutTheChangeTheyMade)
{
    m_model->addActivity(Activity{ .name = "Gym", .difficulty = Difficulty::HARD });
    int weekPlanChanges = 0;
    m_model->signalWeekPlanChanged().connect([&weekPlanChanges] { ++weekPlanChanges; });

    planInFirstSlot(Weekday::MONDAY, 0);
    EXPECT_EQ(weekPlanChanges, 0);

    EXPECT_TRUE(m_model->undoStack().undo());
    EXPECT_EQ(weekPlanChanges, 1);

    EXPECT_TRUE(m_model->undoStack().redo());
    EXPECT_EQ(weekPlanChanges, 2);
}

TEST_F(PlannerModelTest, ViewsAreToldWhichPartOfTheContentWasUndone)
{
    int activityChanges = 0;
    int settingsChanges = 0;
    m_model->signalActivitiesChanged().connect([&activityChanges] { ++activityChanges; });
    m_model->signalSettingsChanged().connect([&settingsChanges] { ++settingsChanges; });

    m_model->addActivity(Activity{ .name = "Gym", .difficulty = Difficulty::HARD });
    EXPECT_EQ(activityChanges, 1);
    EXPECT_EQ(settingsChanges, 0);

    EXPECT_FALSE(m_model->updateSlotCount(3));
    EXPECT_EQ(settingsChanges, 1);

    EXPECT_TRUE(m_model->undoStack().undo());
    EXPECT_EQ(settingsChanges, 2);
    EXPECT_EQ(activityChanges, 1) << "undoing the slot count does not touch the activities";
}

TEST_F(PlannerModelTest, TheHistoryNamesTheLastStep)
{
    m_model->addActivity(Activity{ .name = "Gym", .difficulty = Difficulty::HARD });
    const auto afterAdding = m_model->undoStack().undoDescription();
    planInFirstSlot(Weekday::MONDAY, 0);
    const auto afterPlanning = m_model->undoStack().undoDescription();

    EXPECT_FALSE(afterAdding.empty());
    EXPECT_FALSE(afterPlanning.empty());
    EXPECT_NE(afterAdding, afterPlanning);

    EXPECT_TRUE(m_model->undoStack().undo());
    EXPECT_EQ(m_model->undoStack().undoDescription(), afterAdding);
    EXPECT_EQ(m_model->undoStack().redoDescription(), afterPlanning);
}
