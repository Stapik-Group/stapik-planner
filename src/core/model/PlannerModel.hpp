#pragma once

#include "PlannerDocument.hpp"

#include "stapik/cloud/CloudStorageConfig.hpp"
#include "stapik/command/UndoStack.hpp"
#include "stapik/document/BackupInfo.hpp"
#include "stapik/sync/CloudSession.hpp"
#include "stapik/time/Clock.hpp"

#include <sigc++/connection.h>
#include <sigc++/signal.h>

#include <chrono>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <vector>

class PlannerStorage;

class PlannerModel
{
public:
    using CloudOutcome = stapik::sync::SyncOutcome<PlannerDocument>;
    using TimePoint = std::chrono::system_clock::time_point;

    // The name of the cloud slot. It identifies the planner document on the server, so it must not change.
    struct PlanSummary
    {
        std::string id;
        std::string name;
        bool active;
    };

    static constexpr auto CLOUD_SLOT_KEY = "planner.json";
    static constexpr std::chrono::milliseconds CLOUD_FLUSH_TIMEOUT{ 5000 };

    explicit PlannerModel();
    PlannerModel(std::unique_ptr<PlannerStorage> storage, stapik::sync::CloudSessionHooks cloudHooks, const stapik::sync::AsyncSyncOptions& syncOptions = {}, const stapik::time::IClock& clock = stapik::time::systemClock());
    ~PlannerModel();

    PlannerModel(const PlannerModel&) = delete;
    PlannerModel& operator=(const PlannerModel&) = delete;

    [[nodiscard]] const Settings& settings() const;
    [[nodiscard]] const Activities& activities() const;
    [[nodiscard]] const WeekPlan& weekPlan() const;

    [[nodiscard]] std::optional<std::size_t> findActivity(const std::string& id) const;
    [[nodiscard]] std::size_t plannedSlotCount(const std::string& id) const;

    void addActivity(Activity activity);
    bool updateActivity(std::size_t index, Activity activity);
    void removeActivity(std::size_t index);
    [[nodiscard]] bool updateSlotCount(int newSlotCount);
    bool updateDay(Weekday weekday, DayPlan dayPlan);
    bool copyDay(Weekday from, Weekday to);
    bool clearDay(Weekday weekday);
    bool assignSlot(Weekday day, std::size_t slotIndex, const std::string& activityId);
    bool transferSlot(Weekday fromDay, std::size_t fromSlot, Weekday toDay, std::size_t toSlot, bool copy);

    [[nodiscard]] std::vector<PlanSummary> plans() const;
    [[nodiscard]] const std::string& activePlanId() const;
    [[nodiscard]] const std::string& activePlanName() const;
    bool switchPlan(const std::string& id);
    bool createPlan(const std::string& name);
    bool duplicatePlan(const std::string& name);
    bool renamePlan(const std::string& id, const std::string& name);
    bool deletePlan(const std::string& id);

    [[nodiscard]] stapik::command::UndoStack& undoStack();

    [[nodiscard]] std::vector<stapik::document::BackupInfo> listBackups() const;
    [[nodiscard]] bool restoreBackup(const stapik::document::BackupInfo& backup);

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
    sigc::signal<void()>& signalPlansChanged();
    sigc::signal<void(stapik::sync::SyncStatus)>& signalCloudStatusChanged();
    sigc::signal<void(const CloudOutcome&)>& signalCloudOutcome();
    sigc::signal<void(TimePoint)>& signalCloudApiKeyExpiring();
    sigc::signal<void(const std::string&)>& signalSaveFailed();
private:
    struct Content
    {
        Settings settings;
        Activities activities;
        WeekPlan weekPlan;
        std::string activePlanId;
        std::string activePlanName;
        std::vector<SavedPlan> savedPlans;

        [[nodiscard]] bool operator==(const Content&) const = default;
    };

    class ContentCommand;

    std::unique_ptr<PlannerStorage> m_storage;
    PlannerDocument m_document;
    stapik::sync::CloudSession<PlannerDocument> m_cloudSession;
    const stapik::time::IClock& m_clock;
    stapik::command::UndoStack m_undoStack;

    sigc::signal<void()> m_signalActivitiesChanged;
    sigc::signal<void()> m_signalSettingsChanged;
    sigc::signal<void()> m_signalWeekPlanChanged;
    sigc::signal<void()> m_signalPlansChanged;
    sigc::signal<void(stapik::sync::SyncStatus)> m_signalCloudStatusChanged;
    sigc::signal<void(const CloudOutcome&)> m_signalCloudOutcome;
    sigc::signal<void(TimePoint)> m_signalCloudApiKeyExpiring;
    sigc::signal<void(const std::string&)> m_signalSaveFailed;
    sigc::connection m_deferredPush;
    bool m_saveFailing = false;

    [[nodiscard]] Content content() const;
    [[nodiscard]] bool isPlanNameUsable(const std::string& name, const std::string& exceptId) const;
    bool addPlan(const std::string& name, WeekPlan weekPlan, const char* descriptionKey);
    bool replaceDay(Weekday weekday, DayPlan dayPlan, const char* descriptionKey, bool announceWeekPlan);
    void execute(const char* descriptionKey, Content after, bool announceWeekPlan = true);
    void restoreContent(const Content& content, bool announceWeekPlan);
    [[nodiscard]] PlannerDocument::TimePoint nextUpdateTime() const;
    void persist();
    void saveToDisk();
    void onCloudOutcome(const CloudOutcome& outcome);
    void replaceWithCloudDocument(const CloudOutcome& outcome);
    void pushLocalEditAfterTheOutcomeIsHandled();
};