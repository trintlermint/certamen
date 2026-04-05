#include "screens/list_questions.hpp"
#include "app.hpp"
#include "list_entry.hpp"
#include "nav.hpp"
#include "syntax.hpp"
#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>

using namespace ftxui;

ftxui::Component make_list_questions_screen(AppState& state)
{
    auto answer_toggle  = Checkbox(" Answers", &state.list_show_answers);
    auto code_toggle    = Checkbox(" Code", &state.list_show_code);
    auto explain_toggle = Checkbox(" Explain", &state.list_show_explain);

    auto controls = Container::Horizontal({answer_toggle, code_toggle, explain_toggle});
    auto entry_boxes = make_entry_boxes();

    auto component = Container::Vertical({controls});

    component |= CatchEvent([&, entry_boxes](Event event) {
        int count = static_cast<int>(state.target_indices.size());

        int clicked = mouse_click_index(event, entry_boxes);
        if (clicked >= 0) { state.list_selected = clicked; return true; }

        if (nav_up_down(event, state.list_selected, count)) return true;
        if (event == Event::Character('b') || event == Event::Escape)
        {
            state.return_to_menu();
            return true;
        }
        return false;
    });

    return Renderer(component, [&, answer_toggle, code_toggle, explain_toggle, entry_boxes] {
        int count = static_cast<int>(state.target_indices.size());
        entry_boxes->resize(count);

        Elements list_entries;
        for (int i = 0; i < count; ++i)
        {
            bool selected = (i == state.list_selected);
            const auto& q = state.questions[state.target_indices[i]];
            auto entry = hbox({
                text(selected ? " > " : "   "),
                text(std::to_string(i + 1) + ". ") | dim,
                text(q.question) | (selected ? bold : nothing),
                q.code
                    ? text(" [code]") | color(Color::Cyan) | dim
                    : text(""),
                q.explain
                    ? text(" [exp]") | color(Color::Yellow) | dim
                    : text(""),
            });
            if (selected) entry = entry | color(Color::Cyan) | focus;
            list_entries.push_back(entry | reflect((*entry_boxes)[i]));
        }

        auto list_panel = vbox(std::move(list_entries))
            | vscroll_indicator | yframe | flex;

        Elements detail;
        if (state.list_selected >= 0 &&
            state.list_selected < static_cast<int>(state.target_indices.size()))
        {
            const auto& q = state.questions[state.target_indices[state.list_selected]];

            detail.push_back(paragraph(q.question) | bold);

            if (state.list_show_code && q.code && !q.code->empty())
            {
                detail.push_back(text(""));
                detail.push_back(render_code_block(*q.code, q.language));
            }

            detail.push_back(text(""));
            for (std::size_t j = 0; j < q.choices.size(); ++j)
            {
                bool correct = (static_cast<int>(j) == q.answer);
                auto line = hbox({
                    text("  " + std::to_string(j + 1) + ". "),
                    paragraph(q.choices[j]) | flex
                        | (state.list_show_answers && correct ? color(Color::Green) : nothing),
                    text(state.list_show_answers && correct ? "  correct" : "") | dim,
                });
                detail.push_back(line);
            }

            if (state.list_show_explain && q.explain && !q.explain->empty())
            {
                detail.push_back(text(""));
                detail.push_back(paragraph(" " + *q.explain) | color(Color::Yellow) | dim);
            }
        }
        else
        {
            detail.push_back(text(" Select a question ") | dim | center);
        }

        auto detail_panel = vbox(std::move(detail))
            | vscroll_indicator | yframe | flex
            | size(HEIGHT, GREATER_THAN, 15);

        auto toggles_bar = hbox({
            filler(),
            answer_toggle->Render(),
            text("  "),
            code_toggle->Render(),
            text("  "),
            explain_toggle->Render(),
            filler(),
        });

        return vbox({
            text(""),
            text(" Questions ") | bold | center,
            text(""),
            separator() | color(Color::GrayDark),
            toggles_bar,
            separator() | color(Color::GrayDark),
            hbox({
                list_panel | size(WIDTH, EQUAL, 42),
                separator() | color(Color::GrayDark),
                detail_panel | flex,
            }) | flex,
            separator() | color(Color::GrayDark),
            text(" j/k navigate  Tab toggles  Esc back ") | dim | center,
        }) | borderRounded;
    });
}
