#include "screens/menu.hpp"
#include "app.hpp"
#include "banner.hpp"
#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>

using namespace ftxui;

ftxui::Component make_menu_screen(AppState& state)
{
    static const std::vector<std::string> entries = {
        "  Take Quiz",
        "  Add Question",
        "  Remove Question",
        "  Change Answer",
        "  Edit Choice",
        "  List Questions",
        "  Set Author and Name",
        "  Save",
        "  Load Quiz File",
    };

    auto menu = Menu(&entries, &state.menu_selected);
    auto component = Container::Vertical({menu});

    component |= CatchEvent([&](Event event) {
        if (event.is_character())
        {
            char ch = event.character()[0];
            if (ch >= '1' && ch <= '9')
                state.menu_selected = ch - '1';
            else
                return false;
        }

        if (event == Event::Return ||
            (event.is_character() && event.character()[0] >= '1' && event.character()[0] <= '9'))
        {
            switch (state.menu_selected)
            {
                case 0:
                    if (state.questions.empty())
                    {
                        state.status_message = "No questions loaded.";
                    }
                    else if (state.loaded_files.size() <= 1)
                    {
                        state.start_quiz();
                    }
                    else
                    {
                        state.quiz_setup_phase = 0;
                        state.quiz_setup_cursor = 0;
                        state.quiz_file_included.assign(state.loaded_files.size(), true);
                        state.quiz_file_order.clear();
                        state.current_screen = AppScreen::QUIZ_SETUP;
                    }
                    return true;
                case 1:
                    state.reset_add_form();
                    state.current_screen = state.route_to(AppScreen::ADD_QUESTION);
                    return true;
                case 2:
                    if (state.questions.empty())
                        state.status_message = "No questions to remove.";
                    else
                    {
                        state.remove_question_idx = 0;
                        state.current_screen = state.route_to(AppScreen::REMOVE_QUESTION);
                    }
                    return true;
                case 3:
                    if (state.questions.empty())
                        state.status_message = "No questions available.";
                    else
                    {
                        state.select_question_idx = 0;
                        state.select_new_answer = 0;
                        state.change_answer_phase = 0;
                        state.current_screen = state.route_to(AppScreen::CHANGE_ANSWER);
                    }
                    return true;
                case 4:
                    if (state.questions.empty())
                        state.status_message = "No questions available.";
                    else
                    {
                        state.edit_choice_question_idx = 0;
                        state.edit_choice_choice_idx = 0;
                        state.edit_choice_phase = 0;
                        state.current_screen = state.route_to(AppScreen::EDIT_CHOICE);
                    }
                    return true;
                case 5:
                    if (state.questions.empty())
                        state.status_message = "No questions available.";
                    else
                    {
                        state.list_selected = 0;
                        state.current_screen = state.route_to(AppScreen::LIST_QUESTIONS);
                    }
                    return true;
                case 6:
                {
                    state.current_screen = state.route_to(AppScreen::SET_METADATA);
                    if (state.current_screen == AppScreen::SET_METADATA)
                    {
                        if (state.target_file >= 0 && state.target_file < static_cast<int>(state.loaded_files.size()))
                        {
                            auto& lf = state.loaded_files[state.target_file];
                            state.meta_name_text = lf.name;
                            state.meta_author_text = lf.author;
                        }
                        else
                        {
                            state.meta_name_text = state.quiz_name;
                            state.meta_author_text = state.quiz_author;
                        }
                    }
                    return true;
                }
                case 7:
                    if (state.loaded_files.empty())
                    {
                        state.status_message = "No file loaded to save.";
                        return true;
                    }
                    state.current_screen = state.route_to(AppScreen::SAVE_CONFIRM);
                    if (state.current_screen == AppScreen::SAVE_CONFIRM)
                        state.compute_diff(state.target_file);
                    return true;
                case 8:
                    state.load_path_text.clear();
                    state.load_screen_mode = 1;
                    state.current_screen = AppScreen::LOAD_QUIZ;
                    return true;
            }
        }
        return false;
    });

    return Renderer(component, [&, menu] {
        bool has_changes = state.has_unsaved_changes();

        Elements info_parts;
        info_parts.push_back(text(" Randomise: ") | dim);
        info_parts.push_back(state.randomise
            ? text(" ON ") | color(Color::Green) | bold
            : text(" OFF ") | dim);
        info_parts.push_back(text("  ") | dim);
        info_parts.push_back(separator());
        info_parts.push_back(text("  ") | dim);
        info_parts.push_back(text(std::to_string(state.questions.size())) | bold);
        info_parts.push_back(text(" questions") | dim);
        info_parts.push_back(text("  ") | dim);
        info_parts.push_back(separator());
        info_parts.push_back(text("  ") | dim);
        if (!state.loaded_files.empty())
        {
            int file_count = static_cast<int>(state.loaded_files.size());
            if (file_count == 1)
                info_parts.push_back(text(state.loaded_files[0].filename) | dim);
            else
                info_parts.push_back(text(std::to_string(file_count) + " files loaded") | dim);
        }
        else
            info_parts.push_back(text("no file loaded") | color(Color::RedLight) | dim);
        info_parts.push_back(filler());
        if (has_changes)
            info_parts.push_back(text(" modified ") | color(Color::Yellow) | dim);
        auto info_bar = hbox(std::move(info_parts));

        Elements content;
        content.push_back(text(""));
        content.push_back(render_banner());
        content.push_back(text(""));
        content.push_back(separator() | color(Color::GrayDark));
        content.push_back(text(""));
        content.push_back(menu->Render() | vscroll_indicator | frame | flex);
        content.push_back(text(""));
        content.push_back(separator() | color(Color::GrayDark));
        content.push_back(info_bar);

        if (!state.status_message.empty())
        {
            content.push_back(separator() | color(Color::GrayDark));
            content.push_back(text(" " + state.status_message + " ") | color(Color::Yellow));
        }

        content.push_back(separator() | color(Color::GrayDark));
        content.push_back(text(" 1-9 select  R randomise  Enter confirm  q quit ") | dim | center);

        return vbox(std::move(content)) | borderRounded;
    });
}
