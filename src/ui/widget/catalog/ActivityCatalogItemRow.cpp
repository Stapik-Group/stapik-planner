#include "ActivityCatalogItemRow.hpp"

#include "../../../core/model/DragPayload.hpp"

#include "stapik/locale/LocaleManager.hpp"

#include <gdkmm/contentprovider.h>
#include <glibmm/value.h>

ActivityCatalogItemRow::ActivityCatalogItemRow(const Activity &activity) : Box(Gtk::Orientation::HORIZONTAL, 8),
    m_activityId(activity.id)
{
    initLayout(activity);
    initDragSource();
}

void ActivityCatalogItemRow::initDragSource()
{
    m_dragSource = Gtk::DragSource::create();
    m_dragSource->set_actions(Gdk::DragAction::COPY | Gdk::DragAction::MOVE);
    m_dragSource->signal_prepare().connect([this](double, double)
    {
        Glib::Value<Glib::ustring> value;
        value.init(Glib::Value<Glib::ustring>::value_type());
        value.set(DragPayload::forActivity(m_activityId).serialize());
        return Gdk::ContentProvider::create(value);
    }, false);
    add_controller(m_dragSource);
}

void ActivityCatalogItemRow::initLayout(const Activity &activity)
{
    m_nameLabel.set_text(activity.name);
    m_nameLabel.set_halign(Gtk::Align::START);
    m_nameLabel.set_hexpand(true);

    m_difficultyLabel.set_text(difficultyLabel(activity.difficulty));
    m_difficultyLabel.set_halign(Gtk::Align::END);
    m_difficultyLabel.add_css_class("difficulty-label");

    m_editButton.set_label("✎");
    m_editButton.set_has_frame(false);
    m_editButton.signal_clicked().connect([this] { m_signalEditRequested.emit(); });

    m_deleteButton.set_label("✕");
    m_deleteButton.set_has_frame(false);
    m_deleteButton.signal_clicked().connect([this] { m_signalDeleteRequested.emit(); });

    set_margin(4);
    append(m_nameLabel);
    append(m_difficultyLabel);
    append(m_editButton);
    append(m_deleteButton);
    add_css_class("activity-row");
}

std::string ActivityCatalogItemRow::difficultyLabel(const Difficulty d)
{
    const auto &loc = LocaleManager::instance();
    switch (d)
    {
        using enum Difficulty;
        case LIGHT: return loc.translate("activity.difficulty.LIGHT.label") + " (10%)";
        case NORMAL: return loc.translate("activity.difficulty.NORMAL.label") + " (20%)";
        case MEDIUM: return loc.translate("activity.difficulty.MEDIUM.label") + " (30%)";
        case HARD: return loc.translate("activity.difficulty.HARD.label") + " (50%)";
    }
    return {};
}

sigc::signal<void()> &ActivityCatalogItemRow::signalEditRequested()
{
    return m_signalEditRequested;
}

sigc::signal<void()> &ActivityCatalogItemRow::signalDeleteRequested()
{
    return m_signalDeleteRequested;
}
