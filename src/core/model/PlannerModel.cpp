#include "PlannerModel.hpp"
#include "../../infrastructure/storage/PlannerStorage.hpp"

#include "stapik/command/ICommand.hpp"
#include "stapik/domain/IdGenerator.hpp"
#include "stapik/locale/LocaleManager.hpp"
#include "stapik/log/Log.hpp"
#include "stapik/storage/PathText.hpp"

#include <glibmm/main.h>

#include <chrono>
#include <utility>

namespace
{
    void refreshPlannedCopies(WeekPlan& weekPlan, const Activity& activity)
    {
        for (auto& day : weekPlan)
        {
            for (auto& slot : day.slots)
            {
                if (slot.has_value() && slot->id == activity.id)
                    slot = activity;
            }
        }
    }

    void clearPlannedCopies(WeekPlan& weekPlan, const std::string& activityId)
    {
        for (auto& day : weekPlan)
        {
            for (auto& slot : day.slots)
            {
                if (slot.has_value() && slot->id == activityId)
                    slot.reset();
            }
        }
    }
}

class PlannerModel::ContentCommand final : public stapik::command::ICommand
{
public:
    ContentCommand(PlannerModel& model, const char* descriptionKey, Content before, Content after, const bool announceWeekPlan) :
        m_model(model),
        m_descriptionKey(descriptionKey),
        m_before(std::move(before)),
        m_after(std::move(after)),
        m_announceWeekPlanOnFirstExecute(announceWeekPlan)
    {}

    void execute() override
    {
        m_model.restoreContent(m_after, m_executedBefore || m_announceWeekPlanOnFirstExecute);
        m_executedBefore = true;
    }

    void undo() override
    {
        m_model.restoreContent(m_before, true);
    }

    [[nodiscard]] std::string description() const override
    {
        return LocaleManager::instance().translate(m_descriptionKey);
    }

private:
    PlannerModel& m_model;
    const char* m_descriptionKey;
    Content m_before;
    Content m_after;
    bool m_announceWeekPlanOnFirstExecute;
    bool m_executedBefore = false;
};

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
    m_cloudSession.signalApiKeyExpiring().connect([this](const TimePoint expiresAt) { m_signalCloudApiKeyExpiring.emit(expiresAt); });

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
    auto after = content();
    activity.id = stapik::domain::generateId();
    after.activities.push_back(std::move(activity));

    execute("command.activity.add", std::move(after));
}

void PlannerModel::updateActivity(const std::size_t index, Activity activity)
{
    if (index >= m_document.activities.size())
        return;

    auto after = content();
    activity.id = after.activities[index].id;
    after.activities[index] = std::move(activity);
    refreshPlannedCopies(after.weekPlan, after.activities[index]);

    execute("command.activity.edit", std::move(after));
}

void PlannerModel::removeActivity(const std::size_t index)
{
    if (index >= m_document.activities.size())
        return;

    auto after = content();
    const auto removedId = after.activities[index].id;
    after.activities.erase(after.activities.begin() + static_cast<Activities::difference_type>(index));
    clearPlannedCopies(after.weekPlan, removedId);

    execute("command.activity.remove", std::move(after));
}

bool PlannerModel::updateSlotCount(const int newSlotCount)
{
    auto after = content();
    bool dataLost = false;
    for (auto& day : after.weekPlan)
    {
        for (auto slotIndex = static_cast<std::size_t>(newSlotCount); slotIndex < day.slots.size(); ++slotIndex)
        {
            if (day.slots[slotIndex].has_value())
                dataLost = true;
        }

        day.slots.resize(static_cast<std::size_t>(newSlotCount));
    }

    after.settings.slots = newSlotCount;
    execute("command.slots.count", std::move(after));

    return dataLost;
}

void PlannerModel::updateDay(const Weekday weekday, DayPlan dayPlan)
{
    const auto index = static_cast<std::size_t>(weekday);
    if (index >= m_document.weekPlan.size())
        return;

    auto after = content();
    after.weekPlan[index] = std::move(dayPlan);

    execute("command.slot.change", std::move(after), false);
}

stapik::command::UndoStack& PlannerModel::undoStack()
{
    return m_undoStack;
}

std::vector<stapik::document::BackupInfo> PlannerModel::listBackups() const
{
    return m_storage->listBackups();
}

bool PlannerModel::restoreBackup(const stapik::document::BackupInfo& backup)
{
    auto loaded = m_storage->loadBackup(backup);
    if (loaded.status != stapik::document::LoadStatus::Loaded || !loaded.document.has_value())
    {
        stapik::log::warning("Cannot restore the planner backup {}", stapik::storage::pathText(backup.path));
        return false;
    }

    const auto& restored = *loaded.document;
    execute("command.backup.restore", Content{ .settings = restored.settings, .activities = restored.activities, .weekPlan = restored.weekPlan });
    return true;
}

bool PlannerModel::connectCloud(const CloudStorageConfig& config)
{
    return m_cloudSession.connectWith(config, m_document);
}

void PlannerModel::syncCloud()
{
    m_cloudSession.syncNow(m_document);
}

void PlannerModel::disconnectCloud()
{
    m_deferredPush.disconnect();
    m_cloudSession.disconnect();
    m_document.cloudBaseline.reset();
}

void PlannerModel::flushCloud(const std::chrono::milliseconds timeout)
{
    m_cloudSession.flushAndWait(timeout);
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

sigc::signal<void(PlannerModel::TimePoint)>& PlannerModel::signalCloudApiKeyExpiring()
{
    return m_signalCloudApiKeyExpiring;
}

sigc::signal<void(const std::string&)>& PlannerModel::signalSaveFailed()
{
    return m_signalSaveFailed;
}

PlannerModel::Content PlannerModel::content() const
{
    return Content{ .settings = m_document.settings, .activities = m_document.activities, .weekPlan = m_document.weekPlan };
}

void PlannerModel::execute(const char* descriptionKey, Content after, const bool announceWeekPlan)
{
    auto before = content();
    if (before == after)
        return;

    m_undoStack.execute(std::make_unique<ContentCommand>(*this, descriptionKey, std::move(before), std::move(after), announceWeekPlan));
}

void PlannerModel::restoreContent(const Content& content, const bool announceWeekPlan)
{
    const bool activitiesChanged = m_document.activities != content.activities;
    const bool settingsChanged = m_document.settings != content.settings;
    const bool weekPlanChanged = m_document.weekPlan != content.weekPlan;

    m_document.settings = content.settings;
    m_document.activities = content.activities;
    m_document.weekPlan = content.weekPlan;
    persist();

    if (activitiesChanged)
        m_signalActivitiesChanged.emit();

    if (settingsChanged)
        m_signalSettingsChanged.emit();

    if (weekPlanChanged && announceWeekPlan)
        m_signalWeekPlanChanged.emit();
}

void PlannerModel::persist()
{
    m_document.updatedAt = std::chrono::system_clock::now();
    saveToDisk();
    m_cloudSession.pushChange(m_document);
}

void PlannerModel::saveToDisk()
{
    const auto saved = m_storage->save(m_document);
    if (saved)
    {
        m_saveFailing = false;
        return;
    }

    const auto message = saved.error().message();
    stapik::log::warning("Cannot save the planner to {}: {}", stapik::storage::pathText(m_storage->filePath()), message);

    if (!std::exchange(m_saveFailing, true))
        m_signalSaveFailed.emit(message);
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

    const auto cloudBaseline = cloudDocument.lastKnownCloudUpdate();
    const bool cloudTimeKnown = cloudDocument.lastUpdate() != PlannerDocument::TimePoint{};
    const bool localIsNewer = m_document.lastUpdate() > cloudDocument.lastUpdate();
    if (outcome.state == stapik::sync::SyncState::ServerWon && cloudBaseline.has_value() && cloudTimeKnown && localIsNewer)
    {
        m_document.cloudBaseline = cloudBaseline;
        pushLocalEditAfterTheOutcomeIsHandled();
        return;
    }

    m_undoStack.clear();
    m_document = cloudDocument;
    saveToDisk();
    m_signalActivitiesChanged.emit();
    m_signalSettingsChanged.emit();
    m_signalWeekPlanChanged.emit();
}

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
