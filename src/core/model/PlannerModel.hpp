#pragma once

#include "Snapshot.hpp"
#include "stapik/cloud/CloudStorageClient.hpp"

#include <sigc++/signal.h>
#include <cstddef>
#include <memory>

class PlannerModel
{
public:
    explicit PlannerModel();

    [[nodiscard]] const Settings& settings() const;
    [[nodiscard]] const Activities& activities() const;
    [[nodiscard]] const WeekPlan& weekPlan() const;

    void addActivity(Activity activity);
    void updateActivity(std::size_t index, Activity activity);
    void removeActivity(std::size_t index);
    void updateSettings(Settings settings);
    [[nodiscard]] bool updateSlotCount(int newSlotCount);
    void updateWeekPlan(const WeekPlan& weekPlan);
    void updateDay(Weekday weekday, DayPlan dayPlan);

    void setCloudClient(std::unique_ptr<CloudStorageClient> client);
    void retrySync();

    sigc::signal<void()>& signalActivitiesChanged();
    sigc::signal<void()>& signalSettingsChanged();
    sigc::signal<void()>& signalWeekPlanChanged();
private:
    Snapshot m_snapshot;
    std::unique_ptr<CloudStorageClient> m_cloudClient;

    sigc::signal<void()> m_signalActivitiesChanged;
    sigc::signal<void()> m_signalSettingsChanged;
    sigc::signal<void()> m_signalWeekPlanChanged;

    void persist();
    void syncFromCloud();
};