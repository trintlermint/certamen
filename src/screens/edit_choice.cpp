#include "screens/edit_choice.hpp"
#include "app.hpp"
#include "nav.hpp"
#include "syntax.hpp"
#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>

using namespace ftxui;

ftxui::Component make_edit_choice_screen(AppState& state)
{
    auto text_input = Input(&state.edit_choice_text, "Choice text...");

    auto save_btn = Button(" Save ", [&] {
        if (state.edit_choice_text.empty())
        {
            state.status_message = "Choice text cannot be empty.";
            return;
        }
        int real_idx = state.target_indices[state.edit_choice_question_idx];
        auto& q = state.questions[real_idx];
        q.choices[state.edit_choice_choice_idx] = state.edit_choice_text;
        state.status_message = "Choice updated.";
        state.edit_choice_phase = 1;
    }, ButtonOption::Simple());

    auto cancel_btn = Button(" Cancel ", [&] {
        state.edit_choice_phase = 1;
    }, ButtonOption::Simple());

    auto action_row = Container::Horizontal({save_btn, cancel_btn});
    auto edit_container = Container::Vertical({text_input, action_row});
    auto focusable = Renderer([](bool) { return text(""); });

    auto inner = Container::Vertical({});

    auto component = CatchEvent(inner, [&](Event event) {
        if (state.target_indices.empty()) return false;
        if (state.edit_choice_phase == 2)
        {
            if (event == Event::Escape)
            {
                state.edit_choice_phase = 1;
                return true;
            }
            return false;
        }

        if (state.edit_choice_phase == 0)
        {
            int count = static_cast<int>(state.target_indices.size());
            if (nav_up_down(event, state.edit_choice_question_idx, count)) return true;
            if (nav_numeric(event, state.edit_choice_question_idx, count)) return true;
            if (event == Event::Return)
            {
                state.edit_choice_phase = 1;
                state.edit_choice_choice_idx = 0;
                return true;
            }
        }
        else if (state.edit_choice_phase == 1)
        {
            int real_idx = state.target_indices[state.edit_choice_question_idx];
            const auto& q = state.questions[real_idx];
            int num_choices = static_cast<int>(q.choices.size());
            if (nav_up_down(event, state.edit_choice_choice_idx, num_choices)) return true;
            if (nav_numeric(event, state.edit_choice_choice_idx, num_choices)) return true;
            if (event == Event::Return)
            {
                state.edit_choice_text = q.choices[state.edit_choice_choice_idx];
                state.edit_choice_phase = 2;
                return true;
            }
        }

        if (event == Event::Escape || event == Event::Character('b'))
        {
            if (state.edit_choice_phase == 1)
                state.edit_choice_phase = 0;
            else
                state.return_to_menu();
            return true;
        }

        return false;
    });

    return Renderer(component, [&, inner, focusable, edit_container,
                                   text_input, save_btn, cancel_btn] {
        inner->DetachAllChildren();
        if (state.edit_choice_phase == 2)
            inner->Add(edit_container);
        else
            inner->Add(focusable);

        if (state.target_indices.empty())
            return text(" No questions for this file. ") | center | borderRounded;

        Elements body;

        if (state.edit_choice_phase == 0)
        {
            body.push_back(text(""));
            body.push_back(text(" Edit Choice ") | bold | center);
            body.push_back(text(""));
            body.push_back(separator() | color(Color::GrayDark));
            body.push_back(text(""));

            for (int i = 0; i < static_cast<int>(state.target_indices.size()); ++i)
            {
                bool sel = (i == state.edit_choice_question_idx);
                const auto& q = state.questions[state.target_indices[i]];
                auto entry = hbox({
                    text(sel ? " > " : "   "),
                    text(std::to_string(i + 1) + ". ") | dim,
                    text(q.question) | (sel ? bold : nothing),
                });
                if (sel) entry = entry | color(Color::Cyan);
                body.push_back(entry);
            }

            body.push_back(text(""));
            body.push_back(separator() | color(Color::GrayDark));
            body.push_back(text(" j/k navigate  Enter select  Esc back ") | dim | center);
        }
        else if (state.edit_choice_phase == 1)
        {
            int real_idx = state.target_indices[state.edit_choice_question_idx];
            const auto& q = state.questions[real_idx];

            body.push_back(text(""));
            body.push_back(text(" Edit Choice ") | bold | center);
            body.push_back(text(""));
            body.push_back(separator() | color(Color::GrayDark));
            body.push_back(text(""));
            body.push_back(paragraph(" " + q.question) | bold);

            if (q.code && !q.code->empty())
            {
                body.push_back(text(""));
                body.push_back(render_code_block(*q.code, q.language));
            }

            body.push_back(text(""));

            for (int i = 0; i < static_cast<int>(q.choices.size()); ++i)
            {
                bool sel = (i == state.edit_choice_choice_idx);
                bool is_answer = (i == q.answer);
                auto choice_el = hbox({
                    text(sel ? " > " : "   "),
                    text(std::to_string(i + 1) + ". "),
                    paragraph(q.choices[i]) | flex | (sel ? bold : nothing),
                    text(is_answer ? "  (correct)" : "") | dim,
                });
                if (sel) choice_el = choice_el | color(Color::Cyan);
                else choice_el = choice_el | dim;
                body.push_back(choice_el);
            }

            body.push_back(text(""));
            body.push_back(separator() | color(Color::GrayDark));
            body.push_back(text(" j/k navigate  Enter edit  Esc back ") | dim | center);
        }
        else
        {
            int real_idx = state.target_indices[state.edit_choice_question_idx];
            const auto& q = state.questions[real_idx];

            body.push_back(text(""));
            body.push_back(text(" Edit Choice ") | bold | center);
            body.push_back(text(""));
            body.push_back(separator() | color(Color::GrayDark));
            body.push_back(text(""));
            body.push_back(paragraph(" " + q.question) | bold | dim);
            body.push_back(text(""));
            body.push_back(text(" Editing choice " +
                std::to_string(state.edit_choice_choice_idx + 1)) | dim);
            body.push_back(text_input->Render() | borderRounded);

            body.push_back(text(""));
            body.push_back(separator() | color(Color::GrayDark));
            body.push_back(text(""));
            body.push_back(hbox({
                filler(),
                save_btn->Render() | color(Color::Green),
                text("    "),
                cancel_btn->Render() | color(Color::RedLight),
                filler(),
            }));

            if (!state.status_message.empty())
            {
                body.push_back(text(""));
                body.push_back(text(" " + state.status_message) | color(Color::Yellow));
            }
        }

        return vbox(std::move(body)) | vscroll_indicator | yframe | flex | borderRounded;
    });
}
