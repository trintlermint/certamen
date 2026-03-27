#include "syntax.hpp"
#include <algorithm>
#include <sstream>
#include <unordered_set>
#include <cctype>

using namespace ftxui;

// keyword tables per language family

static const std::unordered_set<std::string> haskell_kw = {
    "module","where","let","in","if","then","else","case","of","do",
    "type","data","newtype","class","instance","deriving","import",
    "qualified","as","hiding","infixl","infixr","infix","forall",
    "default","foreign","mdo","rec","proc",
};

static const std::unordered_set<std::string> c_family_kw = {
    "auto","break","case","char","const","continue","default","do",
    "double","else","enum","extern","float","for","goto","if","int",
    "long","register","return","short","signed","sizeof","static",
    "struct","switch","typedef","union","unsigned","void","volatile",
    "while","inline","restrict","class","namespace","template",
    "typename","virtual","override","public","private","protected",
    "new","delete","this","throw","try","catch","using","nullptr",
    "bool","true","false","constexpr","noexcept","final","abstract",
    "#include","#define","#ifdef","#ifndef","#endif","#pragma",
    "extends","implements","interface","package","import","super",
    "synchronized","throws","transient","native","assert","final",
    "instanceof","strictfp",
};

static const std::unordered_set<std::string> python_kw = {
    "and","as","assert","async","await","break","class","continue",
    "def","del","elif","else","except","finally","for","from",
    "global","if","import","in","is","lambda","nonlocal","not",
    "or","pass","raise","return","try","while","with","yield",
    "True","False","None","self",
};

static const std::unordered_set<std::string> rust_kw = {
    "as","async","await","break","const","continue","crate","dyn",
    "else","enum","extern","false","fn","for","if","impl","in",
    "let","loop","match","mod","move","mut","pub","ref","return",
    "self","Self","static","struct","super","trait","true","type",
    "unsafe","use","where","while",
};

enum class Lang { Haskell, CFam, Python, Rust, Unknown };

static Lang detect_lang(const std::optional<std::string>& language)
{
    if (!language || language->empty()) return Lang::Unknown;
    std::string low = *language;
    std::transform(low.begin(), low.end(), low.begin(), ::tolower);

    if (low == "haskell" || low == "hs") return Lang::Haskell;
    if (low == "c" || low == "cpp" || low == "c++" || low == "java" ||
        low == "csharp" || low == "c#" || low == "go" || low == "javascript" ||
        low == "js" || low == "typescript" || low == "ts")
        return Lang::CFam;
    if (low == "python" || low == "py") return Lang::Python;
    if (low == "rust" || low == "rs") return Lang::Rust;
    return Lang::Unknown;
}

static const std::unordered_set<std::string>& keywords_for(Lang lang)
{
    static const std::unordered_set<std::string> empty;
    switch (lang)
    {
        case Lang::Haskell: return haskell_kw;
        case Lang::CFam:    return c_family_kw;
        case Lang::Python:  return python_kw;
        case Lang::Rust:    return rust_kw;
        default:            return empty;
    }
}

static std::string line_comment_prefix(Lang lang)
{
    switch (lang)
    {
        case Lang::Haskell: return "--";
        case Lang::Python:  return "#";
        case Lang::CFam:
        case Lang::Rust:    return "//";
        default:            return "";
    }
}

static Element highlight_line(const std::string& line, Lang lang,
                              const std::unordered_set<std::string>& kw)
{
    Elements parts;
    std::string cmt_prefix = line_comment_prefix(lang);
    std::size_t i = 0;
    std::size_t len = line.size();

    auto flush_text = [&](const std::string& s, Color c) {
        if (!s.empty()) parts.push_back(text(s) | color(c));
    };

    while (i < len)
    {
        // line comment
        if (!cmt_prefix.empty() &&
            i + cmt_prefix.size() <= len &&
            line.compare(i, cmt_prefix.size(), cmt_prefix) == 0)
        {
            flush_text(line.substr(i), Color::GrayDark);
            i = len;
            break;
        }

        // block comment start (Haskell {-, C-family /*)
        if (lang == Lang::Haskell && i + 1 < len && line[i] == '{' && line[i+1] == '-')
        {
            auto end = line.find("-}", i + 2);
            if (end != std::string::npos)
            {
                flush_text(line.substr(i, end + 2 - i), Color::GrayDark);
                i = end + 2;
            }
            else
            {
                flush_text(line.substr(i), Color::GrayDark);
                i = len;
            }
            continue;
        }
        if ((lang == Lang::CFam || lang == Lang::Rust) &&
            i + 1 < len && line[i] == '/' && line[i+1] == '*')
        {
            auto end = line.find("*/", i + 2);
            if (end != std::string::npos)
            {
                flush_text(line.substr(i, end + 2 - i), Color::GrayDark);
                i = end + 2;
            }
            else
            {
                flush_text(line.substr(i), Color::GrayDark);
                i = len;
            }
            continue;
        }

        // strings
        if (line[i] == '"' || line[i] == '\'')
        {
            char quote = line[i];
            std::string s(1, quote);
            ++i;
            while (i < len && line[i] != quote)
            {
                if (line[i] == '\\' && i + 1 < len)
                {
                    s += line[i];
                    ++i;
                }
                s += line[i];
                ++i;
            }
            if (i < len) { s += line[i]; ++i; }
            flush_text(s, Color::Yellow);
            continue;
        }

        // numbers
        if (std::isdigit(static_cast<unsigned char>(line[i])))
        {
            std::string num;
            while (i < len && (std::isalnum(static_cast<unsigned char>(line[i])) || line[i] == '.'))
            {
                num += line[i];
                ++i;
            }
            flush_text(num, Color::Magenta);
            continue;
        }

        // identifiers and keywords
        if (std::isalpha(static_cast<unsigned char>(line[i])) || line[i] == '_' || line[i] == '#')
        {
            std::string word;
            if (line[i] == '#')
            {
                word += '#';
                ++i;
            }
            while (i < len && (std::isalnum(static_cast<unsigned char>(line[i])) ||
                               line[i] == '_' || line[i] == '\''))
            {
                word += line[i];
                ++i;
            }

            if (kw.count(word))
            {
                flush_text(word, Color::Cyan);
            }
            else if (lang == Lang::Haskell && !word.empty() &&
                     std::isupper(static_cast<unsigned char>(word[0])))
            {
                flush_text(word, Color::Green);
            }
            else
            {
                flush_text(word, Color::Default);
            }
            continue;
        }

        // operators and punctuation
        {
            std::string op(1, line[i]);
            ++i;
            if (lang == Lang::Haskell)
            {
                // Haskell ops can be multi-char
                while (i < len && !std::isalnum(static_cast<unsigned char>(line[i])) &&
                       !std::isspace(static_cast<unsigned char>(line[i])) &&
                       line[i] != '"' && line[i] != '\'' &&
                       line[i] != '(' && line[i] != ')' &&
                       line[i] != '{' && line[i] != '}' &&
                       line[i] != '[' && line[i] != ']')
                {
                    op += line[i];
                    ++i;
                }
            }
            if (op == "::" || op == "->" || op == "=>" || op == "<-" ||
                op == "=" || op == "|")
            {
                flush_text(op, Color::Red);
            }
            else
            {
                flush_text(op, Color::Default);
            }
        }
    }

    if (parts.empty()) return text("");
    return hbox(std::move(parts));
}

ftxui::Element render_code_block(
    const std::string& code,
    const std::optional<std::string>& language)
{
    Lang lang = detect_lang(language);
    const auto& kw = keywords_for(lang);

    Elements lines;
    std::istringstream stream(code);
    std::string line;
    while (std::getline(stream, line))
    {
        lines.push_back(hbox({
            text("  "),
            highlight_line(line, lang, kw),
        }));
    }

    std::string label = " Code";
    if (language && !language->empty())
        label += " [" + *language + "]";

    return vbox({
        text(label) | bold | color(Color::Cyan),
        vbox(std::move(lines)),
    }) | borderRounded | color(Color::GrayLight);
}
