#pragma once

#include <iomanip>
#include <string>
#include <vector>

#include "colors.hpp"
#include "result.hpp"
#include "string.hpp"

/*
    Parser
 */

enum TokenType
{
    TOKEN_INVALID,
    TOKEN_IDENTIFIER,
    TOKEN_STRING,
    TOKEN_NUMBER,
    TOKEN_LEFT_CURLY,
    TOKEN_RIGHT_CURLY,
    TOKEN_LINE_BREAK,
    TOKEN_EOF
};

class Token
{
public:
    Token();

    static Token ident(std::string str, int line, int column);
    static Token num(int num, int line, int column);
    static Token str(std::string str, int line, int column);
    static Token left_curly(int line, int column);
    static Token right_curly(int line, int column);
    static Token ln(int line, int column);
    static Token invalid();

    TokenType type() const;
    std::string str() const;
    int number() const;
    int line() const;
    int column() const;

    int width();
    std::string content();

private:
    TokenType m_type;
    std::string m_str;
    int m_int;

    int m_line;
    int m_column;
};

std::ostream& operator<<(std::ostream& os, Token const& tok);

class ConfigEntry
{
public:
    ConfigEntry();
    ~ConfigEntry();

    bool is_inline();
    void set_inline(bool b);

    std::vector<Token>& args()
    {
        return m_arguments;
    }
    void add_arg(Token tok);

    std::vector<ConfigEntry>& children()
    {
        return m_children;
    }
    void add_child(ConfigEntry entry);

    std::string& source()
    {
        return m_source;
    }
    void set_source(std::string source)
    {
        m_source = source;
    }

    Token& left_curly()
    {
        return m_left_curly;
    }
    Token& right_curly()
    {
        return m_right_curly;
    }
    void set_curly(Token left, Token right)
    {
        m_left_curly = left;
        m_right_curly = right;
    }

private:
    std::vector<Token> m_arguments;
    std::vector<ConfigEntry> m_children;
    bool m_inline;
    std::string m_source; // Maybe use a pointer ?

    Token m_left_curly;
    Token m_right_curly;
};

class Arg
{
public:
    Arg(std::string name, TokenType type, void *ptr, bool optional = false)
        : m_ptr(ptr), m_type(type), m_name(name), m_optional(optional)
    {
    }

    template <typename T>
    T *ptr()
    {
        return (T *)m_ptr;
    }

    TokenType type()
    {
        return m_type;
    }

    std::string& name()
    {
        return m_name;
    }

    bool optional()
    {
        return m_optional;
    }

private:
    void *m_ptr;
    TokenType m_type;
    std::string m_name;
    bool m_optional;
};

class Usage
{
public:
    Usage()
    {
    }

    Usage(std::string name, std::vector<Arg> args) : m_name(name), m_args(args)
    {
    }

    std::string& name()
    {
        return m_name;
    }

    std::vector<Arg>& args()
    {
        return m_args;
    }

private:
    std::string m_name;
    std::vector<Arg> m_args;
};

class ConfigError
{
public:
    enum Type
    {
        FILE_NOT_FOUND,
        UNEXPECTED_TOKEN,
        NOT_INLINE,
        MISMATCH_CURLY,
        MISMATCH_ENTRY,
        UNKNOWN_ENTRY,
        ADDR,
        INVALID_METHOD
    };

    static ConfigError not_found(std::string filename);
    static ConfigError unexpected(std::string source, Token got, TokenType expected);
    static ConfigError not_inline(std::string source, Token name);
    static ConfigError mismatch_curly(std::string source, Token curly);
    static ConfigError mismatch_entry(std::string source, Token tok, std::string expected, std::vector<Arg> args);
    static ConfigError unknown_entry(std::string source, Token tok, std::vector<std::string> entries);
    static ConfigError address(std::string source, Token addr);
    static ConfigError invalid_method(std::string source, Token tok);

    ConfigError();

    template <typename S>
    void print(S& os)
    {
        if (m_type == FILE_NOT_FOUND)
        {
            os << "File `" << m_notfound.filename << "` not found\n";
            return;
        }

        _print_lines(os, m_token.line() - 2, 3);

        os << "     ";
        os << NRED << _spaces(m_token.column()) << "^";

        for (int i = 0; i < m_token.width() - 1; i++)
            os << "~";

        os << RESET << "\n";
        os << "     " << _spaces(m_token.column()) << "|\n";
        os << "     " << _spaces(m_token.column()) << "+----- " << _strerror() << "\n";
    }

private:
    std::string m_source;
    Token m_token;
    Type m_type;

    struct
    {
        std::string filename;
    } m_notfound;

    struct UnexpectedToken
    {
        TokenType type;
    } m_unexpected;

    struct MismatchEntry
    {
        Usage usage;
    } m_mismatch_entry;

    struct UnknownEntry
    {
        std::vector<std::string> entries;
    } m_unknown;

    ConfigError(Type type, Token token, std::string source);

    template <typename S>
    void _print_lines(S& os, int first_line, size_t num)
    {
        std::vector<std::string> lines = split(m_source, "\n");

        if (first_line < 0)
        {
            num += first_line;
            first_line = 0;
        }

        for (size_t i = first_line; i < first_line + num && i < lines.size(); i++)
            os << std::setw(2) << (i + 1) << " | " << lines[i] << "\n";
    }

    std::string _strerror();
    std::string _strtok(TokenType type);

    std::string _spaces(size_t n);
};

class ConfigParser
{
public:
    ConfigParser();
    ~ConfigParser();

    Result<int, ConfigError> parse(std::string filename);
    ConfigEntry& root();

private:
    ConfigEntry m_root;

    std::vector<Token> _read_tokens(std::string source);
};

/*
    Deserialisation
 */

class Deser
{
public:
    virtual Result<int, ConfigError> deserialize(ConfigEntry& from) = 0;

    Result<int, ConfigError> expect_ident(std::string& content);
    Result<int, ConfigError> expect_str(std::string& content);
};
