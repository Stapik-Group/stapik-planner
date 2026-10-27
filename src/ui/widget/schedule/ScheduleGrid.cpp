#include "ScheduleGrid.hpp"

#include "stapik/locale/LocaleManager.hpp"
#include "stapik/ui/dialog/ConfirmDialog.hpp"
#include "stapik/ui/dialog/DialogUtils.hpp"

#include <glibmm/main.h>
#include <gtkmm/window.h>

ScheduleGrid::ScheduleGrid(PlannerModel& model) :
    m_model(model),
    m_slotCount(model.settings().slots)
{
    for (int i = 0; i < DAYS; ++i)
        m_cells.push_back(std::make_unique<ScheduleDayColumn>(model));

    initLayout();
    connectCellSignals();

    m_model.signalActivitiesChanged().connect([this]
    {
        for (const auto& cell : m_cells)
            cell->setActivities(m_model.activities());
    });
    m_model.signalWeekPlanChanged().connect([this]
    {
        const auto& weekPlan = m_model.weekPlan();
        for (int i = 0; i < DAYS; ++i)
            m_cells[i]->setDayPlan(weekPlan[static_cast<std::size_t>(i)]);
    });
    m_model.signalSettingsChanged().connect([this]
    {
        const auto slotCount = m_model.settings().slots;
        const auto& weekPlan = m_model.weekPlan();
        for (int i = 0; i < DAYS; ++i)
        {
            m_cells[i]->setSlotCount(slotCount);
            m_cells[i]->setActivities(m_model.activities());
            m_cells[i]->setDayPlan(weekPlan[static_cast<std::size_t>(i)]);
        }
        m_slotCount = slotCount;
    });
}

void ScheduleGrid::initLayout()
{
    set_column_homogeneous(true);
    set_hexpand(true);
    set_vexpand(true);

    set_column_spacing(ScheduleWeekdays::COLUMN_SPACING);
    set_row_spacing(0);

    for (int i = 0; i < DAYS; ++i)
    {
        m_cells[i]->setActivities(m_model.activities());
        m_cells[i]->setDayPlan(m_model.weekPlan()[i]);
        attach(*m_cells[i], i, 0);
    }
}

void ScheduleGrid::connectCellSignals()
{
    for (int i = 0; i < DAYS; ++i)
    {
        const auto weekday = ScheduleWeekdays::ALL[static_cast<std::size_t>(i)];

        m_cells[i]->signalChanged().connect([this, i] { onDayChanged(i); });
        m_cells[i]->signalCopyRequested().connect([this, weekday](const Weekday target) { onCopyRequested(weekday, target); });
        m_cells[i]->signalClearRequested().connect([this, weekday] { m_model.clearDay(weekday); });
        m_cells[i]->signalDropRequested().connect([this, weekday](const DragPayload& payload, const std::size_t slotIndex) { onDropRequested(weekday, slotIndex, payload); });
    }
}

void ScheduleGrid::onDayChanged(const int index)
{
    const auto cellIndex = static_cast<std::size_t>(index);
    if (m_model.updateDay(ScheduleWeekdays::ALL[cellIndex], m_cells[cellIndex]->getDayPlan()))
        return;

    m_cells[cellIndex]->setDayPlan(m_model.weekPlan()[cellIndex]);

    if (auto* window = dynamic_cast<Gtk::Window*>(get_root()))
    {
        const auto& localeManager = LocaleManager::instance();
        showMessageDialog(*window, localeManager.translate("dialog.dayLimit.title"), localeManager.translate("dialog.dayLimit.slot"));
    }
}

void ScheduleGrid::onCopyRequested(const Weekday from, const Weekday to)
{
    const auto& weekPlan = m_model.weekPlan();
    const auto& source = weekPlan[static_cast<std::size_t>(from)];
    const auto& target = weekPlan[static_cast<std::size_t>(to)];

    if (target.isEmpty() || target.slots == source.slots)
    {
        copyDay(from, to);
        return;
    }

    auto* window = dynamic_cast<Gtk::Window*>(get_root());
    if (window == nullptr)
        return;

    const auto& localeManager = LocaleManager::instance();
    const LocalizationEngine::Arguments arguments = {
        { "from", localeManager.translate(ScheduleWeekdays::FULL_KEYS[static_cast<std::size_t>(from)]) },
        { "day", localeManager.translate(ScheduleWeekdays::FULL_KEYS[static_cast<std::size_t>(to)]) }
    };

    showConfirmDialog(
        *window,
        ConfirmDialogOptions{
            .title = localeManager.translate("dialog.copyDay.title"),
            .message = localeManager.translate("dialog.copyDay.message", arguments),
            .confirmLabel = localeManager.translate("dialog.copyDay.button"),
            .destructive = true
        },
        [this, from, to] { copyDay(from, to); });
}

void ScheduleGrid::copyDay(const Weekday from, const Weekday to)
{
    if (m_model.copyDay(from, to))
        return;

    if (auto* window = dynamic_cast<Gtk::Window*>(get_root()))
    {
        const auto& localeManager = LocaleManager::instance();
        showMessageDialog(*window, localeManager.translate("dialog.dayLimit.title"), localeManager.translate("dialog.dayLimit.copy"));
    }
}

void ScheduleGrid::onDropRequested(const Weekday target, const std::size_t slotIndex, const DragPayload& payload)
{
    Glib::signal_idle().connect_once([this, target, slotIndex, payload] { applyDrop(target, slotIndex, payload); });
}

void ScheduleGrid::applyDrop(const Weekday target, const std::size_t slotIndex, const DragPayload& payload)
{
    const bool applied = payload.kind == DragPayload::Kind::Activity
        ? m_model.assignSlot(target, slotIndex, payload.activityId)
        : m_model.transferSlot(payload.day, payload.slot, target, slotIndex, payload.copy);
    if (applied)
        return;

    if (auto* window = dynamic_cast<Gtk::Window*>(get_root()))
    {
        const auto& localeManager = LocaleManager::instance();
        showMessageDialog(*window, localeManager.translate("dialog.dayLimit.title"), localeManager.translate("dialog.dayLimit.slot"));
    }
}
