#pragma once

#include <gtkmm/dialog.h>
#include <gtkmm/label.h>
#include <gtkmm/box.h>
#include <gtkmm/separator.h>

class AboutDialog : public Gtk::Dialog
{
public:
    explicit AboutDialog(Window& parent);
private:
    static constexpr int CONTENT_SPACING = 8;
    static constexpr int CONTENT_MARGIN = 24;
    static constexpr int DEFAULT_WIDTH = 320;

    Gtk::Box m_contentBox;
    Gtk::Label m_titleLabel;
    Gtk::Separator m_separator;
    Gtk::Label m_versionLabel;
    Gtk::Label m_authorLabel;
    Gtk::Label m_githubLabel;

    void initLayout();
};