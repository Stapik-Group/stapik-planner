#include "ActivityName.hpp"

#include "stapik/text/TextFolding.hpp"

#include <algorithm>

namespace ActivityName
{
    std::string trimmed(const std::string_view name)
    {
        constexpr std::string_view whitespace = " \t\n\r\v\f";

        const auto first = name.find_first_not_of(whitespace);
        if (first == std::string_view::npos)
            return {};

        const auto last = name.find_last_not_of(whitespace);
        return std::string(name.substr(first, last - first + 1));
    }

    bool isTaken(const std::string_view name, const std::vector<std::string>& otherNames)
    {
        const auto folded = stapik::text::foldCase(trimmed(name));

        return std::ranges::any_of(otherNames, [&folded](const std::string& other)
        {
            return stapik::text::foldCase(trimmed(other)) == folded;
        });
    }
}
