#include "core/model/PlannerModel.hpp"

#include "infrastructure/storage/PlannerStorage.hpp"

#include "support/FakeCloudStorage.hpp"
#include "support/MainLoopPump.hpp"
#include "support/PlannerTestDocuments.hpp"
#include "support/ScopedEnvironment.hpp"

#include "stapik/app/AppContext.hpp"
#include "stapik/sync/SyncEnvelope.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <optional>
#include <vector>

namespace
{
    namespace fs = std::filesystem;
    using namespace std::chrono_literals;
    using planner::test::at;
    using planner::test::cloudDocumentOf;
    using planner::test::makeSampleDocument;
    using stapik::sync::AsyncSyncOptions;
    using stapik::sync::CloudSessionHooks;
    using stapik::sync::SyncState;
    using stapik::sync::SyncStatus;
    using stapik::test::FakeCloudStorage;
    using stapik::test::pumpUntil;

    class StorageProxy final : public ICloudStorage
    {
    public:
        explicit StorageProxy(FakeCloudStorage& target) :
            m_target(target)
        {}

        [[nodiscard]] std::optional<CloudDocument> loadDocument() const override
        {
            return m_target.loadDocument();
        }

        [[nodiscard]] CloudWriteResult saveDocument(const nlohmann::json& data, const std::chrono::system_clock::time_point clientLastKnownUpdate) const override
        {
            return m_target.saveDocument(data, clientLastKnownUpdate);
        }

    private:
        FakeCloudStorage& m_target;
    };

    CloudStorageConfig validConfig()
    {
        return CloudStorageConfig{ .apiUrl = "https://example.com", .apiKey = "key" };
    }

    class PlannerModelCloudTest : public testing::Test
    {
    protected:
        void SetUp() override
        {
            auto pattern = (fs::temp_directory_path() / "stapik-planner-cloud-XXXXXX").string();
            ASSERT_NE(mkdtemp(pattern.data()), nullptr);
            m_directory = pattern;
            setenv("XDG_DATA_HOME", m_directory.c_str(), 1);

            stapik::app::AppInfo appInfo;
            appInfo.internalName = "stapikplannercloudtest";
            stapik::app::AppContext::resetForTests();
            stapik::app::AppContext::initialize(appInfo);

            m_fake.nextServerTime = at(1000);
            m_fake.serverTimeStep = 1s;
        }

        void TearDown() override
        {
            m_model.reset();
            stapik::app::AppContext::resetForTests();
            std::error_code errorCode;
            fs::remove_all(m_directory, errorCode);
        }

        // Without a stored configuration the model starts disconnected.
        void createModel(const std::optional<CloudStorageConfig>& storedConfig = std::nullopt, const AsyncSyncOptions& options = { 20ms, 20ms, 80ms })
        {
            CloudSessionHooks hooks;
            hooks.loadConfig = [storedConfig] { return storedConfig; };
            hooks.saveConfig = [this](const CloudStorageConfig& config)
            {
                m_savedConfigs.push_back(config);
                return true;
            };
            hooks.createStorage = [this](const CloudStorageConfig&) { return std::make_unique<StorageProxy>(m_fake); };

            m_model = std::make_unique<PlannerModel>(std::make_unique<PlannerStorage>(), std::move(hooks), options);
            m_model->signalCloudOutcome().connect([this](const PlannerModel::CloudOutcome& outcome) { m_outcomes.push_back(outcome.state); });
            m_model->signalCloudStatusChanged().connect([this](const SyncStatus status) { m_statuses.push_back(status); });
        }

        [[nodiscard]] bool waitForOutcomes(const std::size_t count)
        {
            return pumpUntil([this, count] { return m_outcomes.size() >= count; });
        }

        FakeCloudStorage m_fake;
        std::unique_ptr<PlannerModel> m_model;
        std::vector<CloudStorageConfig> m_savedConfigs;
        std::vector<SyncState> m_outcomes;
        std::vector<SyncStatus> m_statuses;
        fs::path m_directory;
    };
}

TEST_F(PlannerModelCloudTest, ConnectingSavesTheConfigurationAndUploadsAPlannerThatIsNotInTheCloudYet)
{
    createModel();
    m_model->addActivity(Activity{ .name = "Gym", .difficulty = Difficulty::HARD });

    ASSERT_TRUE(m_model->connectCloud(validConfig()));
    ASSERT_TRUE(waitForOutcomes(1));

    ASSERT_EQ(m_savedConfigs.size(), 1U);
    EXPECT_EQ(m_savedConfigs[0].apiUrl, "https://example.com");
    EXPECT_TRUE(m_model->isCloudConnected());
    EXPECT_EQ(m_outcomes[0], SyncState::Synchronized);
    ASSERT_TRUE(m_fake.stored.has_value());
    EXPECT_EQ(m_fake.stored->content.at("payload").at("activities").size(), 1U);
    EXPECT_EQ(m_fake.stored->content.at("payload").at("activities")[0].at("name"), "Gym");
}

TEST_F(PlannerModelCloudTest, IncompleteConfigurationIsRejected)
{
    createModel();

    EXPECT_FALSE(m_model->connectCloud(CloudStorageConfig{}));
    EXPECT_FALSE(m_model->isCloudConnected());
    EXPECT_TRUE(m_savedConfigs.empty());
}

TEST_F(PlannerModelCloudTest, StatusPassesThroughSyncingBackToIdle)
{
    createModel();

    ASSERT_TRUE(m_model->connectCloud(validConfig()));
    ASSERT_TRUE(waitForOutcomes(1));

    ASSERT_FALSE(m_statuses.empty());
    EXPECT_EQ(m_statuses.back(), SyncStatus::Idle);
    EXPECT_NE(std::ranges::find(m_statuses, SyncStatus::Syncing), m_statuses.end());
    EXPECT_EQ(m_model->cloudStatus(), SyncStatus::Idle);
}

TEST_F(PlannerModelCloudTest, ConnectionFromTheSavedConfigurationIsSynchronizedAtStartAndAdoptsANewerCloudDocument)
{
    m_fake.stored = cloudDocumentOf(makeSampleDocument(200), 900);
    int activitiesChanges = 0;

    createModel(validConfig());
    m_model->signalActivitiesChanged().connect([&activitiesChanges] { ++activitiesChanges; });
    ASSERT_TRUE(m_model->isCloudConnected());
    ASSERT_TRUE(waitForOutcomes(1));

    EXPECT_EQ(m_outcomes[0], SyncState::ServerWon);
    EXPECT_EQ(activitiesChanges, 1);
    ASSERT_EQ(m_model->activities().size(), 2U);
    EXPECT_EQ(m_model->activities()[0].name, "Gym");

    m_model.reset();
    createModel();
    EXPECT_EQ(m_model->activities().size(), 2U) << "the adopted document must be saved locally";
}

TEST_F(PlannerModelCloudTest, EditsAreUploadedInTheBackgroundUsingTheBaselineOfThePreviousUpload)
{
    createModel();
    ASSERT_TRUE(m_model->connectCloud(validConfig()));
    ASSERT_TRUE(waitForOutcomes(1));
    const auto firstServerTime = m_fake.stored->updatedAt;

    m_model->addActivity(Activity{ .name = "Gym", .difficulty = Difficulty::HARD });
    ASSERT_TRUE(waitForOutcomes(2));

    EXPECT_EQ(m_outcomes[1], SyncState::Synchronized);
    EXPECT_EQ(m_fake.stored->content.at("payload").at("activities").size(), 1U);
    // Knowing the baseline, the second upload is accepted at once instead of being rejected and retried.
    EXPECT_EQ(m_fake.saveCalls, 2);
    ASSERT_EQ(m_fake.receivedBaselines.size(), 2U);
    EXPECT_EQ(m_fake.receivedBaselines.back(), firstServerTime);
}

TEST_F(PlannerModelCloudTest, EditsMadeOfflineAreKeptAndUploadedWhenTheCloudIsBack)
{
    createModel();
    m_fake.unreachable = true;
    ASSERT_TRUE(m_model->connectCloud(validConfig()));
    ASSERT_TRUE(waitForOutcomes(1));
    EXPECT_EQ(m_outcomes[0], SyncState::OfflineKeptLocal);

    m_model->addActivity(Activity{ .name = "Gym", .difficulty = Difficulty::HARD });
    m_fake.unreachable = false;
    ASSERT_TRUE(pumpUntil([this] { return !m_outcomes.empty() && m_outcomes.back() == SyncState::Synchronized; }));

    ASSERT_TRUE(m_fake.stored.has_value());
    EXPECT_EQ(m_fake.stored->content.at("payload").at("activities")[0].at("name"), "Gym");
    EXPECT_EQ(m_model->activities().size(), 1U);
}

TEST_F(PlannerModelCloudTest, EditMadeWhileTheFirstSynchronizationRunsIsKeptWhenItIsNewerThanTheCloudCopy)
{
    m_fake.stored = cloudDocumentOf(makeSampleDocument(200), 900);
    createModel(validConfig());

    // The synchronization started at construction has not been delivered yet.
    m_model->addActivity(Activity{ .name = "Local", .difficulty = Difficulty::LIGHT });
    ASSERT_TRUE(pumpUntil([this] { return !m_outcomes.empty() && m_outcomes.back() == SyncState::Synchronized; }));

    EXPECT_EQ(m_outcomes.front(), SyncState::ServerWon);
    ASSERT_EQ(m_model->activities().size(), 1U);
    EXPECT_EQ(m_model->activities()[0].name, "Local");
    ASSERT_TRUE(m_fake.stored.has_value());
    const auto& uploaded = m_fake.stored->content.at("payload").at("activities");
    ASSERT_EQ(uploaded.size(), 1U);
    EXPECT_EQ(uploaded[0].at("name"), "Local");
}

TEST_F(PlannerModelCloudTest, CloudCopyWithoutAKnownTimeIsTakenEvenWhenThePlannerWasEditedMeanwhile)
{
    // An earlier version wrote no "lastUpdate" into the payload, so its age cannot be compared.
    auto payload = makeSampleDocument(0).toJson();
    payload.erase("lastUpdate");
    m_fake.stored = CloudDocument{
        .content = stapik::sync::SyncEnvelope{ .lastUpdate = at(200), .payload = payload }.toJson(),
        .updatedAt = at(900)
    };
    createModel(validConfig());

    m_model->addActivity(Activity{ .name = "Local", .difficulty = Difficulty::LIGHT });
    ASSERT_TRUE(waitForOutcomes(1));

    EXPECT_EQ(m_outcomes.front(), SyncState::ServerWon);
    ASSERT_EQ(m_model->activities().size(), 2U);
    EXPECT_EQ(m_model->activities()[0].name, "Gym");
}

TEST_F(PlannerModelCloudTest, FlushUploadsThePendingChangeWithoutWaitingForTheDelay)
{
    createModel(std::nullopt, AsyncSyncOptions{ 60s, 20ms, 80ms });
    ASSERT_TRUE(m_model->connectCloud(validConfig()));
    ASSERT_TRUE(waitForOutcomes(1));
    m_model->addActivity(Activity{ .name = "Gym", .difficulty = Difficulty::HARD });

    m_model->flushCloud(3000ms);

    EXPECT_EQ(m_fake.stored->content.at("payload").at("activities").size(), 1U);
}

TEST_F(PlannerModelCloudTest, FlushDoesNotKeepTheApplicationWaitingWhenTheCloudIsUnreachable)
{
    createModel(std::nullopt, AsyncSyncOptions{ 60s, 20ms, 80ms });
    ASSERT_TRUE(m_model->connectCloud(validConfig()));
    ASSERT_TRUE(waitForOutcomes(1));
    m_fake.unreachable = true;
    m_model->addActivity(Activity{ .name = "Gym", .difficulty = Difficulty::HARD });

    const auto started = std::chrono::steady_clock::now();
    m_model->flushCloud(4000ms);
    const auto elapsed = std::chrono::steady_clock::now() - started;

    EXPECT_LT(elapsed, 2000ms);
}

TEST_F(PlannerModelCloudTest, FlushWithoutAConnectionReturnsImmediately)
{
    createModel();
    m_model->addActivity(Activity{ .name = "Gym", .difficulty = Difficulty::HARD });

    const auto started = std::chrono::steady_clock::now();
    m_model->flushCloud(4000ms);

    EXPECT_LT(std::chrono::steady_clock::now() - started, 500ms);
    EXPECT_TRUE(m_outcomes.empty());
}

TEST_F(PlannerModelCloudTest, ADocumentTakenFromTheCloudClearsTheHistory)
{
    createModel();
    m_model->addActivity(Activity{ .name = "Local", .difficulty = Difficulty::LIGHT });
    ASSERT_TRUE(m_model->undoStack().canUndo());

    auto newerThanTheLocalOne = makeSampleDocument(0);
    newerThanTheLocalOne.updatedAt = std::chrono::system_clock::now() + 1h;
    m_fake.stored = cloudDocumentOf(newerThanTheLocalOne, 900);
    ASSERT_TRUE(m_model->connectCloud(validConfig()));
    ASSERT_TRUE(waitForOutcomes(1));

    EXPECT_EQ(m_outcomes.front(), SyncState::ServerWon);
    EXPECT_EQ(m_model->activities().size(), 2U);
    EXPECT_FALSE(m_model->undoStack().canUndo()) << "the steps describe a document that has been replaced";
    EXPECT_FALSE(m_model->undoStack().canRedo());
}

TEST_F(PlannerModelCloudTest, AnEditThatStaysAfterTheCloudCopyWonCanStillBeUndone)
{
    m_fake.stored = cloudDocumentOf(makeSampleDocument(200), 900);
    createModel(validConfig());

    m_model->addActivity(Activity{ .name = "Local", .difficulty = Difficulty::LIGHT });
    ASSERT_TRUE(pumpUntil([this] { return !m_outcomes.empty() && m_outcomes.back() == SyncState::Synchronized; }));

    ASSERT_EQ(m_model->activities().size(), 1U);
    EXPECT_TRUE(m_model->undoStack().canUndo());
}

TEST_F(PlannerModelCloudTest, UndoingAChangeIsUploadedLikeAnyOtherChange)
{
    createModel();
    ASSERT_TRUE(m_model->connectCloud(validConfig()));
    ASSERT_TRUE(waitForOutcomes(1));
    m_model->addActivity(Activity{ .name = "Gym", .difficulty = Difficulty::HARD });
    ASSERT_TRUE(waitForOutcomes(2));
    ASSERT_EQ(m_fake.stored->content.at("payload").at("activities").size(), 1U);

    EXPECT_TRUE(m_model->undoStack().undo());
    ASSERT_TRUE(waitForOutcomes(3));

    EXPECT_EQ(m_outcomes[2], SyncState::Synchronized);
    EXPECT_EQ(m_fake.stored->content.at("payload").at("activities").size(), 0U);
}
