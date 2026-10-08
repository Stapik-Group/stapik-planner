#include "PlannerModel.hpp"
#include "../../infrastructure/storage/PlannerStorage.hpp"
#include "../../infrastructure/storage/PlannerSyncCoordinator.hpp"

#include "stapik/log/Log.hpp"

#include <chrono>
#include <utility>
#include <glibmm/main.h>

PlannerModel::PlannerModel() :
    m_storage(std::make_unique<PlannerStorage>()),
    m_document(m_storage->load())
{}

PlannerModel::~PlannerModel() = default;

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
    m_document.activities.push_back(std::move(activity));
    persist();
    m_signalActivitiesChanged.emit();
}

void PlannerModel::updateActivity(const std::size_t index, Activity activity)
{
    if (index >= m_document.activities.size())
        return;

    m_document.activities[index] = std::move(activity);
    persist();
    m_signalActivitiesChanged.emit();
}

void PlannerModel::removeActivity(const std::size_t index)
{
    if (index >= m_document.activities.size())
        return;

    m_document.activities.erase(m_document.activities.begin() + static_cast<Activities::difference_type>(index));
    persist();
    m_signalActivitiesChanged.emit();
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

void PlannerModel::setCloudClient(std::unique_ptr<CloudStorageClient> client)
{
    m_cloudClient = std::move(client);
    syncFromCloud();
}

void PlannerModel::retrySync()
{
    syncFromCloud();
}

void PlannerModel::syncFromCloud()
{
    if (m_cloudClient == nullptr)
        return;

    m_document = PlannerSyncCoordinator::resolveOnConnect(m_document, *m_cloudClient);
    saveToDisk();
    m_signalActivitiesChanged.emit();
    m_signalSettingsChanged.emit();
    m_signalWeekPlanChanged.emit();
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

void PlannerModel::persist()
{
    m_document.updatedAt = std::chrono::system_clock::now();

    if (m_cloudClient != nullptr)
    {
        g_message("[Cloud] Saving in cloud...");
        m_document = PlannerSyncCoordinator::pushLocalChange(m_document, *m_cloudClient);
        g_message("[Cloud] Saved in cloud.");
    }

    saveToDisk();
}

void PlannerModel::saveToDisk()
{
    if (!m_storage->save(m_document))
        stapik::log::warning("Cannot save the planner to {}", m_storage->filePath().string());
}