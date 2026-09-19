#include "model.hpp"
#include <fstream>
#include <stdexcept>
#include <yaml-cpp/yaml.h>

std::optional<std::string> validate_question(const Question& q)
{
    if (q.question.empty())
        return std::string("Question cannot be blank.");
    if (q.choices.size() < 2)
        return std::string("Question must have more than one (1) choice.");
    if (q.answer < 0 || q.answer >= static_cast<int>(q.choices.size()))
        return std::string("Answer provided is out of range of the number of choices provided.");
    return std::nullopt;
}

static std::vector<Question> parse_question_sequence(const std::optional<int>& default_answer, const YAML::Node& seq)
{
    std::vector<Question> questions;
    questions.reserve(seq.size());

    for (const auto& item : seq)
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
        const auto lang_node = item["language"];

        if (!q_node || !c_node || (!a_node && !default_answer))
        {
            throw std::runtime_error(
                "Missing required keys (question, choices, answer)."
                " Check each question in your .yaml file or provide 'default-answer'.");
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
        q.answer = a_node ? a_node.as<int>() : *default_answer;
        if (code_node) q.code = code_node.as<std::string>();
        if (e_node)    q.explain = e_node.as<std::string>();
        if (lang_node) q.language = lang_node.as<std::string>();

        if (auto err = validate_question(q))
        {
            throw std::runtime_error("Invalid question: " + *err);
        }
        questions.push_back(std::move(q));
    }

    return questions;
}

QuizFile load_quiz(const std::string& filename)
{
    YAML::Node root = YAML::LoadFile(filename);
    QuizFile quiz;

    if (!root.IsMap())
    {
        throw std::runtime_error(
            "YAML must be a map with 'questions' (and optionally 'name', 'author')."
            " See example_quiz.yaml.");
    }

    if (root["name"])   quiz.name   = root["name"].as<std::string>();
    if (root["author"]) quiz.author = root["author"].as<std::string>();
    if (root["default-answer"])
    {
        quiz.default_answer = root["default-answer"].as<int>();
    }

    auto q_node = root["questions"];
    if (!q_node)
        throw std::runtime_error(
            "YAML map must contain a 'questions' key with a sequence of questions.");
    if (!q_node.IsSequence())
        throw std::runtime_error("'questions' must be a sequence.");

    quiz.questions = parse_question_sequence(quiz.default_answer, q_node);
    return quiz;
}

static void emit_questions(YAML::Emitter& out, const std::optional<int>& default_answer, const std::vector<Question>& questions)
{
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
        if (q.language && !q.language->empty())
        {
            out << YAML::Key << "language" << YAML::Value << *q.language;
        }
        out << YAML::Key << "choices" << YAML::Value << YAML::BeginSeq;
        for (const auto& c : q.choices)
        {
            out << c;
        }
        out << YAML::EndSeq;
        if (!default_answer || *default_answer != q.answer)
        {
            out << YAML::Key << "answer" << YAML::Value << q.answer;
        }
        out << YAML::EndMap;
    }
    out << YAML::EndSeq;
}

void save_quiz(const QuizFile& quiz, const std::string& filename)
{
    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "name"   << YAML::Value << quiz.name;
    out << YAML::Key << "author" << YAML::Value << quiz.author;
    if (quiz.default_answer)
    {
        out << YAML::Key << "default-answer" << YAML::Value << *quiz.default_answer;
    }
    out << YAML::Key << "questions" << YAML::Value;
    emit_questions(out, quiz.default_answer, quiz.questions);
    out << YAML::EndMap;

    std::ofstream file_out(filename);
    if (!file_out)
    {
        throw std::runtime_error(
            "Cannot open file for writing. Check permissions and disk space: " + filename);
    }
    file_out << out.c_str();
}

