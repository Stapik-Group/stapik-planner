#include "PlannerModel.hpp"
#include "../../infrastructure/storage/Storage.hpp"
#include "../../infrastructure/storage/PlannerSyncCoordinator.hpp"

#include <chrono>
#include <utility>
#include <glibmm/main.h>

PlannerModel::PlannerModel() : m_snapshot(Storage::load())
{}

const Settings& PlannerModel::settings() const
{
    return m_snapshot.settings;
}

const Activities& PlannerModel::activities() const
{
    return m_snapshot.activities;
}

const WeekPlan& PlannerModel::weekPlan() const
{
    return m_snapshot.weekPlan;
}

void PlannerModel::addActivity(Activity activity)
{
    m_snapshot.activities.push_back(std::move(activity));
    persist();
    m_signalActivitiesChanged.emit();
}

void PlannerModel::updateActivity(const std::size_t index, Activity activity)
{
    if (index >= m_snapshot.activities.size())
        return;

    m_snapshot.activities[index] = std::move(activity);
    persist();
    m_signalActivitiesChanged.emit();
}

void PlannerModel::removeActivity(const std::size_t index)
{
    if (index >= m_snapshot.activities.size())
        return;

    m_snapshot.activities.erase(m_snapshot.activities.begin() + static_cast<Activities::difference_type>(index));
    persist();
    m_signalActivitiesChanged.emit();
}

void PlannerModel::updateSettings(Settings settings)
{
    m_snapshot.settings = settings;
    persist();
    m_signalSettingsChanged.emit();
}

void PlannerModel::updateWeekPlan(const WeekPlan& weekPlan)
{
    m_snapshot.weekPlan = weekPlan;
    persist();
    m_signalWeekPlanChanged.emit();
}

void PlannerModel::updateDay(const Weekday weekday, DayPlan dayPlan)
{
    const auto index = static_cast<std::size_t>(weekday);
    if (index >= m_snapshot.weekPlan.size())
        return;

    m_snapshot.weekPlan[index] = std::move(dayPlan);
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

    const auto resolved = PlannerSyncCoordinator::resolveOnConnect(m_snapshot, *m_cloudClient);

    m_snapshot = resolved;
    Storage::save(m_snapshot);
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
    m_snapshot.lastUpdate = std::chrono::system_clock::now();

    if (m_cloudClient != nullptr)
    {
        g_message("[Cloud] Saving in cloud...");
        m_snapshot = PlannerSyncCoordinator::pushLocalChange(m_snapshot, *m_cloudClient);
        g_message("[Cloud] Saved in cloud.");
    }

    Storage::save(m_snapshot);
}