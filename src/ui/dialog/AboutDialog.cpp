#include "AboutDialog.hpp"

#include "../../core/locale/LocaleManager.hpp"
#include "Version.hpp"

AboutDialog::AboutDialog(Window& parent) :
    Dialog(LocaleManager::instance().translate("dialog.about.title"), parent, true),
    m_contentBox(Gtk::Orientation::VERTICAL, CONTENT_SPACING)
{
    initLayout();
}

void AboutDialog::initLayout()
{
    const auto& loc = LocaleManager::instance();

    m_titleLabel.set_markup(std::string("<b>") + AppInfo::NAME + "</b>");
    m_titleLabel.set_halign(Gtk::Align::CENTER);

    m_versionLabel.set_text(loc.translate("dialog.about.version") + " " + AppInfo::VERSION);
    m_versionLabel.set_halign(Gtk::Align::CENTER);

    m_authorLabel.set_text(loc.translate("dialog.about.author") + " " + AppInfo::AUTHOR);
    m_authorLabel.set_halign(Gtk::Align::CENTER);

    m_githubLabel.set_markup(std::string("<a href=\"") + AppInfo::GITHUB + "\">" + AppInfo::GITHUB + "</a>");
    m_githubLabel.set_halign(Gtk::Align::CENTER);

    m_contentBox.set_margin(CONTENT_MARGIN);
    m_contentBox.append(m_titleLabel);
    m_contentBox.append(m_versionLabel);
    m_contentBox.append(m_separator);
    m_contentBox.append(m_authorLabel);
    m_contentBox.append(m_githubLabel);

    get_content_area()->append(m_contentBox);

    add_button(loc.translate("button.ok"), Gtk::ResponseType::OK);
    set_default_response(Gtk::ResponseType::OK);
    set_default_size(DEFAULT_WIDTH, -1);
}