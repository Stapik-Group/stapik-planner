#include "core/model/PlannerDocument.hpp"

#include "support/FakeCloudStorage.hpp"
#include "support/PlannerTestDocuments.hpp"

#include "stapik/sync/SyncCoordinator.hpp"
#include "stapik/sync/SyncEnvelope.hpp"

#include <gtest/gtest.h>

#include <optional>

namespace
{
    using planner::test::at;
    using planner::test::cloudDocumentOf;
    using planner::test::makeSampleDocument;
    using stapik::sync::SyncCoordinator;
    using stapik::sync::SyncEnvelope;
    using stapik::sync::SyncState;
    using stapik::test::FakeCloudStorage;

    class PlannerSyncCompatibilityTest : public testing::Test
    {
    protected:
        PlannerSyncCompatibilityTest()
        {
            m_fake.nextServerTime = at(1000);
        }

        FakeCloudStorage m_fake;
        SyncCoordinator<PlannerDocument> m_coordinator{ m_fake };
    };
}

TEST_F(PlannerSyncCompatibilityTest, NewerCloudDocumentReplacesTheLocalOneAndKeepsItsLastUpdate)
{
    const auto remote = makeSampleDocument(200);
    m_fake.stored = cloudDocumentOf(remote, 900);

    const auto outcome = m_coordinator.resolveOnConnect(PlannerDocument::createDefault());

    EXPECT_EQ(outcome.state, SyncState::ServerWon);
    EXPECT_EQ(outcome.document.toJson(), remote.toJson());
    EXPECT_EQ(outcome.document.lastUpdate(), at(200));
    EXPECT_EQ(outcome.document.lastKnownCloudUpdate(), std::optional<PlannerDocument::TimePoint>{ at(900) });
}

TEST_F(PlannerSyncCompatibilityTest, AdoptedCloudDocumentIsRecognizedAsSynchronizedOnTheNextConnect)
{
    m_fake.stored = cloudDocumentOf(makeSampleDocument(200), 900);
    const auto adopted = m_coordinator.resolveOnConnect(PlannerDocument::createDefault()).document;

    const auto outcome = m_coordinator.resolveOnConnect(adopted);

    EXPECT_EQ(outcome.state, SyncState::Synchronized);
    EXPECT_EQ(m_fake.saveCalls, 0);
}

TEST_F(PlannerSyncCompatibilityTest, NewerLocalDocumentIsUploadedInTheEnvelopeFormatOfEarlierVersions)
{
    m_fake.stored = cloudDocumentOf(makeSampleDocument(100), 900);
    const auto local = makeSampleDocument(300);

    const auto outcome = m_coordinator.resolveOnConnect(local);

    EXPECT_EQ(outcome.state, SyncState::Synchronized);
    ASSERT_TRUE(m_fake.stored.has_value());
    const auto& content = m_fake.stored->content;
    EXPECT_EQ(content.at("lastUpdate"), "2026-01-01T12:05:00Z");
    EXPECT_EQ(content.at("payload"), local.toJson());
    EXPECT_TRUE(content.at("payload").contains("settings"));
    EXPECT_TRUE(content.at("payload").contains("activities"));
    EXPECT_TRUE(content.at("payload").contains("weekPlan"));
    EXPECT_EQ(outcome.document.lastKnownCloudUpdate(), std::optional<PlannerDocument::TimePoint>{ at(1000) });
}

TEST_F(PlannerSyncCompatibilityTest, CloudDocumentUploadedByAnEarlierVersionIsReadable)
{
    // An earlier version put "lastUpdate" only in the envelope, and wrote no ids.
    auto payload = makeSampleDocument(0).toJson();
    payload.erase("lastUpdate");
    for (auto& activityJson : payload.at("activities"))
        activityJson.erase("id");
    for (auto& dayJson : payload.at("weekPlan"))
    {
        for (auto& slotJson : dayJson.at("slots"))
        {
            if (!slotJson.is_null())
                slotJson.erase("id");
        }
    }
    m_fake.stored = CloudDocument{
        .content = SyncEnvelope{ .lastUpdate = at(200), .payload = payload }.toJson(),
        .updatedAt = at(900)
    };

    const auto outcome = m_coordinator.resolveOnConnect(PlannerDocument::createDefault());

    EXPECT_EQ(outcome.state, SyncState::ServerWon);
    EXPECT_EQ(outcome.document.activities.size(), 2U);
    ASSERT_TRUE(outcome.document.weekPlan[0].slots[0].has_value());
    EXPECT_EQ(outcome.document.weekPlan[0].slots[0]->name, "Gym");
    EXPECT_EQ(outcome.document.weekPlan[0].slots[0]->id, outcome.document.activities[0].id);
    EXPECT_FALSE(outcome.document.activities[0].id.empty());
}

TEST_F(PlannerSyncCompatibilityTest, UnreadableCloudDocumentKeepsTheLocalOne)
{
    m_fake.stored = CloudDocument{
        .content = SyncEnvelope{ .lastUpdate = at(500), .payload = { { "settings", { { "slots", "many" } } } } }.toJson(),
        .updatedAt = at(900)
    };
    const auto local = makeSampleDocument(100);

    const auto outcome = m_coordinator.resolveOnConnect(local);

    EXPECT_EQ(outcome.state, SyncState::RemoteUnreadable);
    EXPECT_EQ(outcome.document.toJson(), local.toJson());
    EXPECT_EQ(m_fake.saveCalls, 0);
}
