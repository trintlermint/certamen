#include "cli.hpp"
#include "model.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <limits>
#include <random>
#include <sstream>
#include <string>
#include <vector>

/*
* This is esentially the legacy "quizzer" CLI which was present as ./main.cpp in the source code.
* Now, this is reimplemented with the current TUI's features. Just for the CLI!
*/

static int read_int(int lo, int hi, const std::string& prompt)
{
    while (true)
    {
        std::cout << prompt;
        int n;
        if (std::cin >> n)
        {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            if (n >= lo && n <= hi) return n;
            std::cout << "Enter a number between " << lo << " and " << hi << ".\n";
        }
        else
        {
            if (std::cin.eof()) { std::cout << '\n'; std::exit(0); }
            std::cout << "Enter a number between " << lo << " and " << hi << ".\n";
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }
    }
}

static std::string read_line(const std::string& prompt)
{
    while (true)
    {
        std::cout << prompt;
        std::string line;
        if (!std::getline(std::cin, line)) { std::cout << '\n'; std::exit(0); }
        if (!line.empty()) return line;
        std::cout << "Input must be non-empty.\n";
    }
}

static std::string read_line_optional(const std::string& prompt)
{
    std::cout << prompt;
    std::string line;
    if (!std::getline(std::cin, line)) { std::cout << '\n'; std::exit(0); }
    return line;
}

static bool read_yes_no(const std::string& prompt)
{
    while (true)
    {
        std::cout << prompt << " (y/n): ";
        std::string line;
        if (!std::getline(std::cin, line)) { std::cout << '\n'; std::exit(0); }
        if (!line.empty())
        {
            char ch = static_cast<char>(std::tolower(static_cast<unsigned char>(line[0])));
            if (ch == 'y') return true;
            if (ch == 'n') return false;
        }
        std::cout << "Answer y or n.\n";
    }
}

static std::string read_multiline(const std::string& header)
{
    std::cout << header << "\n(End with a line containing only \"END\")\n";
    std::string result;
    std::string line;
    while (true)
    {
        if (!std::getline(std::cin, line)) { std::cout << '\n'; std::exit(0); }
        if (line == "END") break;
        if (!result.empty()) result.push_back('\n');
        result.append(line);
    }
    return result;
}

// state tracker
struct CliFile
{
    std::string filename;
    QuizFile    quiz;
    QuizFile    saved; // at load/save time for diff
};

struct CliState
{
    std::vector<CliFile> files;
    bool randomise = false;
};

static void print_diff(const CliState& state)
{
    bool any = false;
    for (const auto& cf : state.files)
    {
        bool file_header_printed = state.files.size() > 1;
        bool file_any = false;

        auto flush_header = [&]() {
            if (file_header_printed)
            {
                std::cout << "  " << cf.filename << ":\n";
                file_header_printed = false;
            }
            file_any = true;
            any = true;
        };

        if (cf.quiz.name != cf.saved.name)
        {
            flush_header();
            std::cout << "  [~] Name: \"" << cf.saved.name << "\" -> \"" << cf.quiz.name << "\"\n";
        }
        if (cf.quiz.author != cf.saved.author)
        {
            flush_header();
            std::cout << "  [~] Author: \"" << cf.saved.author << "\" -> \"" << cf.quiz.author << "\"\n";
        }

        for (const auto& orig : cf.saved.questions)
        {
            bool found = false;
            for (const auto& cur : cf.quiz.questions)
                if (cur.question == orig.question) { found = true; break; }
            if (!found) { flush_header(); std::cout << "  [-] Removed: " << orig.question << "\n"; }
        }
        for (const auto& cur : cf.quiz.questions)
        {
            bool found = false;
            for (const auto& orig : cf.saved.questions)
                if (orig.question == cur.question) { found = true; break; }
            if (!found) { flush_header(); std::cout << "  [+] Added: " << cur.question << "\n"; }
        }
        for (const auto& cur : cf.quiz.questions)
        {
            for (const auto& orig : cf.saved.questions)
            {
                if (cur.question != orig.question) continue;
                if (cur.answer != orig.answer)
                    { flush_header(); std::cout << "  [~] Answer changed: " << cur.question << "\n"; }
                if (cur.choices != orig.choices)
                    { flush_header(); std::cout << "  [~] Choices changed: " << cur.question << "\n"; }
                if (cur.code != orig.code)
                    { flush_header(); std::cout << "  [~] Code changed: " << cur.question << "\n"; }
                if (cur.explain != orig.explain)
                    { flush_header(); std::cout << "  [~] Explanation changed: " << cur.question << "\n"; }
                break;
            }
        }
        (void)file_any;
    }
    if (!any)
        std::cout << "No unsaved changes.\n";
}

static bool has_unsaved(const CliState& state)
{
    for (const auto& cf : state.files)
    {
        if (cf.quiz.name != cf.saved.name) return true;
        if (cf.quiz.author != cf.saved.author) return true;
        if (cf.quiz.questions != cf.saved.questions) return true;
    }
    return false;
}

// files > 1 => picker
static int pick_file(const CliState& state, const std::string& action)
{
    if (state.files.size() == 1) return 0;
    std::cout << "\nWhich file to " << action << "?\n";
    for (std::size_t i = 0; i < state.files.size(); ++i)
    {
        const auto& cf = state.files[i];
        std::cout << "  " << (i + 1) << ". " << cf.filename;
        if (!cf.quiz.name.empty()) std::cout << " (" << cf.quiz.name << ")";
        std::cout << " [" << cf.quiz.questions.size() << " questions]\n";
    }
    return read_int(1, static_cast<int>(state.files.size()),
                    "File (1-" + std::to_string(state.files.size()) + "): ") - 1;
}

static void print_question(const Question& q, std::size_t idx, std::size_t total)
{
    std::cout << "\nQuestion " << (idx + 1) << " of " << total << ": " << q.question << "\n";
    if (q.code && !q.code->empty())
    {
        std::cout << "\n::: code";
        if (q.language && !q.language->empty()) std::cout << " (" << *q.language << ")";
        std::cout << " :::\n" << *q.code << ":::\n";
    }
    for (std::size_t i = 0; i < q.choices.size(); ++i)
        std::cout << "  " << (i + 1) << ". " << q.choices[i] << "\n";
}

static std::vector<Question> build_quiz_pool(const CliState& state)
{
    if (state.files.size() == 1)
        return state.files[0].quiz.questions;

    std::cout << "\nWhich files to include? (space-separated numbers, or 'all')\n";
    for (std::size_t i = 0; i < state.files.size(); ++i)
    {
        std::cout << "  " << (i + 1) << ". " << state.files[i].filename
                  << " [" << state.files[i].quiz.questions.size() << " questions]\n";
    }

    std::cout << "Files: ";
    std::string line;
    if (!std::getline(std::cin, line)) { std::cout << '\n'; std::exit(0); }

    std::vector<int> chosen;
    if (line == "all" || line.empty())
    {
        for (std::size_t i = 0; i < state.files.size(); ++i)
            chosen.push_back(static_cast<int>(i));
    }
    else
    {
        std::istringstream ss(line);
        int n;
        while (ss >> n)
            if (n >= 1 && n <= static_cast<int>(state.files.size()))
                chosen.push_back(n - 1);
        if (chosen.empty())
            for (std::size_t i = 0; i < state.files.size(); ++i)
                chosen.push_back(static_cast<int>(i));
    }

    std::vector<Question> pool;
    for (int idx : chosen)
        for (const auto& q : state.files[idx].quiz.questions)
            pool.push_back(q);
    return pool;
}

static std::vector<Question> randomize(const std::vector<Question>& src, std::mt19937& rng)
{
    std::vector<Question> session;
    session.reserve(src.size());
    for (const auto& q : src)
    {
        std::vector<std::pair<std::string, bool>> entries;
        entries.reserve(q.choices.size());
        for (std::size_t i = 0; i < q.choices.size(); ++i)
            entries.emplace_back(q.choices[i], static_cast<int>(i) == q.answer);
        std::shuffle(entries.begin(), entries.end(), rng);

        Question sq;
        sq.question = q.question;
        sq.code     = q.code;
        sq.explain  = q.explain;
        sq.language = q.language;
        sq.choices.reserve(entries.size());
        sq.answer = 0;
        for (std::size_t i = 0; i < entries.size(); ++i)
        {
            sq.choices.push_back(std::move(entries[i].first));
            if (entries[i].second) sq.answer = static_cast<int>(i);
        }
        session.push_back(std::move(sq));
    }
    std::shuffle(session.begin(), session.end(), rng);
    return session;
}

// cmd_* denotes a given function associated with the CLI part of the program.

static void cmd_take_quiz(CliState& state)
{
    std::vector<Question> pool = build_quiz_pool(state);
    if (pool.empty()) { std::cout << "No questions to quiz.\n"; return; }

    std::mt19937 rng{std::random_device{}()};
    const std::vector<Question> session = state.randomise ? randomize(pool, rng) : pool;
    const std::size_t total = session.size();

    std::cout << "\nStarting quiz (" << total << " questions"
              << (state.randomise ? ", randomised" : "") << ")\n";

    int score = 0;
    for (std::size_t i = 0; i < total; ++i)
    {
        const Question& q = session[i];
        print_question(q, i, total);
        int ans = read_int(1, static_cast<int>(q.choices.size()), "Answer: ");

        if ((ans - 1) == q.answer)
        {
            std::cout << "Correct!\n";
            ++score;
        }
        else
        {
            std::cout << "Wrong. Correct answer: " << (q.answer + 1) << ". "
                      << q.choices[q.answer] << "\n";
        }

        if (q.explain && !q.explain->empty())
            std::cout << "Explanation: " << *q.explain << "\n";

        double pct = 100.0 * static_cast<double>(score) / static_cast<double>(total);
        std::cout << "Score: " << score << "/" << total
                  << " (" << std::fixed << std::setprecision(1) << pct << "%)\n";
    }

    double final_pct = 100.0 * static_cast<double>(score) / static_cast<double>(total);
    std::cout << "\nFinal: " << score << "/" << total
              << " (" << std::fixed << std::setprecision(1) << final_pct << "%)\n";
}

static void cmd_add_question(CliState& state)
{
    if (state.files.empty()) { std::cout << "No file loaded.\n"; return; }
    int fi = pick_file(state, "add a question to");

    Question q;
    q.question = read_line("Question: ");

    if (read_yes_no("Include a code snippet?"))
    {
        q.code = read_multiline("Paste code");
        std::string lang = read_line_optional("Language hint (blank to skip): ");
        if (!lang.empty()) q.language = lang;
    }

    if (read_yes_no("Include an explanation?"))
        q.explain = read_multiline("Enter explanation");

    int num_choices = read_int(2, 10, "Number of choices (2-10): ");
    q.choices.reserve(static_cast<std::size_t>(num_choices));
    for (int i = 0; i < num_choices; ++i)
        q.choices.push_back(read_line("Choice " + std::to_string(i + 1) + ": "));

    q.answer = read_int(1, num_choices,
                        "Correct choice (1-" + std::to_string(num_choices) + "): ") - 1;

    if (auto err = validate_question(q)) { std::cout << "Error: " << *err << "\n"; return; }

    state.files[fi].quiz.questions.push_back(std::move(q));
    std::cout << "Question added.\n";
}

static void cmd_remove_question(CliState& state)
{
    if (state.files.empty()) { std::cout << "No file loaded.\n"; return; }
    int fi = pick_file(state, "remove a question from");

    auto& questions = state.files[fi].quiz.questions;
    if (questions.empty()) { std::cout << "No questions.\n"; return; }

    std::cout << "\nRemove which question?\n";
    for (std::size_t i = 0; i < questions.size(); ++i)
    {
        std::cout << "  " << (i + 1) << ". " << questions[i].question
                  << (questions[i].code    ? " [code]"    : "")
                  << (questions[i].explain ? " [explain]" : "") << "\n";
    }

    int idx = read_int(1, static_cast<int>(questions.size()),
                       "Remove (1-" + std::to_string(questions.size()) + "): ") - 1;
    std::cout << "Removed: " << questions[idx].question << "\n";
    questions.erase(questions.begin() + idx);
}

static void cmd_change_answer(CliState& state)
{
    if (state.files.empty()) { std::cout << "No file loaded.\n"; return; }
    int fi = pick_file(state, "change an answer in");

    auto& questions = state.files[fi].quiz.questions;
    if (questions.empty()) { std::cout << "No questions.\n"; return; }

    std::cout << "\nChange answer for which question?\n";
    for (std::size_t i = 0; i < questions.size(); ++i)
        std::cout << "  " << (i + 1) << ". " << questions[i].question << "\n";

    int qi = read_int(1, static_cast<int>(questions.size()),
                      "Question (1-" + std::to_string(questions.size()) + "): ") - 1;
    Question& q = questions[qi];

    std::cout << "Choices:\n";
    for (std::size_t i = 0; i < q.choices.size(); ++i)
    {
        std::cout << "  " << (i + 1) << ". " << q.choices[i];
        if (static_cast<int>(i) == q.answer) std::cout << "  [current]";
        std::cout << "\n";
    }

    q.answer = read_int(1, static_cast<int>(q.choices.size()),
                        "New correct answer (1-" + std::to_string(q.choices.size()) + "): ") - 1;
    std::cout << "Answer updated.\n";
}

static void cmd_edit_choice(CliState& state)
{
    if (state.files.empty()) { std::cout << "No fil loaded.\n"; return; }
    int fi = pick_file(state, "edit a choice in");

    auto& questions = state.files[fi].quiz.questions;
    if (questions.empty()) { std::cout << "No questions.\n"; return; }

    std::cout << "\nEdit a choice in whic question?\n";
    for (std::size_t i = 0; i < questions.size(); ++i)
        std::cout << "  " << (i + 1) << ". " << questions[i].question << "\n";

    int qi = read_int(1, static_cast<int>(questions.size()),
                      "Question (1-" + std::to_string(questions.size()) + "): ") - 1;
    Question& q = questions[qi];

    std::cout << "Choices:\n";
    for (std::size_t i = 0; i < q.choices.size(); ++i)
    {
        std::cout << "  " << (i + 1) << ". " << q.choices[i];
        if (static_cast<int>(i) == q.answer) std::cout << "  [correct]";
        std::cout << "\n";
    }

    int ci = read_int(1, static_cast<int>(q.choices.size()),
                      "Edit choice (1-" + std::to_string(q.choices.size()) + "): ") - 1;
    std::cout << "Current text: " << q.choices[ci] << "\n";
    q.choices[ci] = read_line("New text: ");
    std::cout << "Choice updated.\n";
}

static void cmd_list_questions(const CliState& state)
{
    if (state.files.empty()) { std::cout << "No file loaded.\n"; return; }

    bool show_answers = read_yes_no("Show correct answers?");
    bool show_code    = read_yes_no("Show code snippets?");
    bool show_explain = read_yes_no("Show explanations?");

    for (const auto& cf : state.files)
    {
        if (state.files.size() > 1)
        {
            std::cout << "\n=== " << cf.filename;
            if (!cf.quiz.name.empty()) std::cout << " (" << cf.quiz.name << ")";
            std::cout << " ===\n";
        }

        if (cf.quiz.questions.empty()) { std::cout << "  (no questions)\n"; continue; }

        for (std::size_t i = 0; i < cf.quiz.questions.size(); ++i)
        {
            const Question& q = cf.quiz.questions[i];
            std::cout << "\n" << (i + 1) << ". " << q.question
                      << (q.code    ? " [code]"    : "")
                      << (q.explain ? " [explain]" : "") << "\n";

            if (show_code && q.code && !q.code->empty())
            {
                std::cout << "--- code";
                if (q.language && !q.language->empty()) std::cout << " (" << *q.language << ")";
                std::cout << " ---\n" << *q.code << "---\n";
            }

            for (std::size_t j = 0; j < q.choices.size(); ++j)
            {
                std::cout << "  " << (j + 1) << ". " << q.choices[j];
                if (show_answers && static_cast<int>(j) == q.answer) std::cout << "  [correct]";
                std::cout << "\n";
            }

            if (show_explain && q.explain && !q.explain->empty())
                std::cout << "Explanation: " << *q.explain << "\n";
        }
    }
}

static void cmd_set_metadata(CliState& state)
{
    if (state.files.empty()) { std::cout << "No file loaded.\n"; return; }
    int fi = pick_file(state, "set metadata for");

    auto& quiz = state.files[fi].quiz;
    std::cout << "Name [" << quiz.name << "]: ";
    std::string name;
    if (!std::getline(std::cin, name)) { std::cout << '\n'; std::exit(0); }
    if (!name.empty()) quiz.name = name;

    std::cout << "Author [" << quiz.author << "]: ";
    std::string author;
    if (!std::getline(std::cin, author)) { std::cout << '\n'; std::exit(0); }
    if (!author.empty()) quiz.author = author;

    std::cout << "Metadata updated.\n";
}

static void cmd_save(CliState& state)
{
    if (state.files.empty()) { std::cout << "No file loaded.\n"; return; }

    print_diff(state);
    if (!has_unsaved(state)) return;
    if (!read_yes_no("Save changes?")) return;

    for (auto& cf : state.files)
    {
        try
        {
            save_quiz(cf.quiz, cf.filename);
            cf.saved = cf.quiz;
            std::cout << "Saved " << cf.filename << ".\n";
        }
        catch (const std::exception& e)
        {
            std::cerr << "Save error (" << cf.filename << "): " << e.what() << "\n";
        }
    }
}

static void cmd_load_unload(CliState& state)
{
    std::cout << "\nLoaded files:\n";
    if (state.files.empty())
        std::cout << "  (none)\n";
    else
        for (std::size_t i = 0; i < state.files.size(); ++i)
            std::cout << "  " << (i + 1) << ". " << state.files[i].filename
                      << " [" << state.files[i].quiz.questions.size() << " questions]\n";

    std::cout << "l) Load a file\n"
                 "u) Unload a file\n"
                 "b) Back\n";
    std::cout << "Choice: ";
    std::string line;
    if (!std::getline(std::cin, line)) { std::cout << '\n'; std::exit(0); }
    if (line.empty()) return;
    char ch = static_cast<char>(std::tolower(static_cast<unsigned char>(line[0])));

    if (ch == 'l')
    {
        std::string path = read_line("Path: ");
        for (const auto& cf : state.files)
            if (cf.filename == path) { std::cout << "Already loaded: " << path << "\n"; return; }
        try
        {
            CliFile cf;
            cf.filename = path;
            cf.quiz     = load_quiz(path);
            cf.saved    = cf.quiz;
            std::cout << "Loaded " << cf.quiz.questions.size() << " questions from " << path << ".\n";
            state.files.push_back(std::move(cf));
        }
        catch (const std::exception& e)
        {
            std::cerr << "Load error: " << e.what() << "\n";
        }
    }
    else if (ch == 'u')
    {
        if (state.files.empty()) { std::cout << "No files loaded.\n"; return; }
        int idx = read_int(1, static_cast<int>(state.files.size()),
                           "Unload which (1-" + std::to_string(state.files.size()) + "): ") - 1;
        if (has_unsaved(state))
        {
            std::cout << "Unsaved changes in " << state.files[idx].filename << ".\n";
            if (!read_yes_no("Discard and unload?")) return;
        }
        std::cout << "Unloaded " << state.files[idx].filename << ".\n";
        state.files.erase(state.files.begin() + idx);
    }
}

// main for --cli

static void print_menu(const CliState& state)
{
    std::cout << "\nCertamen CLI"
              << " | Randomise: " << (state.randomise ? "ON" : "OFF")
              << " | Files: ";
    if (state.files.empty())
        std::cout << "(none)";
    else
        for (std::size_t i = 0; i < state.files.size(); ++i)
        {
            if (i > 0) std::cout << ", ";
            std::cout << state.files[i].filename;
        }
    std::cout << "\n"
                 " 1. Take Quiz\n"
                 " 2. Add Question\n"
                 " 3. Remove Question\n"
                 " 4. Change Answer\n"
                 " 5. Edit Choice\n"
                 " 6. List Questions\n"
                 " 7. Set Author/Name\n"
                 " 8. Save\n"
                 " 9. Load/Unload File\n"
                 "10. Toggle Randomise\n"
                 "11. Quit\n";
}

int run_cli(const std::vector<std::string>& files)
{
    CliState state;

    for (const auto& path : files)
    {
        try
        {
            CliFile cf;
            cf.filename = path;
            cf.quiz     = load_quiz(path);
            cf.saved    = cf.quiz;
            std::cout << "Loaded " << cf.quiz.questions.size() << " questions from " << path << ".\n";
            state.files.push_back(std::move(cf));
        }
        catch (const std::exception& e)
        {
            std::cerr << "Load error (" << path << "): " << e.what() << "\n";
        }
    }

    if (state.files.empty() && !files.empty())
        std::cout << "No files loaded. Use option 9 to load a quiz file.\n";
    else if (state.files.empty())
        std::cout << "No files provided. Use option 9 to load a quiz file.\n";

    while (true)
    {
        print_menu(state);
        int choice = read_int(1, 11, "Choose (1-11): ");
        switch (choice)
        {
            case 1:  cmd_take_quiz(state);       break;
            case 2:  cmd_add_question(state);    break;
            case 3:  cmd_remove_question(state); break;
            case 4:  cmd_change_answer(state);   break;
            case 5:  cmd_edit_choice(state);     break;
            case 6:  cmd_list_questions(state);  break;
            case 7:  cmd_set_metadata(state);    break;
            case 8:  cmd_save(state);            break;
            case 9:  cmd_load_unload(state);     break;
            case 10:
                state.randomise = !state.randomise;
                std::cout << "Randomise is now " << (state.randomise ? "ON" : "OFF") << ".\n";
                break;
            case 11:
                if (has_unsaved(state))
                {
                    std::cout << "Unsaved changes:\n";
                    print_diff(state);
                    if (!read_yes_no("Discard and quit?")) break;
                }
                return 0;
        }
    }
}
