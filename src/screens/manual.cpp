#include "screens/manual.hpp"
#include "app.hpp"
#include "list_entry.hpp"
#include "nav.hpp"
#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <string>
#include <vector>

using namespace ftxui;

struct ManualSection
{
    std::string title;
    std::vector<std::string> lines;
};

static const std::vector<ManualSection>& get_sections()
{
    static const std::vector<ManualSection> sections = {
        {"Getting Started", {
            "Certamen is a terminal quiz game engine.",
            "Run it with quiz files or start empty:",
            "",
            "  certamen                     Start empty",
            "  certamen quiz.yaml           Load one file",
            "  certamen a.yaml b.yaml       Load multiple",
            "",
            "With no arguments, use Load Quiz File (option 9)",
            "to open .yaml files from within the TUI.",
            "",
            "Multiple files are tracked separately. Editing",
            "operations ask which file to target.",
        }},
        {"Main Menu", {
            "The main menu is navigated with j/k or arrow keys.",
            "Press 1-9 to jump directly to an entry.",
            "",
            " 1. Take Quiz         Start answering questions",
            " 2. Add Question      Write a new question",
            " 3. Remove Question   Delete a question",
            " 4. Change Answer     Pick a new correct answer",
            " 5. Edit Choice       Rewrite a choice's text",
            " 6. List Questions    Browse with detail panel",
            " 7. Set Author/Name   Edit quiz metadata",
            " 8. Save              Save to disk with diff",
            " 9. Load Quiz File    Load/unload .yaml files",
            "10. Manual            You are here",
            "",
            "Press r to toggle randomised order.",
            "Press q to quit (with confirmation).",
        }},
        {"Taking a Quiz", {
            "Select Take Quiz from the menu.",
            "",
            "If multiple files are loaded, you first choose",
            "which files to include (Space/Enter to toggle),",
            "then set the order they appear in (J/K to move).",
            "",
            "During the quiz:",
            "  j/k, arrows   Navigate answers",
            "  1-9           Jump to answer by number",
            "  Enter         Submit your answer",
            "",
            "After answering, the correct answer is shown",
            "in green. If an explanation exists, it appears",
            "below. Press Enter to advance.",
            "",
            "  q, Esc        Quit (confirmation prompt)",
            "",
            "At the end, a results screen shows your score",
            "with a breakdown of correct/incorrect answers.",
        }},
        {"Adding Questions", {
            "Add Question lets you compose a new question",
            "for the selected quiz file.",
            "",
            "Fields:",
            "  Question text    The prompt shown to players",
            "  Choices          2-10 answer options",
            "  Correct answer   Which choice is right",
            "  Code snippet     Optional, with language hint",
            "  Explanation      Optional, shown after answer",
            "",
            "Use the Add/Sub buttons to change choice count.",
            "Use +/- buttons to change the correct answer.",
            "",
            "The code snippet supports syntax highlighting",
            "for: C, C++, Python, Java, Haskell, JavaScript,",
            "Rust, Go, Ruby, and shell scripts.",
        }},
        {"Editing Questions", {
            "Remove Question:",
            "  Navigate with j/k, press Enter to delete.",
            "  Shows [code] and [exp] tags next to entries.",
            "",
            "Change Answer:",
            "  Phase 1: Pick a question (shows current answer)",
            "  Phase 2: Pick the new correct answer",
            "  Press Esc to go back a phase.",
            "",
            "Edit Choice:",
            "  Phase 1: Pick a question",
            "  Phase 2: Pick which choice to edit",
            "  Phase 3: Type the new text, Save or Cancel",
            "",
            "List Questions:",
            "  Split view: question list left, detail right.",
            "  Toggle what's shown with the checkboxes:",
            "    Answers   Highlight the correct choice",
            "    Code      Show code snippets",
            "    Explain   Show explanations",
        }},
        {"File Management", {
            "Load Quiz File (option 9) has two modes:",
            "",
            "  Input mode    Type a filepath and press Load",
            "  File list     Navigate loaded files with j/k",
            "                Press Right or Enter to unload",
            "",
            "Tab switches between modes.",
            "Esc returns to the main menu.",
            "",
            "When entering the screen, the cursor starts in",
            "the input field so you can type a path right away.",
            "",
            "Save (option 8) shows a diff of your changes",
            "before writing to disk. With multiple files,",
            "you pick which file to save.",
            "",
            "Set Author/Name (option 7) edits the name and",
            "author fields stored at the top of the .yaml.",
        }},
        {"Quiz Format", {
            "Quizzes are .yaml files with this structure:",
            "",
            "  name: My Quiz",
            "  author: yourname",
            "  questions:",
            "    - question: What is 2+2?",
            "      choices: [3, 4, 5, 6]",
            "      answer: 1",
            "      code: |",
            "        print(2+2)",
            "      language: python",
            "      explain: |",
            "        Basic arithmetic.",
            "",
            "Required: question, choices (>1), answer",
            "Optional: code, language, explain",
            "Optional top-level: name, author",
            "",
            "answer is 0-based (the TUI shows 1-based).",
            "See example_quiz.yaml for a working template.",
        }},
        {"SSH Server", {
            "Host quizzes for others over SSH:",
            "",
            "  certamen serve quiz.yaml",
            "  certamen serve --password pw quiz.yaml",
            "  certamen serve --port 3000 a.yaml b.yaml",
            "",
            "Options:",
            "  --port <N>          Listen port (default 2222)",
            "  --password <pw>     Require password",
            "  --key <path>        RSA host key path",
            "  --max-clients <N>   Connection limit (default 8)",
            "",
            "Clients connect with:",
            "  ssh -p 2222 playername@server-ip",
            "",
            "The SSH username becomes the player name.",
            "Each client gets an isolated quiz session.",
            "The server logs scores to stdout.",
            "",
            "A host key (certamen_host_rsa) is generated on",
            "first run. Ctrl+C stops the server.",
            "",
            "See SERVING.md for full server documentation.",
        }},
        {"Keybindings", {
            "Global:",
            "  q             Quit / exit current screen",
            "  Esc           Go back / cancel",
            "",
            "Navigation (most screens):",
            "  j / Up        Move up",
            "  k / Down      Move down",
            "  1-9           Jump by number",
            "  Enter         Confirm / select",
            "",
            "Main menu only:",
            "  r             Toggle randomise",
            "",
            "Quiz setup (multi-file):",
            "  Space/Enter   Toggle file inclusion",
            "  Tab           Proceed to ordering / go back",
            "  J (shift)     Move selected file down",
            "  K (shift)     Move selected file up",
            "",
            "Load Quiz File:",
            "  Tab           Switch file list / input",
            "  Right/Enter   Unload selected file",
        }},
    };
    return sections;
}

ftxui::Component make_manual_screen(AppState& state)
{
    auto focusable = Renderer([](bool) { return text(""); });
    auto entry_boxes = make_entry_boxes();

    auto component = CatchEvent(focusable, [&, entry_boxes](Event event) {
        const auto& sections = get_sections();
        int topic_count = static_cast<int>(sections.size());

        int clicked = mouse_click_index(event, entry_boxes);
        if (clicked >= 0)
        {
            state.manual_topic = clicked;
            state.manual_scroll = 0;
            return true;
        }

        if (nav_up_down(event, state.manual_topic, topic_count))
        {
            state.manual_scroll = 0;
            return true;
        }
        if (nav_numeric(event, state.manual_topic, topic_count)) return true;

        if (event == Event::Escape || event == Event::Character('q')
            || event == Event::Character('b'))
        {
            state.return_to_menu();
            return true;
        }

        return false;
    });

    return Renderer(component, [&, entry_boxes] {
        const auto& sections = get_sections();

        // left panel: topic list
        int topic_count = static_cast<int>(sections.size());
        entry_boxes->resize(topic_count);
        Elements toc_entries;
        for (int i = 0; i < topic_count; ++i)
        {
            bool sel = (i == state.manual_topic);
            auto entry = hbox({
                text(sel ? " > " : "   "),
                text(sections[i].title) | (sel ? bold : nothing),
            });
            if (sel) entry = entry | color(Color::Cyan) | focus;
            toc_entries.push_back(entry | reflect((*entry_boxes)[i]));
        }

        auto toc_panel = vbox({
            text(" Topics") | dim,
            text(""),
            vbox(std::move(toc_entries)),
        }) | vscroll_indicator | frame;

        // right panel: content for selected topic
        Elements content_lines;
        if (state.manual_topic >= 0 &&
            state.manual_topic < static_cast<int>(sections.size()))
        {
            const auto& sec = sections[state.manual_topic];
            content_lines.push_back(text(" " + sec.title) | bold | color(Color::Cyan));
            content_lines.push_back(text(""));

            for (const auto& line : sec.lines)
            {
                if (line.empty())
                    content_lines.push_back(text(""));
                else if (line.size() >= 2 && line[0] == ' ' && line[1] == ' ')
                    content_lines.push_back(text(line) | dim);
                else
                    content_lines.push_back(text(" " + line));
            }
        }

        auto content_panel = vbox(std::move(content_lines))
            | vscroll_indicator | yframe | flex;

        return vbox({
            text(""),
            text(" Manual ") | bold | center,
            text(""),
            separator() | color(Color::GrayDark),
            hbox({
                toc_panel | size(WIDTH, EQUAL, 24),
                separator() | color(Color::GrayDark),
                content_panel | flex,
            }) | flex,
            separator() | color(Color::GrayDark),
            text(" j/k topics  q/Esc back ") | dim | center,
        }) | borderRounded;
    });
}
