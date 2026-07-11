#include "ActivityCatalogItemRow.hpp"

#include "stapik/locale/LocaleManager.hpp"

ActivityCatalogItemRow::ActivityCatalogItemRow(const Activity &activity) : Box(Gtk::Orientation::HORIZONTAL, 8)
{
    initLayout(activity);
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
