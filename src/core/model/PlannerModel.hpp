#pragma once

#include "PlannerDocument.hpp"

#include "stapik/cloud/CloudStorageConfig.hpp"
#include "stapik/command/UndoStack.hpp"
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
    using TimePoint = std::chrono::system_clock::time_point;

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
    [[nodiscard]] bool updateSlotCount(int newSlotCount);
    void updateDay(Weekday weekday, DayPlan dayPlan);

    [[nodiscard]] stapik::command::UndoStack& undoStack();

    [[nodiscard]] bool connectCloud(const CloudStorageConfig& config);
    void syncCloud();
    void disconnectCloud();
    void flushCloud(std::chrono::milliseconds timeout = CLOUD_FLUSH_TIMEOUT);
    [[nodiscard]] bool isCloudConnected() const;
    [[nodiscard]] const std::optional<CloudStorageConfig>& cloudConfig() const;
    [[nodiscard]] stapik::sync::SyncStatus cloudStatus() const;

    sigc::signal<void()>& signalActivitiesChanged();
    sigc::signal<void()>& signalSettingsChanged();
    sigc::signal<void()>& signalWeekPlanChanged();
    sigc::signal<void(stapik::sync::SyncStatus)>& signalCloudStatusChanged();
    sigc::signal<void(const CloudOutcome&)>& signalCloudOutcome();
    sigc::signal<void(TimePoint)>& signalCloudApiKeyExpiring();
private:
    struct Content
    {
        Settings settings;
        Activities activities;
        WeekPlan weekPlan;

        [[nodiscard]] bool operator==(const Content&) const = default;
    };

    class ContentCommand;

    std::unique_ptr<PlannerStorage> m_storage;
    PlannerDocument m_document;
    stapik::sync::CloudSession<PlannerDocument> m_cloudSession;
    stapik::command::UndoStack m_undoStack;

    sigc::signal<void()> m_signalActivitiesChanged;
    sigc::signal<void()> m_signalSettingsChanged;
    sigc::signal<void()> m_signalWeekPlanChanged;
    sigc::signal<void(stapik::sync::SyncStatus)> m_signalCloudStatusChanged;
    sigc::signal<void(const CloudOutcome&)> m_signalCloudOutcome;
    sigc::signal<void(TimePoint)> m_signalCloudApiKeyExpiring;
    sigc::connection m_deferredPush;

    [[nodiscard]] Content content() const;
    void execute(const char* descriptionKey, Content after, bool announceWeekPlan = true);
    void restoreContent(const Content& content, bool announceWeekPlan);
    void persist();
    void saveToDisk();
    void onCloudOutcome(const CloudOutcome& outcome);
    void replaceWithCloudDocument(const CloudOutcome& outcome);
    void pushLocalEditAfterTheOutcomeIsHandled();
};