#include "screens/remove_question.hpp"
#include "app.hpp"
#include "list_entry.hpp"
#include "nav.hpp"
#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>

using namespace ftxui;

ftxui::Component make_remove_question_screen(AppState& state)
{
    auto entry_boxes = make_entry_boxes();

    auto component = CatchEvent(Renderer([](bool) { return text(""); }), [&, entry_boxes](Event event) {
        if (state.target_indices.empty()) return false;
        int count = static_cast<int>(state.target_indices.size());

        int clicked = mouse_click_index(event, entry_boxes);
        if (clicked >= 0) { state.remove_question_idx = clicked; return true; }

        if (nav_up_down(event, state.remove_question_idx, count)) return true;
        if (nav_numeric(event, state.remove_question_idx, count)) return true;
        if (event == Event::Return)
        {
            int real_idx = state.target_indices[state.remove_question_idx];
            state.questions.erase(state.questions.begin() + real_idx);
            state.build_target_indices();
            state.status_message = "Question removed.";
            if (state.remove_question_idx >= static_cast<int>(state.target_indices.size()) &&
                !state.target_indices.empty())
            {
                state.remove_question_idx = static_cast<int>(state.target_indices.size()) - 1;
            }
            if (state.target_indices.empty())
                state.current_screen = AppScreen::MENU;
            return true;
        }
        if (event == Event::Escape || event == Event::Character('b'))
        {
            state.return_to_menu();
            return true;
        }
        return false;
    });

    return Renderer(component, [&, entry_boxes] {
        if (state.target_indices.empty())
            return text(" No questions for this file. ") | center | borderRounded;

        int count = static_cast<int>(state.target_indices.size());
        entry_boxes->resize(count);

        Elements body;
        body.push_back(text(""));
        body.push_back(text(" Remove Question ") | bold | center);
        body.push_back(text(""));
        body.push_back(separator() | color(Color::GrayDark));
        body.push_back(text(""));

        for (int i = 0; i < count; ++i)
        {
            bool sel = (i == state.remove_question_idx);
            const auto& q = state.questions[state.target_indices[i]];
            auto entry = hbox({
                text(sel ? " > " : "   "),
                text(std::to_string(i + 1) + ". ") | dim,
                text(q.question) | (sel ? bold : nothing),
                q.code
                    ? text(" [code]") | color(Color::Cyan) | dim
                    : text(""),
                q.explain
                    ? text(" [exp]") | color(Color::Yellow) | dim
                    : text(""),
            });
            if (sel) entry = entry | color(Color::Cyan) | focus;
            body.push_back(entry | reflect((*entry_boxes)[i]));
        }

        body.push_back(text(""));
        body.push_back(separator() | color(Color::GrayDark));
        body.push_back(text(" j/k navigate  Enter remove  Esc back ") | dim | center);

        return vbox(std::move(body)) | vscroll_indicator | yframe | flex | borderRounded;
    });
}
