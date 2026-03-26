#ifndef QUIZZER_SCREEN_QUIT_CONFIRM_HPP
#define QUIZZER_SCREEN_QUIT_CONFIRM_HPP

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
struct AppState;

ftxui::Component make_quit_confirm_screen(AppState& state, ftxui::ScreenInteractive& screen);

#endif
