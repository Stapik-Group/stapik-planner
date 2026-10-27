#include "PlannerModel.hpp"
#include "ActivityName.hpp"
#include "../../infrastructure/storage/PlannerStorage.hpp"

#include "stapik/command/ICommand.hpp"
#include "stapik/domain/IdGenerator.hpp"
#include "stapik/locale/LocaleManager.hpp"
#include "stapik/log/Log.hpp"
#include "stapik/storage/PathText.hpp"

#include <glibmm/main.h>

#include <algorithm>
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

    bool raisesLoadAboveLimit(const DayPlan& before, const DayPlan& after)
    {
        return after.totalLoad() > DayPlan::LOAD_LIMIT && after.totalLoad() > before.totalLoad();
    }

    bool raisesAnyDayAboveLimit(const WeekPlan& before, const WeekPlan& after)
    {
        for (std::size_t index = 0; index < before.size() && index < after.size(); ++index)
        {
            if (raisesLoadAboveLimit(before[index], after[index]))
                return true;
        }

        return false;
    }

    std::size_t countPlannedSlots(const WeekPlan& weekPlan, const std::string& activityId)
    {
        std::size_t count = 0;
        for (const auto& day : weekPlan)
        {
            count += static_cast<std::size_t>(std::ranges::count_if(day.slots, [&activityId](const std::optional<Activity>& slot)
            {
                return slot.has_value() && slot->id == activityId;
            }));
        }

        return count;
    }

    std::vector<std::pair<std::string, std::string>> planIdentities(const std::string& activeId, const std::string& activeName, const std::vector<SavedPlan>& savedPlans)
    {
        std::vector<std::pair<std::string, std::string>> identities{ { activeId, activeName } };
        for (const auto& plan : savedPlans)
            identities.emplace_back(plan.id, plan.name);

        return identities;
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

PlannerModel::PlannerModel(std::unique_ptr<PlannerStorage> storage, stapik::sync::CloudSessionHooks cloudHooks, const stapik::sync::AsyncSyncOptions& syncOptions, const stapik::time::IClock& clock) :
    m_storage(std::move(storage)),
    m_document(m_storage->load()),
    m_cloudSession(std::move(cloudHooks), syncOptions),
    m_clock(clock)
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

std::optional<std::size_t> PlannerModel::findActivity(const std::string& id) const
{
    const auto found = std::ranges::find(m_document.activities, id, &Activity::id);
    if (found == m_document.activities.end())
        return std::nullopt;

    return static_cast<std::size_t>(found - m_document.activities.begin());
}

std::size_t PlannerModel::plannedSlotCount(const std::string& id) const
{
    auto count = countPlannedSlots(m_document.weekPlan, id);
    for (const auto& plan : m_document.savedPlans)
        count += countPlannedSlots(plan.weekPlan, id);

    return count;
}

void PlannerModel::addActivity(Activity activity)
{
    auto after = content();
    activity.id = stapik::domain::generateId();
    after.activities.push_back(std::move(activity));

    execute("command.activity.add", std::move(after));
}

bool PlannerModel::updateActivity(const std::size_t index, Activity activity)
{
    if (index >= m_document.activities.size())
        return false;

    auto after = content();
    activity.id = after.activities[index].id;
    after.activities[index] = std::move(activity);
    refreshPlannedCopies(after.weekPlan, after.activities[index]);
    for (auto& plan : after.savedPlans)
        refreshPlannedCopies(plan.weekPlan, after.activities[index]);

    if (raisesAnyDayAboveLimit(m_document.weekPlan, after.weekPlan))
        return false;

    for (std::size_t planIndex = 0; planIndex < after.savedPlans.size(); ++planIndex)
    {
        if (raisesAnyDayAboveLimit(m_document.savedPlans[planIndex].weekPlan, after.savedPlans[planIndex].weekPlan))
            return false;
    }

    execute("command.activity.edit", std::move(after));
    return true;
}

void PlannerModel::removeActivity(const std::size_t index)
{
    if (index >= m_document.activities.size())
        return;

    auto after = content();
    const auto removedId = after.activities[index].id;
    after.activities.erase(after.activities.begin() + static_cast<Activities::difference_type>(index));
    clearPlannedCopies(after.weekPlan, removedId);
    for (auto& plan : after.savedPlans)
        clearPlannedCopies(plan.weekPlan, removedId);

    execute("command.activity.remove", std::move(after));
}

bool PlannerModel::updateSlotCount(const int newSlotCount)
{
    auto after = content();
    bool dataLost = false;
    const auto resizePlan = [&dataLost, newSlotCount](WeekPlan& weekPlan)
    {
        for (auto& day : weekPlan)
        {
            for (auto slotIndex = static_cast<std::size_t>(newSlotCount); slotIndex < day.slots.size(); ++slotIndex)
            {
                if (day.slots[slotIndex].has_value())
                    dataLost = true;
            }

            day.slots.resize(static_cast<std::size_t>(newSlotCount));
        }
    };

    resizePlan(after.weekPlan);
    for (auto& plan : after.savedPlans)
        resizePlan(plan.weekPlan);

    after.settings.slots = newSlotCount;
    execute("command.slots.count", std::move(after));

    return dataLost;
}

bool PlannerModel::updateDay(const Weekday weekday, DayPlan dayPlan)
{
    return replaceDay(weekday, std::move(dayPlan), "command.slot.change", false);
}

bool PlannerModel::copyDay(const Weekday from, const Weekday to)
{
    const auto fromIndex = static_cast<std::size_t>(from);
    if (fromIndex >= m_document.weekPlan.size())
        return false;

    DayPlan copy = m_document.weekPlan[fromIndex];
    copy.weekday = to;

    return replaceDay(to, std::move(copy), "command.day.copy", true);
}

bool PlannerModel::clearDay(const Weekday weekday)
{
    return replaceDay(weekday, DayPlan{ weekday, m_document.settings.slots }, "command.day.clear", true);
}

bool PlannerModel::replaceDay(const Weekday weekday, DayPlan dayPlan, const char* descriptionKey, const bool announceWeekPlan)
{
    const auto index = static_cast<std::size_t>(weekday);
    if (index >= m_document.weekPlan.size() || raisesLoadAboveLimit(m_document.weekPlan[index], dayPlan))
        return false;

    auto after = content();
    after.weekPlan[index] = std::move(dayPlan);

    execute(descriptionKey, std::move(after), announceWeekPlan);
    return true;
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
    execute("command.backup.restore", Content{
        .settings = restored.settings,
        .activities = restored.activities,
        .weekPlan = restored.weekPlan,
        .activePlanId = restored.activePlanId,
        .activePlanName = restored.activePlanName,
        .savedPlans = restored.savedPlans
    });
    return true;
}

std::vector<PlannerModel::PlanSummary> PlannerModel::plans() const
{
    std::vector<PlanSummary> summaries{ { m_document.activePlanId, m_document.activePlanName, true } };
    for (const auto& plan : m_document.savedPlans)
        summaries.push_back({ plan.id, plan.name, false });

    return summaries;
}

const std::string& PlannerModel::activePlanId() const
{
    return m_document.activePlanId;
}

const std::string& PlannerModel::activePlanName() const
{
    return m_document.activePlanName;
}

bool PlannerModel::switchPlan(const std::string& id)
{
    if (id == m_document.activePlanId)
        return true;

    auto after = content();
    const auto target = std::ranges::find(after.savedPlans, id, &SavedPlan::id);
    if (target == after.savedPlans.end())
        return false;

    SavedPlan previous{ after.activePlanId, after.activePlanName, std::move(after.weekPlan) };
    after.activePlanId = target->id;
    after.activePlanName = target->name;
    after.weekPlan = std::move(target->weekPlan);
    *target = std::move(previous);

    execute("command.plan.switch", std::move(after));
    return true;
}

bool PlannerModel::createPlan(const std::string& name)
{
    return addPlan(name, PlannerDocument::emptyWeekPlan(m_document.settings.slots), "command.plan.create");
}

bool PlannerModel::duplicatePlan(const std::string& name)
{
    return addPlan(name, m_document.weekPlan, "command.plan.duplicate");
}

bool PlannerModel::renamePlan(const std::string& id, const std::string& name)
{
    if (!isPlanNameUsable(name, id))
        return false;

    auto after = content();
    const auto trimmedName = ActivityName::trimmed(name);

    if (id == after.activePlanId)
    {
        after.activePlanName = trimmedName;
    }
    else
    {
        const auto plan = std::ranges::find(after.savedPlans, id, &SavedPlan::id);
        if (plan == after.savedPlans.end())
            return false;

        plan->name = trimmedName;
    }

    execute("command.plan.rename", std::move(after));
    return true;
}

bool PlannerModel::deletePlan(const std::string& id)
{
    if (m_document.savedPlans.empty())
        return false;

    auto after = content();

    if (id == after.activePlanId)
    {
        SavedPlan next = std::move(after.savedPlans.front());
        after.savedPlans.erase(after.savedPlans.begin());
        after.activePlanId = std::move(next.id);
        after.activePlanName = std::move(next.name);
        after.weekPlan = std::move(next.weekPlan);
    }
    else
    {
        const auto plan = std::ranges::find(after.savedPlans, id, &SavedPlan::id);
        if (plan == after.savedPlans.end())
            return false;

        after.savedPlans.erase(plan);
    }

    execute("command.plan.delete", std::move(after));
    return true;
}

bool PlannerModel::isPlanNameUsable(const std::string& name, const std::string& exceptId) const
{
    if (ActivityName::trimmed(name).empty())
        return false;

    std::vector<std::string> otherNames;
    if (m_document.activePlanId != exceptId)
        otherNames.push_back(m_document.activePlanName);

    for (const auto& plan : m_document.savedPlans)
    {
        if (plan.id != exceptId)
            otherNames.push_back(plan.name);
    }

    return !ActivityName::isTaken(name, otherNames);
}

bool PlannerModel::addPlan(const std::string& name, WeekPlan weekPlan, const char* descriptionKey)
{
    if (!isPlanNameUsable(name, {}))
        return false;

    auto after = content();
    after.savedPlans.push_back(SavedPlan{ after.activePlanId, after.activePlanName, std::move(after.weekPlan) });
    after.activePlanId = stapik::domain::generateId();
    after.activePlanName = ActivityName::trimmed(name);
    after.weekPlan = std::move(weekPlan);

    execute(descriptionKey, std::move(after));
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

sigc::signal<void()>& PlannerModel::signalPlansChanged()
{
    return m_signalPlansChanged;
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
    return Content{
        .settings = m_document.settings,
        .activities = m_document.activities,
        .weekPlan = m_document.weekPlan,
        .activePlanId = m_document.activePlanId,
        .activePlanName = m_document.activePlanName,
        .savedPlans = m_document.savedPlans
    };
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
    const bool plansChanged = planIdentities(m_document.activePlanId, m_document.activePlanName, m_document.savedPlans)
        != planIdentities(content.activePlanId, content.activePlanName, content.savedPlans);

    m_document.settings = content.settings;
    m_document.activities = content.activities;
    m_document.weekPlan = content.weekPlan;
    m_document.activePlanId = content.activePlanId;
    m_document.activePlanName = content.activePlanName;
    m_document.savedPlans = content.savedPlans;
    persist();

    if (activitiesChanged)
        m_signalActivitiesChanged.emit();

    if (settingsChanged)
        m_signalSettingsChanged.emit();

    if (weekPlanChanged && announceWeekPlan)
        m_signalWeekPlanChanged.emit();

    if (plansChanged)
        m_signalPlansChanged.emit();
}

PlannerDocument::TimePoint PlannerModel::nextUpdateTime() const
{
    const PlannerDocument::TimePoint earliest = std::chrono::floor<std::chrono::seconds>(m_document.updatedAt) + std::chrono::seconds{ 1 };
    return std::max(m_clock.now(), earliest);
}

void PlannerModel::persist()
{
    m_document.updatedAt = nextUpdateTime();
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
    m_signalPlansChanged.emit();
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
