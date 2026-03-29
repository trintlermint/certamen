#include "screens/add_question.hpp"
#include "app.hpp"
#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>

using namespace ftxui;

static constexpr int MAX_CHOICES = 10;

ftxui::Component make_add_question_screen(AppState& state)
{
    auto question_input = Input(&state.add_question_text, "Enter question text...");
    auto code_input = Input(&state.add_code_text, "Paste code here...");
    auto explain_input = Input(&state.add_explain_text, "Enter explanation...");
    auto language_input = Input(&state.add_language_text, "java, haskell, python...");

    auto code_checkbox = Checkbox(" Include code snippet", &state.add_include_code);
    auto explain_checkbox = Checkbox(" Include explanation", &state.add_include_explain);

    std::vector<Component> choice_inputs;
    for (int i = 0; i < MAX_CHOICES; ++i)
        choice_inputs.push_back(Input(&state.add_choices[i], "Choice text..."));

    auto add_choice_btn = Button(" Add ", [&] {
        if (state.add_num_choices < MAX_CHOICES)
            state.add_num_choices++;
    }, ButtonOption::Simple());

    // Track last known choice count so we can re-focus after Sub
    auto prev_num_choices = std::make_shared<int>(state.add_num_choices);

    auto sub_choice_btn = Button(" Sub ", [&] {
        if (state.add_num_choices > 2)
        {
            state.add_choices[state.add_num_choices - 1].clear();
            state.add_num_choices--;
            if (state.add_correct_idx >= state.add_num_choices)
                state.add_correct_idx = state.add_num_choices - 1;
        }
    }, ButtonOption::Simple());

    auto correct_up_btn = Button(" + ", [&] {
        if (state.add_correct_idx < state.add_num_choices - 1) state.add_correct_idx++;
    }, ButtonOption::Simple());

    auto correct_down_btn = Button(" - ", [&] {
        if (state.add_correct_idx > 0) state.add_correct_idx--;
    }, ButtonOption::Simple());

    auto save_btn = Button(" Save Question ", [&] {
        Question q;
        q.question = state.add_question_text;
        q.choices.assign(state.add_choices.begin(),
                         state.add_choices.begin() + state.add_num_choices);
        q.answer = state.add_correct_idx;
        if (state.add_include_code && !state.add_code_text.empty())
        {
            q.code = state.add_code_text;
            if (!state.add_language_text.empty())
                q.language = state.add_language_text;
        }
        if (state.add_include_explain && !state.add_explain_text.empty())
            q.explain = state.add_explain_text;

        if (auto err = validate_question(q))
        {
            state.status_message = "Error: " + *err;
            return;
        }

        q.source_file = state.target_file;
        state.questions.push_back(std::move(q));
        state.status_message = "Question added.";
        state.current_screen = AppScreen::MENU;
    }, ButtonOption::Simple());

    auto cancel_btn = Button(" Cancel ", [&] {
        state.return_to_menu();
    }, ButtonOption::Simple());

    auto choice_btns_row = Container::Horizontal({add_choice_btn, sub_choice_btn});
    auto correct_btns_row = Container::Horizontal({correct_down_btn, correct_up_btn});
    auto action_row = Container::Horizontal({save_btn, cancel_btn});

    auto inner = Container::Vertical({});

    auto component = CatchEvent(inner, [&](Event event) {
        if (event == Event::Escape)
        {
            state.return_to_menu();
            return true;
        }
        return false;
    });

    return Renderer(component, [&, inner, question_input, code_checkbox, code_input,
                                   language_input, explain_checkbox, explain_input,
                                   choice_inputs,
                                   choice_btns_row, correct_btns_row, action_row,
                                   save_btn, cancel_btn,
                                   add_choice_btn, sub_choice_btn,
                                   correct_up_btn, correct_down_btn,
                                   prev_num_choices] {
        // Re-focus on choice buttons if a choice was just removed
        if (state.add_num_choices < *prev_num_choices)
            choice_btns_row->TakeFocus();
        *prev_num_choices = state.add_num_choices;

        inner->DetachAllChildren();
        inner->Add(question_input);
        inner->Add(code_checkbox);
        if (state.add_include_code)
        {
            inner->Add(language_input);
            inner->Add(code_input);
        }
        inner->Add(explain_checkbox);
        if (state.add_include_explain)
            inner->Add(explain_input);

        for (int i = 0; i < state.add_num_choices; ++i)
            inner->Add(choice_inputs[i]);
        inner->Add(choice_btns_row);
        inner->Add(correct_btns_row);
        inner->Add(action_row);

        Elements body;
        body.push_back(text(""));
        body.push_back(text(" Add Question ") | bold | center);
        body.push_back(text(""));
        body.push_back(separator() | color(Color::GrayDark));

        body.push_back(text(""));
        body.push_back(text(" Question") | dim);
        body.push_back(question_input->Render() | borderRounded);
        body.push_back(text(""));

        body.push_back(code_checkbox->Render());
        if (state.add_include_code)
        {
            body.push_back(text("  Language") | dim);
            body.push_back(language_input->Render() | borderRounded
                | size(WIDTH, LESS_THAN, 30));
            body.push_back(text("  Code") | dim);
            body.push_back(code_input->Render() | borderRounded);
        }

        body.push_back(explain_checkbox->Render());
        if (state.add_include_explain)
            body.push_back(explain_input->Render() | borderRounded);

        body.push_back(text(""));
        body.push_back(separator() | color(Color::GrayDark));
        body.push_back(text(""));

        body.push_back(hbox({
            text(" Choices ") | dim,
            text(std::to_string(state.add_num_choices)) | bold,
            text("  "),
            add_choice_btn->Render(),
            text(" "),
            sub_choice_btn->Render(),
        }));
        body.push_back(text(""));

        for (int i = 0; i < state.add_num_choices; ++i)
        {
            bool is_correct = (i == state.add_correct_idx);
            body.push_back(hbox({
                text(is_correct ? " > " : "   "),
                text(std::to_string(i + 1) + ". ") | dim,
                choice_inputs[i]->Render() | flex,
            }) | (is_correct ? color(Color::Green) : nothing));
        }

        body.push_back(text(""));
        body.push_back(hbox({
            text(" Correct: ") | dim,
            text(std::to_string(state.add_correct_idx + 1)) | bold | color(Color::Green),
            text("  "),
            correct_down_btn->Render(),
            text(" "),
            correct_up_btn->Render(),
        }));

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

        return vbox(std::move(body)) | vscroll_indicator | frame | flex | borderRounded;
    });
}
