#ifndef CERTAMEN_DIFF_HPP
#define CERTAMEN_DIFF_HPP

#include <ftxui/dom/elements.hpp>
#include <string>
#include <vector>

inline ftxui::Elements render_diff_lines(const std::vector<std::string>& diff_lines)
{
    using namespace ftxui;
    Elements entries;
    for (const auto& line : diff_lines)
    {
        Color line_color = Color::Default;
        if (line.size() > 1 && line[0] == '[')
        {
            if (line[1] == '+') line_color = Color::Green;
            else if (line[1] == '-') line_color = Color::RedLight;
            else if (line[1] == '~') line_color = Color::Yellow;
        }
        entries.push_back(text("  " + line) | color(line_color));
    }
    return entries;
}

#endif
