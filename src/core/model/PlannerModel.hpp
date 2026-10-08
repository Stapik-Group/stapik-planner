#pragma once

#include "PlannerDocument.hpp"

#include "stapik/cloud/CloudStorageConfig.hpp"
#include "stapik/sync/CloudSession.hpp"

#include <sigc++/connection.h>
#include <sigc++/signal.h>

#include <chrono>
#include <cstddef>
#include <memory>
#include <optional>

class PlannerStorage;

class PlannerModel
{
public:
    using CloudOutcome = stapik::sync::SyncOutcome<PlannerDocument>;

    // The name of the cloud slot. It identifies the planner document on the server, so it must not change.
    static constexpr auto CLOUD_SLOT_KEY = "planner.json";
    static constexpr std::chrono::milliseconds CLOUD_FLUSH_TIMEOUT{ 5000 };

    explicit PlannerModel();
    PlannerModel(std::unique_ptr<PlannerStorage> storage, stapik::sync::CloudSessionHooks cloudHooks, const stapik::sync::AsyncSyncOptions& syncOptions = {});
    ~PlannerModel();

    PlannerModel(const PlannerModel&) = delete;
    PlannerModel& operator=(const PlannerModel&) = delete;

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

    // Saves the configuration, connects and starts a synchronization in the background.
    // Returns false when the configuration is incomplete or no connection could be created.
    [[nodiscard]] bool connectCloud(const CloudStorageConfig& config);
    void syncCloud();
    // Pushes the pending changes and waits for the result, but not longer than the timeout.
    void flushCloud(std::chrono::milliseconds timeout = CLOUD_FLUSH_TIMEOUT);
    [[nodiscard]] bool isCloudConnected() const;
    [[nodiscard]] const std::optional<CloudStorageConfig>& cloudConfig() const;
    [[nodiscard]] stapik::sync::SyncStatus cloudStatus() const;

    sigc::signal<void()>& signalActivitiesChanged();
    sigc::signal<void()>& signalSettingsChanged();
    sigc::signal<void()>& signalWeekPlanChanged();
    sigc::signal<void(stapik::sync::SyncStatus)>& signalCloudStatusChanged();
    // Emitted after the model has applied the result of a synchronization.
    sigc::signal<void(const CloudOutcome&)>& signalCloudOutcome();
private:
    std::unique_ptr<PlannerStorage> m_storage;
    PlannerDocument m_document;
    stapik::sync::CloudSession<PlannerDocument> m_cloudSession;

    sigc::signal<void()> m_signalActivitiesChanged;
    sigc::signal<void()> m_signalSettingsChanged;
    sigc::signal<void()> m_signalWeekPlanChanged;
    sigc::signal<void(stapik::sync::SyncStatus)> m_signalCloudStatusChanged;
    sigc::signal<void(const CloudOutcome&)> m_signalCloudOutcome;
    sigc::connection m_deferredPush;

    void persist();
    void saveToDisk();
    void onCloudOutcome(const CloudOutcome& outcome);
    void replaceWithCloudDocument(const CloudOutcome& outcome);
    void pushLocalEditAfterTheOutcomeIsHandled();
};