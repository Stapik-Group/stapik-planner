#include "PlannerModel.hpp"
#include "../../infrastructure/storage/PlannerStorage.hpp"

#include "stapik/domain/IdGenerator.hpp"
#include "stapik/log/Log.hpp"

#include <glib.h>
#include <glibmm/main.h>

#include <chrono>
#include <thread>
#include <utility>

namespace
{
    // Brings the copies of an activity kept in the plan in line with its catalog entry.
    bool refreshPlannedCopies(WeekPlan& weekPlan, const Activity& activity)
    {
        bool changed = false;
        for (auto& day : weekPlan)
        {
            for (auto& slot : day.slots)
            {
                if (!slot.has_value() || slot->id != activity.id)
                    continue;

                if (slot->name == activity.name && slot->difficulty == activity.difficulty)
                    continue;

                slot = activity;
                changed = true;
            }
        }

        return changed;
    }

    bool clearPlannedCopies(WeekPlan& weekPlan, const std::string& activityId)
    {
        bool changed = false;
        for (auto& day : weekPlan)
        {
            for (auto& slot : day.slots)
            {
                if (slot.has_value() && slot->id == activityId)
                {
                    slot.reset();
                    changed = true;
                }
            }
        }

        return changed;
    }
}

PlannerModel::PlannerModel() :
    PlannerModel(std::make_unique<PlannerStorage>(), stapik::sync::defaultCloudSessionHooks(CLOUD_SLOT_KEY))
{}

PlannerModel::PlannerModel(std::unique_ptr<PlannerStorage> storage, stapik::sync::CloudSessionHooks cloudHooks, const stapik::sync::AsyncSyncOptions& syncOptions) :
    m_storage(std::move(storage)),
    m_document(m_storage->load()),
    m_cloudSession(std::move(cloudHooks), syncOptions)
{
    m_cloudSession.signalStatusChanged().connect([this](const stapik::sync::SyncStatus status) { m_signalCloudStatusChanged.emit(status); });
    m_cloudSession.signalOutcome().connect([this](const CloudOutcome& outcome) { onCloudOutcome(outcome); });

    // A connection restored from the saved configuration is synchronized at start-up.
    if (m_cloudSession.isConnected())
        m_cloudSession.syncNow(m_document);
}

PlannerModel::~PlannerModel()
{
    m_deferredPush.disconnect();
}

const Settings& PlannerModel::settings() const
{
    return m_document.settings;
}

const Activities& PlannerModel::activities() const
{
    return m_document.activities;
}

const WeekPlan& PlannerModel::weekPlan() const
{
    return m_document.weekPlan;
}

void PlannerModel::addActivity(Activity activity)
{
    activity.id = stapik::domain::generateId();
    m_document.activities.push_back(std::move(activity));
    persist();
    m_signalActivitiesChanged.emit();
}

void PlannerModel::updateActivity(const std::size_t index, Activity activity)
{
    if (index >= m_document.activities.size())
        return;

    activity.id = m_document.activities[index].id;
    m_document.activities[index] = std::move(activity);
    const auto planChanged = refreshPlannedCopies(m_document.weekPlan, m_document.activities[index]);

    persist();
    m_signalActivitiesChanged.emit();
    if (planChanged)
        m_signalWeekPlanChanged.emit();
}

void PlannerModel::removeActivity(const std::size_t index)
{
    if (index >= m_document.activities.size())
        return;

    const auto removedId = m_document.activities[index].id;
    m_document.activities.erase(m_document.activities.begin() + static_cast<Activities::difference_type>(index));
    const auto planChanged = clearPlannedCopies(m_document.weekPlan, removedId);

    persist();
    m_signalActivitiesChanged.emit();
    if (planChanged)
        m_signalWeekPlanChanged.emit();
}

void PlannerModel::updateSettings(Settings settings)
{
    m_document.settings = settings;
    persist();
    m_signalSettingsChanged.emit();
}

bool PlannerModel::updateSlotCount(const int newSlotCount)
{
    bool dataLost = false;
    for (auto& day : m_document.weekPlan)
    {
        for (auto i = static_cast<std::size_t>(newSlotCount); i < day.slots.size(); ++i)
        {
            if (day.slots[i].has_value())
                dataLost = true;
        }
        day.slots.resize(static_cast<std::size_t>(newSlotCount));
    }

    m_document.settings.slots = newSlotCount;

    persist();
    m_signalSettingsChanged.emit();
    m_signalWeekPlanChanged.emit();

    return dataLost;
}

void PlannerModel::updateWeekPlan(const WeekPlan& weekPlan)
{
    m_document.weekPlan = weekPlan;
    persist();
    m_signalWeekPlanChanged.emit();
}

void PlannerModel::updateDay(const Weekday weekday, DayPlan dayPlan)
{
    const auto index = static_cast<std::size_t>(weekday);
    if (index >= m_document.weekPlan.size())
        return;

    m_document.weekPlan[index] = std::move(dayPlan);
    persist();
    m_signalWeekPlanChanged.emit();
}

bool PlannerModel::connectCloud(const CloudStorageConfig& config)
{
    return m_cloudSession.connectWith(config, m_document);
}

void PlannerModel::syncCloud()
{
    m_cloudSession.syncNow(m_document);
}

void PlannerModel::flushCloud(const std::chrono::milliseconds timeout)
{
    if (!m_cloudSession.hasPendingWork())
        return;

    // Waiting is over when the synchronization is done or cannot go on (offline, conflict, unreadable cloud copy).
    bool settled = false;
    auto outcomeConnection = m_cloudSession.signalOutcome().connect([&settled](const CloudOutcome& outcome)
    {
        settled = outcome.state != stapik::sync::SyncState::Synchronized;
    });

    m_cloudSession.flush();

    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (!settled && m_cloudSession.hasPendingWork() && std::chrono::steady_clock::now() < deadline)
    {
        while (g_main_context_iteration(nullptr, FALSE))
        {
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    outcomeConnection.disconnect();
}

bool PlannerModel::isCloudConnected() const
{
    return m_cloudSession.isConnected();
}

const std::optional<CloudStorageConfig>& PlannerModel::cloudConfig() const
{
    return m_cloudSession.config();
}

stapik::sync::SyncStatus PlannerModel::cloudStatus() const
{
    return m_cloudSession.status();
}

sigc::signal<void()>& PlannerModel::signalActivitiesChanged()
{
    return m_signalActivitiesChanged;
}

sigc::signal<void()>& PlannerModel::signalSettingsChanged()
{
    return m_signalSettingsChanged;
}

sigc::signal<void()>& PlannerModel::signalWeekPlanChanged()
{
    return m_signalWeekPlanChanged;
}

sigc::signal<void(stapik::sync::SyncStatus)>& PlannerModel::signalCloudStatusChanged()
{
    return m_signalCloudStatusChanged;
}

sigc::signal<void(const PlannerModel::CloudOutcome&)>& PlannerModel::signalCloudOutcome()
{
    return m_signalCloudOutcome;
}

void PlannerModel::persist()
{
    m_document.updatedAt = std::chrono::system_clock::now();
    saveToDisk();
    m_cloudSession.pushChange(m_document);
}

void PlannerModel::saveToDisk()
{
    if (!m_storage->save(m_document))
        stapik::log::warning("Cannot save the planner to {}", m_storage->filePath().string());
}

void PlannerModel::onCloudOutcome(const CloudOutcome& outcome)
{
    if (outcome.replacesLocal())
        replaceWithCloudDocument(outcome);
    else if (const auto baseline = outcome.document.lastKnownCloudUpdate(); baseline.has_value() && outcome.state == stapik::sync::SyncState::Synchronized)
        m_document.cloudBaseline = baseline;

    m_signalCloudOutcome.emit(outcome);
}

void PlannerModel::replaceWithCloudDocument(const CloudOutcome& outcome)
{
    const auto& cloudDocument = outcome.document;

    // The synchronization runs in the background, so the planner may have been edited since it started.
    // The scheduler drops that queued change when the cloud copy wins, which would lose the edit even
    // when it is newer than the cloud copy, so the newer local edit is pushed again. A cloud copy
    // without a known time (written by an earlier version) is always taken, and so is a copy that
    // the scheduler accepted after losing the race twice, to avoid retrying forever.
    const auto cloudBaseline = cloudDocument.lastKnownCloudUpdate();
    const bool cloudTimeKnown = cloudDocument.lastUpdate() != PlannerDocument::TimePoint{};
    const bool localIsNewer = m_document.lastUpdate() > cloudDocument.lastUpdate();
    if (outcome.state == stapik::sync::SyncState::ServerWon && cloudBaseline.has_value() && cloudTimeKnown && localIsNewer)
    {
        m_document.cloudBaseline = cloudBaseline;
        pushLocalEditAfterTheOutcomeIsHandled();
        return;
    }

    m_document = cloudDocument;
    saveToDisk();
    m_signalActivitiesChanged.emit();
    m_signalSettingsChanged.emit();
    m_signalWeekPlanChanged.emit();
}

// The scheduler discards its queued change after it has announced the outcome, so a push requested
// from the outcome handler would be discarded with it. It has to wait until the handling is over.
void PlannerModel::pushLocalEditAfterTheOutcomeIsHandled()
{
    if (m_deferredPush.connected())
        return;

    m_deferredPush = Glib::signal_idle().connect([this]
    {
        m_cloudSession.pushChange(m_document);
        return false;
    });
}
