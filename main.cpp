#include <iostream>
#include <vector>
#include <string>
#include <optional>
#include <fstream>
#include <limits>
#include <iomanip>
#include <random>
#include <cctype>
#include <cstdlib>
#include <algorithm>
#include <yaml-cpp/yaml.h>

struct Question
{
    std::string question;
    std::vector<std::string> choices;
    int answer; // 0-based index into 'choices'.
    std::optional<std::string> code;
    std::optional<std::string> explain;

    bool operator==(const Question& other) const
    {
        return question == other.question
            && choices  == other.choices
            && answer   == other.answer
            && code     == other.code
            && explain  == other.explain;
    }
};

static std::optional<std::string> validate_question(const Question& q)
{
    if (q.question.empty())
        return std::string("Question cannot be blank.");
    if (q.choices.size() < 2)
        return std::string("Question must have more than one (1) choice.");
    if (q.answer < 0 || q.answer >= static_cast<int>(q.choices.size()))
        return std::string("Answer provided is out of range of the number of choices provided.");
    return std::nullopt;
}

static int read_int_in_range(int min_val, int max_val, const std::string& prompt)
{
    while (true)
    {
        std::cout << prompt;
        int value;
        if (std::cin >> value)
        {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            if (value >= min_val && value <= max_val) return value;
            std::cout << "Invalid. Enter a number between "
                      << min_val << " and " << max_val << ".\n";
        }
        else
        {
            if (std::cin.eof()) { std::cout << "\n"; std::exit(0); }
            std::cout << "Invalid. Enter a number (integer) between "
                      << min_val << " and " << max_val << ".\n";
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
        if (!std::getline(std::cin, line)) { std::cout << "\n"; std::exit(0); }
        if (!line.empty()) return line;
        std::cout << "Invalid. Input must be non-empty. Re-enter.\n";
    }
}

static bool read_yes_no(const std::string& prompt)
{
    while (true)
    {
        std::cout << prompt << " (y/n): ";
        std::string line;
        if (!std::getline(std::cin, line)) { std::cout << "\n"; std::exit(0); }
        if (!line.empty())
        {
            char ch = static_cast<char>(std::tolower(line[0]));
            if (ch == 'y') return true;
            if (ch == 'n') return false;
        }
        std::cout << "Invalid. Answer with 'y' (as in yes) or 'n' (as in no).\n";
    }
}

static std::string read_multiline_until(const std::string& header, const std::string& terminator)
{
    std::cout << header << "\n";
    std::cout << "(Finish by entering a line with only " << terminator << ")\n";
    std::string result;
    std::string line;
    while (true)
    {
        if (!std::getline(std::cin, line)) { std::cout << "\n"; std::exit(0); }
        if (line == terminator) break;
        if (!result.empty()) result.push_back('\n');
        result.append(line);
    }
    return result;
}

static std::vector<Question> load_questions(const std::string& filename)
{
    YAML::Node root = YAML::LoadFile(filename);
    if (!root.IsSequence())
    {
        throw std::runtime_error(
            "YAML must be a sequence. Ensure your file starts with a"
            " dash (-) for each question. See example_quiz.yaml.");
    }

    std::vector<Question> questions;
    questions.reserve(root.size());

    for (const auto& item : root)
    {
        if (!item.IsMap())
        {
            throw std::runtime_error(
                "Each entry must be a map. Ensure each question starts with"
                " a dash (-) followed by keys like 'question', 'choices', etc.");
        }

        const auto q_node    = item["question"];
        const auto c_node    = item["choices"];
        const auto a_node    = item["answer"];
        const auto code_node = item["code"];
        const auto e_node    = item["explain"];

        if (!q_node || !c_node || !a_node)
        {
            throw std::runtime_error(
                "Missing required keys (question, choices, answer)."
                " Check each question in your .yaml file.");
        }
        if (!c_node.IsSequence())
        {
            throw std::runtime_error(
                "'choices' must be a sequence. Ensure 'choices' is a list"
                " with a dash (-) for each choice.");
        }

        Question q;
        q.question = q_node.as<std::string>();
        q.choices.reserve(c_node.size());
        for (const auto& c : c_node)
        {
            q.choices.push_back(c.as<std::string>());
        }
        q.answer = a_node.as<int>();
        if (code_node) q.code = code_node.as<std::string>();
        if (e_node)    q.explain = e_node.as<std::string>();

        if (auto err = validate_question(q))
        {
            throw std::runtime_error("Invalid question: " + *err);
        }
        questions.push_back(std::move(q));
    }

    return questions;
}

static void save_questions(const std::vector<Question>& questions, const std::string& filename)
{
    YAML::Emitter out;
    out << YAML::BeginSeq;
    for (const auto& q : questions)
    {
        out << YAML::BeginMap;
        out << YAML::Key << "question" << YAML::Value << q.question;
        if (q.code && !q.code->empty())
        {
            out << YAML::Key << "code" << YAML::Value << YAML::Literal << *q.code;
        }
        if (q.explain && !q.explain->empty())
        {
            out << YAML::Key << "explain" << YAML::Value << YAML::Literal << *q.explain;
        }
        out << YAML::Key << "choices" << YAML::Value << YAML::BeginSeq;
        for (const auto& c : q.choices)
        {
            out << c;
        }
        out << YAML::EndSeq;
        out << YAML::Key << "answer" << YAML::Value << q.answer;
        out << YAML::EndMap;
    }
    out << YAML::EndSeq;

    std::ofstream file_out(filename);
    if (!file_out)
    {
        throw std::runtime_error(
            "Cannot open file for writing. Check permissions and disk space: " + filename);
    }
    file_out << out.c_str();
}

static void print_question(const Question& q, std::size_t idx, std::size_t total)
{
    std::cout << "Question " << (idx + 1) << " of " << total << ":\n";
    std::cout << q.question << "\n";
    if (q.code && !q.code->empty())
    {
        std::cout << "\n%%% Code %%%\n";
        std::cout << *q.code << "\n";
        std::cout << "%%%%%%\n";
    }
    for (std::size_t i = 0; i < q.choices.size(); ++i)
    {
        std::cout << "  " << (i + 1) << ". " << q.choices[i] << "\n";
    }
}

static Question shuffle_question_choices(const Question& q, std::mt19937& rng)
{
    std::vector<std::pair<std::string, bool>> entries;
    entries.reserve(q.choices.size());
    for (std::size_t i = 0; i < q.choices.size(); ++i)
    {
        entries.emplace_back(q.choices[i], static_cast<int>(i) == q.answer);
    }
    std::shuffle(entries.begin(), entries.end(), rng);

    Question shuffled;
    shuffled.question = q.question;
    shuffled.code     = q.code;
    shuffled.explain  = q.explain;
    shuffled.choices.reserve(entries.size());
    shuffled.answer = 0;

    for (std::size_t i = 0; i < entries.size(); ++i)
    {
        shuffled.choices.push_back(std::move(entries[i].first));
        if (entries[i].second) shuffled.answer = static_cast<int>(i);
    }
    return shuffled;
}

static std::vector<Question> randomize_quiz(const std::vector<Question>& questions, std::mt19937& rng)
{
    std::vector<Question> shuffled;
    shuffled.reserve(questions.size());
    for (const auto& q : questions)
    {
        shuffled.push_back(shuffle_question_choices(q, rng));
    }
    std::shuffle(shuffled.begin(), shuffled.end(), rng);
    return shuffled;
}

static void take_quiz(const std::vector<Question>& questions, bool randomise)
{
    if (questions.empty())
    {
        std::cout << "No questions available. Add some first.\n";
        return;
    }

    std::random_device rd;
    std::mt19937 rng(rd());
    const std::vector<Question> session_qs = randomise ? randomize_quiz(questions, rng) : questions;

    int correct = 0;
    const auto total = session_qs.size();
    std::cout << "\nStarting Quiz " << (randomise ? "(randomised)" : "") << "\n";

    for (std::size_t i = 0; i < total; ++i)
    {
        print_question(session_qs[i], i, total);
        int ans = read_int_in_range(1, static_cast<int>(session_qs[i].choices.size()), "Your answer: ");

        if ((ans - 1) == session_qs[i].answer)
        {
            std::cout << "Correct!\n";
            ++correct;
        }
        else
        {
            std::cout << "Incorrect. The correct answer is " << (session_qs[i].answer + 1)
                      << ". " << session_qs[i].choices[session_qs[i].answer] << "\n";
        }

        if (session_qs[i].explain && !session_qs[i].explain->empty())
        {
            std::cout << "Explanation:\n" << *session_qs[i].explain << "\n";
        }

        double pct = 100.0 * static_cast<double>(correct) / static_cast<double>(total);
        std::cout << "Score: " << correct << "/" << total
                  << " (" << std::fixed << std::setprecision(1) << pct << "%)\n\n";
    }

    double final_pct = 100.0 * static_cast<double>(correct) / static_cast<double>(total);
    std::cout << "Quiz Complete.\n";
    std::cout << "Final Score: " << correct << "/" << total
              << " (" << std::fixed << std::setprecision(1) << final_pct << "%)\n";
}

static void add_question(std::vector<Question>& questions)
{
    std::string question_text = read_line("\nEnter the question: ");

    std::optional<std::string> code;
    if (read_yes_no("Include a code snippet?"))
    {
        code = read_multiline_until("Paste your code below", "END");
    }

    std::optional<std::string> explain;
    if (read_yes_no("Include an explanation for the correct answer?"))
    {
        explain = read_multiline_until("Enter the explanation", "END");
    }

    int num_choices = read_int_in_range(2, 10, "How many choices (2-10): ");
    std::vector<std::string> choices;
    choices.reserve(static_cast<std::size_t>(num_choices));
    for (int i = 0; i < num_choices; ++i)
    {
        choices.push_back(read_line("Choice " + std::to_string(i + 1) + ": "));
    }

    int correct_idx = read_int_in_range(
        1, num_choices, "Which choice is correct (1-" + std::to_string(num_choices) + "): ");

    Question q;
    q.question = std::move(question_text);
    q.choices  = std::move(choices);
    q.answer   = correct_idx - 1;
    q.code     = std::move(code);
    q.explain  = std::move(explain);

    if (auto err = validate_question(q))
    {
        std::cout << "Error: " << *err << "\n";
        return;
    }

    questions.push_back(std::move(q));
    std::cout << "Question added.\n";
}

static void remove_question(std::vector<Question>& questions)
{
    if (questions.empty())
    {
        std::cout << "No questions to remove.\n";
        return;
    }

    std::cout << "\nRemove Question\n";
    for (std::size_t i = 0; i < questions.size(); ++i)
    {
        std::cout << (i + 1) << ". " << questions[i].question
                  << (questions[i].code ? " [code]" : "")
                  << (questions[i].explain ? " [explain]" : "") << "\n";
    }

    int idx = read_int_in_range(
        1, static_cast<int>(questions.size()),
        "Remove which (1-" + std::to_string(questions.size()) + "): ");

    questions.erase(questions.begin() + (idx - 1));
    std::cout << "Question removed.\n";
}

static void change_answer(std::vector<Question>& questions)
{
    if (questions.empty())
    {
        std::cout << "No questions available.\n";
        return;
    }

    std::cout << "\nChange Answer\n";
    for (std::size_t i = 0; i < questions.size(); ++i)
    {
        std::cout << (i + 1) << ". " << questions[i].question << "\n";
    }

    int idx = read_int_in_range(
        1, static_cast<int>(questions.size()),
        "Modify which (1-" + std::to_string(questions.size()) + "): ");

    Question& chosen = questions[static_cast<std::size_t>(idx - 1)];
    print_question(chosen, static_cast<std::size_t>(idx - 1), questions.size());
    std::cout << "Current correct answer: " << (chosen.answer + 1)
              << ". " << chosen.choices[chosen.answer] << "\n";

    int new_ans = read_int_in_range(
        1, static_cast<int>(chosen.choices.size()), "New correct answer: ");

    chosen.answer = new_ans - 1;
    std::cout << "Answer updated.\n";
}

static void list_questions(const std::vector<Question>& questions)
{
    if (questions.empty())
    {
        std::cout << "No questions available.\n";
        return;
    }

    bool show_answers = read_yes_no("Show correct answers?");
    bool show_code    = read_yes_no("Show code snippets?");
    bool show_explain = read_yes_no("Show explanations?");

    std::cout << "\nAll Questions:\n";
    for (std::size_t i = 0; i < questions.size(); ++i)
    {
        const Question& q = questions[i];

        std::cout << "\n" << (i + 1) << ". " << q.question
                  << (q.code ? " [code]" : "")
                  << (q.explain ? " [explain]" : "") << "\n";

        if (show_code && q.code && !q.code->empty())
        {
            std::cout << "\n%%% Code %%%\n";
            std::cout << *q.code << "\n";
            std::cout << "%%%%%%%%%%\n";
        }

        for (std::size_t j = 0; j < q.choices.size(); ++j)
        {
            std::cout << "  " << (j + 1) << ". " << q.choices[j];
            if (show_answers && static_cast<int>(j) == q.answer)
            {
                std::cout << "  [x] correct";
            }
            std::cout << "\n";
        }

        if (show_explain && q.explain && !q.explain->empty())
        {
            std::cout << "Explanation:\n" << *q.explain << "\n";
        }
    }
}

static void show_unsaved_changes(
    const std::vector<Question>& original,
    const std::vector<Question>& current)
{
    if (original == current)
    {
        std::cout << "\nNo unsaved changes.\n";
        return;
    }

    std::cout << "\nUnsaved changes:\n";

    for (std::size_t i = 0; i < original.size(); ++i)
    {
        bool found = false;
        for (std::size_t j = 0; j < current.size(); ++j)
        {
            if (current[j].question == original[i].question)
            {
                found = true;
                break;
            }
        }
        if (!found)
        {
            std::cout << "  [-] Removed: " << original[i].question << "\n";
        }
    }

    for (std::size_t i = 0; i < current.size(); ++i)
    {
        bool found = false;
        for (std::size_t j = 0; j < original.size(); ++j)
        {
            if (original[j].question == current[i].question)
            {
                found = true;
                break;
            }
        }
        if (!found)
        {
            std::cout << "  [+] Added: " << current[i].question << "\n";
        }
    }

    for (std::size_t i = 0; i < current.size(); ++i)
    {
        for (std::size_t j = 0; j < original.size(); ++j)
        {
            if (current[i].question != original[j].question) continue;

            if (current[i].answer != original[j].answer)
            {
                std::cout << "  [~] Answer changed: " << current[i].question << "\n";
                std::cout << "       was: " << (original[j].answer + 1)
                          << ". " << original[j].choices[original[j].answer] << "\n";
                std::cout << "       now: " << (current[i].answer + 1)
                          << ". " << current[i].choices[current[i].answer] << "\n";
            }
            if (current[i].choices != original[j].choices)
            {
                std::cout << "  [~] Choices changed: " << current[i].question << "\n";
            }
            if (current[i].code != original[j].code)
            {
                std::cout << "  [~] Code changed: " << current[i].question << "\n";
            }
            if (current[i].explain != original[j].explain)
            {
                std::cout << "  [~] Explanation changed: " << current[i].question << "\n";
            }
            break;
        }
    }
}

static int menu_choice(bool randomise)
{
    std::cout << "\nQuizzer (Randomise: " << (randomise ? "ON" : "OFF") << ")\n"
                 "1. Take Quiz\n"
                 "2. Add Question\n"
                 "3. Remove Question\n"
                 "4. Change Answer\n"
                 "5. List Questions\n"
                 "6. Save and Exit\n"
                 "7. Quit without Saving\n"
                 "8. Toggle Randomise\n";
    return read_int_in_range(1, 8, "Choose (1-8): ");
}

int main(int argc, char* argv[])
{
    try
    {
        const std::string filename = (argc > 1) ? std::string(argv[1]) : std::string("quiz.yaml");

        std::vector<Question> questions;
        try
        {
            questions = load_questions(filename);
            std::cout << "Loaded " << questions.size() << " questions from " << filename << ".\n";
        }
        catch (const std::exception& e)
        {
            std::cout << "Warning: " << e.what() << "\nStarting with an empty quiz.\n";
        }

        const std::vector<Question> saved_questions = questions;
        bool randomise = false;

        while (true)
        {
            int choice = menu_choice(randomise);
            switch (choice)
            {
                case 1:
                    take_quiz(questions, randomise);
                    break;
                case 2:
                    add_question(questions);
                    break;
                case 3:
                    remove_question(questions);
                    break;
                case 4:
                    change_answer(questions);
                    break;
                case 5:
                    list_questions(questions);
                    break;
                case 6:
                    try
                    {
                        save_questions(questions, filename);
                        std::cout << "Saved to " << filename << ". =)\n";
                    }
                    catch (const std::exception& e)
                    {
                        std::cerr << "Save error: " << e.what() << "\n";
                        std::cerr << "Changes are UN-saved! =(\n";
                        return 1;
                    }
                    return 0;
                case 7:
                    show_unsaved_changes(saved_questions, questions);
                    if (read_yes_no("Discard these changes and exit?"))
                    {
                        std::cout << "Exiting without saving.\n";
                        return 0;
                    }
                    break;
                case 8:
                    randomise = !randomise;
                    std::cout << "Randomise is now " << (randomise ? "ON" : "OFF") << ".\n";
                    break;
            }
        }
    }
    catch (const std::bad_alloc&)
    {
        std::cerr << "Fatal: Out of memory.\n";
        return 1;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Fatal error: " << e.what()
                  << ". Please report this as a github issue. (see README.md)\n";
        return 1;
    }
    catch (...)
    {
        std::cerr << "Fatal: Unknown error. Please report this as a github issue (see README.md)\n";
        return 1;
    }
}
