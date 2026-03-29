#include "screens/quiz_setup.hpp"
#include "app.hpp"
#include "nav.hpp"
#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>

using namespace ftxui;

ftxui::Component make_quiz_setup_screen(AppState& state)
{
    auto focusable = Renderer([](bool) { return text(""); });

    auto component = CatchEvent(focusable, [&](Event event) {
        if (event == Event::Escape)
        {
            state.return_to_menu();
            return true;
        }

        int file_count = static_cast<int>(state.loaded_files.size());

        if (state.quiz_setup_phase == 0)
        {
            if (nav_up_down(event, state.quiz_setup_cursor, file_count)) return true;
            if (nav_numeric(event, state.quiz_setup_cursor, file_count)) return true;

            if (event == Event::Return || event == Event::Character(' '))
            {
                int idx = state.quiz_setup_cursor;
                if (idx < static_cast<int>(state.quiz_file_included.size()))
                    state.quiz_file_included[idx] = !state.quiz_file_included[idx];
                return true;
            }

            if (event == Event::Tab)
            {
                int selected_count = 0;
                for (bool inc : state.quiz_file_included)
                    if (inc) selected_count++;

                if (selected_count == 0)
                {
                    state.status_message = "Select at least one file.";
                    return true;
                }

                if (selected_count == 1)
                {
                    std::vector<Question> selected_questions;
                    for (int i = 0; i < file_count; ++i)
                    {
                        if (!state.quiz_file_included[i]) continue;
                        for (const auto& q : state.questions)
                            if (q.source_file == i)
                                selected_questions.push_back(q);
                    }
                    state.start_quiz_from(selected_questions);
                    return true;
                }
                state.quiz_file_order.clear();
                for (int i = 0; i < file_count; ++i)
                    if (state.quiz_file_included[i])
                        state.quiz_file_order.push_back(i);
                state.quiz_setup_cursor = 0;
                state.quiz_setup_phase = 1;
                return true;
            }

            return false;
        }

        // reorder
        int order_count = static_cast<int>(state.quiz_file_order.size());
        if (nav_up_down(event, state.quiz_setup_cursor, order_count)) return true;

        if (event == Event::Character('J'))
        {
            int pos = state.quiz_setup_cursor;
            if (pos < order_count - 1)
            {
                std::swap(state.quiz_file_order[pos], state.quiz_file_order[pos + 1]);
                state.quiz_setup_cursor++;
            }
            return true;
        }
        if (event == Event::Character('K'))
        {
            int pos = state.quiz_setup_cursor;
            if (pos > 0)
            {
                std::swap(state.quiz_file_order[pos], state.quiz_file_order[pos - 1]);
                state.quiz_setup_cursor--;
            }
            return true;
        }

        if (event == Event::Return)
        {
            // build question list in file (importred) order
            std::vector<Question> ordered_questions;
            for (int fi : state.quiz_file_order)
                for (const auto& q : state.questions)
                    if (q.source_file == fi)
                        ordered_questions.push_back(q);
            state.start_quiz_from(ordered_questions);
            return true;
        }

        if (event == Event::Tab)
        {
            // back i go!
            state.quiz_setup_cursor = 0;
            state.quiz_setup_phase = 0;
            return true;
        }

        return false;
    });

    return Renderer(component, [&] {
        int file_count = static_cast<int>(state.loaded_files.size());

        Elements body;
        body.push_back(text(""));

        if (state.quiz_setup_phase == 0)
        {
            body.push_back(text(" Select Quiz Files ") | bold | center);
            body.push_back(text(""));
            body.push_back(separator() | color(Color::GrayDark));
            body.push_back(text(""));

            for (int i = 0; i < file_count; ++i)
            {
                bool sel = (i == state.quiz_setup_cursor);
                bool included = (i < static_cast<int>(state.quiz_file_included.size()))
                                && state.quiz_file_included[i];
                const auto& lf = state.loaded_files[i];

                int qcount = 0;
                for (const auto& q : state.questions)
                    if (q.source_file == i) qcount++;

                auto entry = hbox({
                    text(sel ? " > " : "   "),
                    text(included ? "[x] " : "[ ] "),
                    text(lf.filename) | (sel ? bold : nothing),
                    text("  " + std::to_string(qcount) + "q") | dim,
                });
                if (sel) entry = entry | color(Color::Cyan);
                if (!included) entry = entry | dim;
                body.push_back(entry);
            }

            body.push_back(text(""));
            body.push_back(filler());
            body.push_back(separator() | color(Color::GrayDark));
            body.push_back(text(" j/k navigate  Space/Enter toggle  Tab next  Esc back ") | dim | center);
        }
        else
        {
            body.push_back(text(" Set Quiz Order ") | bold | center);
            body.push_back(text(""));
            body.push_back(separator() | color(Color::GrayDark));
            body.push_back(text(""));

            for (int pos = 0; pos < static_cast<int>(state.quiz_file_order.size()); ++pos)
            {
                bool sel = (pos == state.quiz_setup_cursor);
                int fi = state.quiz_file_order[pos];
                const auto& lf = state.loaded_files[fi];

                int qcount = 0;
                for (const auto& q : state.questions)
                    if (q.source_file == fi) qcount++;

                auto entry = hbox({
                    text(sel ? " > " : "   "),
                    text(std::to_string(pos + 1) + ". ") | dim,
                    text(lf.filename) | (sel ? bold : nothing),
                    text("  " + std::to_string(qcount) + "q") | dim,
                });
                if (sel) entry = entry | color(Color::Cyan);
                body.push_back(entry);
            }

            body.push_back(text(""));
            body.push_back(filler());
            body.push_back(separator() | color(Color::GrayDark));
            body.push_back(text(" j/k navigate  J/K reorder  Enter start  Tab back  Esc menu ") | dim | center);
        }

        return vbox(std::move(body)) | borderRounded;
    });
}
