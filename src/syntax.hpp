#ifndef QUIZZER_SYNTAX_HPP
#define QUIZZER_SYNTAX_HPP

#include <ftxui/dom/elements.hpp>
#include <string>
#include <optional>

// render a code block with basic syntax highlighting based on language.
ftxui::Element render_code_block(
    const std::string& code,
    const std::optional<std::string>& language);

#endif
