#include "ActivityDialog.hpp"

#include "../../core/locale/LocaleManager.hpp"

ActivityDialog::ActivityDialog(Window &parent) :
    Dialog("", parent, true),
    m_contentBox(Gtk::Orientation::VERTICAL, 8)
{
    const auto &loc = LocaleManager::instance();
    set_title(loc.translate("dialog.activity.add"));
    initLayout();
}

ActivityDialog::ActivityDialog(Window &parent, const Activity &existing) :
    Dialog("", parent, true),
    m_contentBox(Gtk::Orientation::VERTICAL, 8)
{
    const auto &loc = LocaleManager::instance();
    set_title(loc.translate("dialog.activity.edit"));
    initLayout();
    m_nameEntry.set_text(existing.name);
    setDifficulty(existing.difficulty);
}

void ActivityDialog::initLayout()
{
    const auto &loc = LocaleManager::instance();

    m_nameLabel.set_text(loc.translate("dialog.activity.name.label"));
    m_nameLabel.set_halign(Gtk::Align::START);
    m_nameEntry.set_placeholder_text(loc.translate("dialog.activity.name.placeholder"));

    m_difficultyLabel.set_text(loc.translate("dialog.activity.name.difficulty"));
    m_difficultyLabel.set_halign(Gtk::Align::START);

    m_difficultyModel = Gtk::StringList::create({
        loc.translate("activity.difficulty.LIGHT.label"),
        loc.translate("activity.difficulty.NORMAL.label"),
        loc.translate("activity.difficulty.MEDIUM.label"),
        loc.translate("activity.difficulty.HARD.label")
    });

    m_difficultyDropDown.set_model(m_difficultyModel);
    m_difficultyDropDown.set_selected(1);

    m_contentBox.set_margin(16);
    m_contentBox.append(m_nameLabel);
    m_contentBox.append(m_nameEntry);
    m_contentBox.append(m_difficultyLabel);
    m_contentBox.append(m_difficultyDropDown);

    get_content_area()->append(m_contentBox);

    add_button(loc.translate("button.cancel"), Gtk::ResponseType::CANCEL);
    add_button(loc.translate("button.ok"), Gtk::ResponseType::OK);

    set_default_response(Gtk::ResponseType::OK);
    m_nameEntry.set_activates_default(true);
    set_default_size(360, -1);
}

void ActivityDialog::setDifficulty(const Difficulty d)
{
    switch (d)
    {
            using enum Difficulty;
        case LIGHT: m_difficultyDropDown.set_selected(0);
            break;
        case NORMAL: m_difficultyDropDown.set_selected(1);
            break;
        case MEDIUM: m_difficultyDropDown.set_selected(2);
            break;
        case HARD: m_difficultyDropDown.set_selected(3);
            break;
    }
}

Difficulty ActivityDialog::getSelectedDifficulty() const
{
    switch (m_difficultyDropDown.get_selected())
    {
        using enum Difficulty;
        case 0: return LIGHT;
        case 1: return NORMAL;
        case 2: return MEDIUM;
        case 3: return HARD;
        default: return NORMAL;
    }
}

std::optional<Activity> ActivityDialog::getResult() const
{
    const auto name = m_nameEntry.get_text();
    if (name.empty())
        return std::nullopt;

    return Activity{name, getSelectedDifficulty()};
}
