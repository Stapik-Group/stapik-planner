#pragma once

#include "stapik/locale/Locale.hpp"
#include "stapik/locale/LocalizationEngine.hpp"

#include <sigc++/signal.h>
#include <filesystem>

class LocaleManager
{
public:
    static LocaleManager& instance();
    void setLocale(Locale locale);
    [[nodiscard]] Locale getLocale() const;
    [[nodiscard]] std::string translate(const std::string& key) const;
    sigc::signal<void()>& signalLocaleChanged();
private:
    LocaleManager();
    LocalizationEngine m_engine;
    sigc::signal<void()> m_signalLocaleChanged;
    static void saveLocale(Locale locale);
    static Locale loadSavedLocale();
    static std::filesystem::path localeConfigPath();
};
