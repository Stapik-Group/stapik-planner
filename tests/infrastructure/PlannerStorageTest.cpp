#include "infrastructure/storage/PlannerStorage.hpp"

#include "support/FakeClock.hpp"
#include "support/PlannerTestDocuments.hpp"
#include "support/ScopedEnvironment.hpp"

#include "stapik/app/AppContext.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace
{
    namespace fs = std::filesystem;
    using planner::test::at;
    using planner::test::makeSampleDocument;

    // The file exactly as the version before the schema was introduced wrote it.
    constexpr auto LEGACY_FILE = R"({
  "lastKnownCloudUpdate": "2026-01-01T12:00:05Z",
  "lastUpdate": "2026-01-01T12:00:10Z",
  "payload": {
    "activities": [
      { "difficulty": "hard", "name": "Gym" }
    ],
    "settings": { "slots": 3 },
    "weekPlan": [
      { "slots": [ { "difficulty": "hard", "name": "Gym" }, null, null ], "weekday": "monday" },
      { "slots": [ null, null, null ], "weekday": "tuesday" }
    ]
  }
})";

    void writeText(const fs::path& path, const std::string& text)
    {
        fs::create_directories(path.parent_path());
        std::ofstream file(path);
        file << text;
    }

    std::string readText(const fs::path& path)
    {
        std::ifstream file(path);
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    class PlannerStorageTest : public testing::Test
    {
    protected:
        void SetUp() override
        {
            auto pattern = (fs::temp_directory_path() / "stapik-planner-storage-XXXXXX").string();
            ASSERT_NE(mkdtemp(pattern.data()), nullptr);
            m_directory = pattern;
            m_filePath = m_directory / "planner.json";
        }

        void TearDown() override
        {
            std::error_code errorCode;
            fs::remove_all(m_directory, errorCode);
        }

        [[nodiscard]] std::vector<fs::path> filesWithPrefix(const std::string& prefix) const
        {
            std::vector<fs::path> matches;
            for (const auto& entry : fs::directory_iterator(m_directory))
            {
                if (entry.path().filename().string().starts_with(prefix))
                    matches.push_back(entry.path());
            }

            return matches;
        }

        fs::path m_directory;
        fs::path m_filePath;
    };
}

TEST_F(PlannerStorageTest, MissingFileYieldsTheDefaultDocument)
{
    const PlannerStorage storage(m_filePath);

    EXPECT_EQ(storage.load().toJson(), PlannerDocument::createDefault().toJson());
}

TEST_F(PlannerStorageTest, SavedDocumentIsLoadedBack)
{
    const PlannerStorage storage(m_filePath);
    const auto document = makeSampleDocument(77);

    ASSERT_TRUE(storage.save(document));

    EXPECT_EQ(storage.load().toJson(), document.toJson());
}

TEST_F(PlannerStorageTest, SavedFileCarriesTheSchemaVersionAroundTheDocument)
{
    const PlannerStorage storage(m_filePath);
    const auto document = makeSampleDocument(77);

    ASSERT_TRUE(storage.save(document));

    const auto root = nlohmann::json::parse(readText(m_filePath));
    EXPECT_EQ(root.at("schemaVersion"), PlannerStorage::SCHEMA_VERSION);
    EXPECT_EQ(root.at("document"), document.toJson());
}

TEST_F(PlannerStorageTest, SecondSaveKeepsTheFirstOneAsBackup)
{
    const PlannerStorage storage(m_filePath);
    const auto first = makeSampleDocument(10);
    auto second = makeSampleDocument(20);
    second.settings.slots = 3;

    ASSERT_TRUE(storage.save(first));
    ASSERT_TRUE(storage.save(second));

    const auto backup = nlohmann::json::parse(readText(fs::path(m_filePath).concat(".bak")));
    EXPECT_EQ(backup.at("document"), first.toJson());
}

TEST_F(PlannerStorageTest, LegacyFileIsMigratedWithoutLosingContent)
{
    writeText(m_filePath, LEGACY_FILE);
    const PlannerStorage storage(m_filePath);

    const auto document = storage.load();

    EXPECT_EQ(document.lastUpdate(), at(10));
    EXPECT_EQ(document.settings.slots, 3);
    ASSERT_EQ(document.activities.size(), 1U);
    EXPECT_EQ(document.activities[0].name, "Gym");
    EXPECT_EQ(document.activities[0].difficulty, Difficulty::HARD);
    ASSERT_TRUE(document.weekPlan[0].slots[0].has_value());
    EXPECT_EQ(document.weekPlan[0].slots[0]->name, "Gym");
    EXPECT_FALSE(document.weekPlan[0].slots[1].has_value());
    EXPECT_EQ(document.weekPlan[1].slots.size(), 3U);
    EXPECT_EQ(document.weekPlan[6].slots.size(), 3U);
}

TEST_F(PlannerStorageTest, LegacyCloudBaselineIsDropped)
{
    writeText(m_filePath, LEGACY_FILE);
    const PlannerStorage storage(m_filePath);

    EXPECT_FALSE(storage.load().lastKnownCloudUpdate().has_value());
}

TEST_F(PlannerStorageTest, LegacyFileIsLeftUntouchedByLoadAndBecomesTheBackupOnSave)
{
    writeText(m_filePath, LEGACY_FILE);
    const PlannerStorage storage(m_filePath);

    const auto document = storage.load();
    EXPECT_EQ(readText(m_filePath), LEGACY_FILE);

    ASSERT_TRUE(storage.save(document));

    EXPECT_EQ(readText(fs::path(m_filePath).concat(".bak")), LEGACY_FILE);
    EXPECT_EQ(nlohmann::json::parse(readText(m_filePath)).at("schemaVersion"), PlannerStorage::SCHEMA_VERSION);
}

TEST_F(PlannerStorageTest, MigratedFileLoadsTheSameAfterBeingSaved)
{
    writeText(m_filePath, LEGACY_FILE);
    const PlannerStorage storage(m_filePath);
    const auto migrated = storage.load();

    ASSERT_TRUE(storage.save(migrated));

    EXPECT_EQ(storage.load().toJson(), migrated.toJson());
}

TEST_F(PlannerStorageTest, InvalidJsonIsMovedAsideAndReplacedWithTheDefaultDocument)
{
    writeText(m_filePath, "{ this is not json");
    const PlannerStorage storage(m_filePath);

    const auto document = storage.load();

    EXPECT_EQ(document.toJson(), PlannerDocument::createDefault().toJson());
    EXPECT_FALSE(fs::exists(m_filePath));
    const auto quarantined = filesWithPrefix("planner.json.corrupt-");
    ASSERT_EQ(quarantined.size(), 1U);
    EXPECT_EQ(readText(quarantined.front()), "{ this is not json");
}

TEST_F(PlannerStorageTest, ValidJsonWithInvalidContentIsMovedAside)
{
    writeText(m_filePath, R"({ "schemaVersion": 1, "document": { "settings": { "slots": "many" } } })");
    const PlannerStorage storage(m_filePath);

    const auto document = storage.load();

    EXPECT_EQ(document.toJson(), PlannerDocument::createDefault().toJson());
    EXPECT_FALSE(fs::exists(m_filePath));
    EXPECT_EQ(filesWithPrefix("planner.json.corrupt-").size(), 1U);
}

TEST_F(PlannerStorageTest, FileFromANewerVersionIsSetAsideSoThatSavingDoesNotOverwriteIt)
{
    const std::string newerFile = R"({ "schemaVersion": 99, "document": { "future": true } })";
    writeText(m_filePath, newerFile);
    const PlannerStorage storage(m_filePath);

    const auto document = storage.load();
    ASSERT_TRUE(storage.save(document));

    EXPECT_EQ(document.toJson(), PlannerDocument::createDefault().toJson());
    const auto setAside = filesWithPrefix("planner.json.newer-version-");
    ASSERT_EQ(setAside.size(), 1U);
    EXPECT_EQ(readText(setAside.front()), newerFile);
    EXPECT_EQ(nlohmann::json::parse(readText(m_filePath)).at("schemaVersion"), PlannerStorage::SCHEMA_VERSION);
}

TEST_F(PlannerStorageTest, LegacyFileThatCannotBeMigratedIsSetAside)
{
    writeText(m_filePath, "[1, 2, 3]");
    const PlannerStorage storage(m_filePath);

    const auto document = storage.load();

    EXPECT_EQ(document.toJson(), PlannerDocument::createDefault().toJson());
    EXPECT_FALSE(fs::exists(m_filePath));
    const auto setAside = filesWithPrefix("planner.json.migration-failed-");
    ASSERT_EQ(setAside.size(), 1U);
    EXPECT_EQ(readText(setAside.front()), "[1, 2, 3]");
}

TEST_F(PlannerStorageTest, DefaultFilePathLivesInTheUserDataDirectoryOfTheApplication)
{
    stapik::test::ScopedEnvironment dataHome("XDG_DATA_HOME");
    setenv("XDG_DATA_HOME", m_directory.c_str(), 1);

    stapik::app::AppInfo appInfo;
    appInfo.internalName = "stapikplannertest";
    stapik::app::AppContext::resetForTests();
    stapik::app::AppContext::initialize(appInfo);

    EXPECT_EQ(PlannerStorage::defaultFilePath(), m_directory / "stapikplannertest" / "planner.json");

    stapik::app::AppContext::resetForTests();
}

TEST_F(PlannerStorageTest, ABackupIsNotRotatedAgainBeforeTheIntervalPasses)
{
    stapik::test::FakeClock clock(std::chrono::system_clock::now());
    const PlannerStorage storage(m_filePath, clock);
    const auto first = makeSampleDocument(1);
    const auto second = makeSampleDocument(2);
    const auto third = makeSampleDocument(3);

    ASSERT_TRUE(storage.save(first));
    ASSERT_TRUE(storage.save(second));
    ASSERT_TRUE(storage.save(third));

    const auto backups = storage.listBackups();
    ASSERT_EQ(backups.size(), 1U);
    EXPECT_EQ(storage.loadBackup(backups[0]).document->toJson(), first.toJson());
    EXPECT_EQ(storage.load().toJson(), third.toJson());
}

TEST_F(PlannerStorageTest, ABackupIsRotatedAgainOnceTheIntervalHasPassed)
{
    stapik::test::FakeClock clock(std::chrono::system_clock::now());
    const PlannerStorage storage(m_filePath, clock);
    const auto first = makeSampleDocument(1);
    const auto second = makeSampleDocument(2);
    const auto third = makeSampleDocument(3);
    const auto fourth = makeSampleDocument(4);

    ASSERT_TRUE(storage.save(first));
    ASSERT_TRUE(storage.save(second));
    ASSERT_TRUE(storage.save(third));
    clock.advance(PlannerStorage::BACKUP_INTERVAL + std::chrono::minutes{ 1 });
    ASSERT_TRUE(storage.save(fourth));

    const auto backups = storage.listBackups();
    ASSERT_EQ(backups.size(), 2U);
    EXPECT_EQ(storage.loadBackup(backups[0]).document->toJson(), third.toJson());
    EXPECT_EQ(storage.loadBackup(backups[1]).document->toJson(), first.toJson());
}

TEST_F(PlannerStorageTest, FileWithAnUnknownDifficultyIsMovedAsideInsteadOfBeingDowngraded)
{
    const std::string futureFile = R"({ "schemaVersion": 1, "document": { "activities": [ { "name": "Odd", "difficulty": "extreme" } ] } })";
    writeText(m_filePath, futureFile);
    const PlannerStorage storage(m_filePath);

    const auto document = storage.load();

    EXPECT_EQ(document.toJson(), PlannerDocument::createDefault().toJson());
    const auto quarantined = filesWithPrefix("planner.json.corrupt-");
    ASSERT_EQ(quarantined.size(), 1U);
    EXPECT_EQ(readText(quarantined.front()), futureFile);
}

TEST_F(PlannerStorageTest, FileWithAnUnknownWeekdayIsMovedAsideInsteadOfOverwritingMonday)
{
    writeText(m_filePath, R"({ "schemaVersion": 1, "document": { "weekPlan": [ { "weekday": "funday", "slots": [] } ] } })");
    const PlannerStorage storage(m_filePath);

    const auto document = storage.load();

    EXPECT_EQ(document.toJson(), PlannerDocument::createDefault().toJson());
    EXPECT_EQ(filesWithPrefix("planner.json.corrupt-").size(), 1U);
}
