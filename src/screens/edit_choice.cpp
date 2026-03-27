#include "screens/edit_choice.hpp"
#include "app.hpp"
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
        auto& q = state.questions[state.edit_choice_question_idx];
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
        if (state.questions.empty()) return false;
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
            int count = static_cast<int>(state.questions.size());
            if (event == Event::ArrowUp || event == Event::Character('k'))
            {
                if (state.edit_choice_question_idx > 0) state.edit_choice_question_idx--;
                return true;
            }
            if (event == Event::ArrowDown || event == Event::Character('j'))
            {
                if (state.edit_choice_question_idx < count - 1) state.edit_choice_question_idx++;
                return true;
            }
            if (event.is_character())
            {
                char ch = event.character()[0];
                int num = ch - '1';
                if (num >= 0 && num < count)
                {
                    state.edit_choice_question_idx = num;
                    return true;
                }
            }
            if (event == Event::Return)
            {
                state.edit_choice_phase = 1;
                state.edit_choice_choice_idx = 0;
                return true;
            }
        }
        else if (state.edit_choice_phase == 1)
        {
            const auto& q = state.questions[state.edit_choice_question_idx];
            int num_choices = static_cast<int>(q.choices.size());

            if (event == Event::ArrowUp || event == Event::Character('k'))
            {
                if (state.edit_choice_choice_idx > 0) state.edit_choice_choice_idx--;
                return true;
            }
            if (event == Event::ArrowDown || event == Event::Character('j'))
            {
                if (state.edit_choice_choice_idx < num_choices - 1) state.edit_choice_choice_idx++;
                return true;
            }
            if (event.is_character())
            {
                char ch = event.character()[0];
                int num = ch - '1';
                if (num >= 0 && num < num_choices)
                {
                    state.edit_choice_choice_idx = num;
                    return true;
                }
            }
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
            {
                state.current_screen = AppScreen::MENU;
                state.status_message.clear();
            }
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

        if (state.questions.empty())
            return text(" No questions. ") | center | borderRounded;

        Elements body;

        if (state.edit_choice_phase == 0)
        {
            body.push_back(text(""));
            body.push_back(text(" Edit Choice ") | bold | center);
            body.push_back(text(""));
            body.push_back(separator() | color(Color::GrayDark));
            body.push_back(text("")); // as many as possibleeee

            for (int i = 0; i < static_cast<int>(state.questions.size()); ++i)
            {
                bool sel = (i == state.edit_choice_question_idx);
                auto entry = hbox({
                    text(sel ? " > " : "   "),
                    text(std::to_string(i + 1) + ". ") | dim,
                    text(state.questions[i].question) | (sel ? bold : nothing),
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
            const auto& q = state.questions[state.edit_choice_question_idx];

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
                    text(q.choices[i]) | (sel ? bold : nothing),
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
            const auto& q = state.questions[state.edit_choice_question_idx];

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

        return vbox(std::move(body)) | vscroll_indicator | frame | flex | borderRounded;
    });
}
