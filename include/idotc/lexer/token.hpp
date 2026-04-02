#ifndef IDOTC_LEXER_TOKEN_HPP
#define IDOTC_LEXER_TOKEN_HPP

#include <cstdint>
#include <string>
#include <string_view>
#include <variant>
#include <optional>

namespace idotc {

/// Source location tracking for error messages
struct SourceSpan {
    std::size_t start;   // Byte offset from start of source
    std::size_t end;     // Byte offset (exclusive)
    std::size_t line;    // 1-indexed line number
    std::size_t column;  // 1-indexed column number
    
    [[nodiscard]] constexpr std::size_t length() const noexcept {
        return end - start;
    }
};

/// All token kinds in the Advanced-idot language
enum class TokenKind : std::uint8_t {
    // End of file
    Eof,
    
    // Keywords
    Use,        // use
    Fn,         // fn
    Let,        // let
    Mut,        // mut
    Struct,     // struct
    Class,      // class
    Trait,      // trait
    Impl,       // impl
    If,         // if
    Else,       // else
    While,      // while
    For,        // for
    In,         // in
    Match,      // match
    Return,     // return
    Break,      // break
    Continue,   // continue
    As,         // as
    True,       // true
    False,      // false
    Legacy,     // legacy
    Py,         // py
    Js,         // js
    Self,       // self
    Pub,        // pub
    
    // Literals
    IntLiteral,     // 42, 0xFF, 0b1010
    FloatLiteral,   // 3.14, 1e-5
    StringLiteral,  // "hello"
    CharLiteral,    // 'a'
    
    // Identifier
    Identifier,     // foo, bar_123
    
    // Arithmetic operators
    Plus,           // +
    Minus,          // -
    Star,           // *
    Slash,          // /
    Percent,        // %
    
    // Comparison operators
    EqEq,           // ==
    NotEq,          // !=
    Lt,             // <
    Gt,             // >
    LtEq,           // <=
    GtEq,           // >=
    
    // Logical operators
    AndAnd,         // &&
    OrOr,           // ||
    Bang,           // !
    
    // Bitwise operators
    And,            // &
    Or,             // |
    Xor,            // ^
    Shl,            // <<
    Shr,            // >>
    Tilde,          // ~
    
    // Assignment operators
    Eq,             // =
    PlusEq,         // +=
    MinusEq,        // -=
    StarEq,         // *=
    SlashEq,        // /=
    PercentEq,      // %=
    AndEq,          // &=
    OrEq,           // |=
    XorEq,          // ^=
    ShlEq,          // <<=
    ShrEq,          // >>=
    
    // Delimiters
    LParen,         // (
    RParen,         // )
    LBrace,         // {
    RBrace,         // }
    LBracket,       // [
    RBracket,       // ]
    
    // Punctuation
    Comma,          // ,
    Colon,          // :
    Semicolon,      // ;
    Dot,            // .
    DotDot,         // ..
    Arrow,          // ->
    FatArrow,       // =>
    DoubleColon,    // ::
    Question,       // ?
    At,             // @
    Hash,           // #
    
    // Special
    Error,          // Lexer error token
};

/// Get a string representation of a TokenKind
[[nodiscard]] constexpr std::string_view token_kind_name(TokenKind kind) noexcept {
    switch (kind) {
        case TokenKind::Eof:          return "EOF";
        case TokenKind::Use:          return "use";
        case TokenKind::Fn:           return "fn";
        case TokenKind::Let:          return "let";
        case TokenKind::Mut:          return "mut";
        case TokenKind::Struct:       return "struct";
        case TokenKind::Class:        return "class";
        case TokenKind::Trait:        return "trait";
        case TokenKind::Impl:         return "impl";
        case TokenKind::If:           return "if";
        case TokenKind::Else:         return "else";
        case TokenKind::While:        return "while";
        case TokenKind::For:          return "for";
        case TokenKind::In:           return "in";
        case TokenKind::Match:        return "match";
        case TokenKind::Return:       return "return";
        case TokenKind::Break:        return "break";
        case TokenKind::Continue:     return "continue";
        case TokenKind::As:           return "as";
        case TokenKind::True:         return "true";
        case TokenKind::False:        return "false";
        case TokenKind::Legacy:       return "legacy";
        case TokenKind::Py:           return "py";
        case TokenKind::Js:           return "js";
        case TokenKind::Self:         return "self";
        case TokenKind::Pub:          return "pub";
        case TokenKind::IntLiteral:   return "integer";
        case TokenKind::FloatLiteral: return "float";
        case TokenKind::StringLiteral:return "string";
        case TokenKind::CharLiteral:  return "char";
        case TokenKind::Identifier:   return "identifier";
        case TokenKind::Plus:         return "+";
        case TokenKind::Minus:        return "-";
        case TokenKind::Star:         return "*";
        case TokenKind::Slash:        return "/";
        case TokenKind::Percent:      return "%";
        case TokenKind::EqEq:         return "==";
        case TokenKind::NotEq:        return "!=";
        case TokenKind::Lt:           return "<";
        case TokenKind::Gt:           return ">";
        case TokenKind::LtEq:         return "<=";
        case TokenKind::GtEq:         return ">=";
        case TokenKind::AndAnd:       return "&&";
        case TokenKind::OrOr:         return "||";
        case TokenKind::Bang:         return "!";
        case TokenKind::And:          return "&";
        case TokenKind::Or:           return "|";
        case TokenKind::Xor:          return "^";
        case TokenKind::Shl:          return "<<";
        case TokenKind::Shr:          return ">>";
        case TokenKind::Tilde:        return "~";
        case TokenKind::Eq:           return "=";
        case TokenKind::PlusEq:       return "+=";
        case TokenKind::MinusEq:      return "-=";
        case TokenKind::StarEq:       return "*=";
        case TokenKind::SlashEq:      return "/=";
        case TokenKind::PercentEq:    return "%=";
        case TokenKind::AndEq:        return "&=";
        case TokenKind::OrEq:         return "|=";
        case TokenKind::XorEq:        return "^=";
        case TokenKind::ShlEq:        return "<<=";
        case TokenKind::ShrEq:        return ">>=";
        case TokenKind::LParen:       return "(";
        case TokenKind::RParen:       return ")";
        case TokenKind::LBrace:       return "{";
        case TokenKind::RBrace:       return "}";
        case TokenKind::LBracket:     return "[";
        case TokenKind::RBracket:     return "]";
        case TokenKind::Comma:        return ",";
        case TokenKind::Colon:        return ":";
        case TokenKind::Semicolon:    return ";";
        case TokenKind::Dot:          return ".";
        case TokenKind::DotDot:       return "..";
        case TokenKind::Arrow:        return "->";
        case TokenKind::FatArrow:     return "=>";
        case TokenKind::DoubleColon:  return "::";
        case TokenKind::Question:     return "?";
        case TokenKind::At:           return "@";
        case TokenKind::Hash:         return "#";
        case TokenKind::Error:        return "error";
    }
    return "unknown";
}

/// Literal value types
using LiteralValue = std::variant<
    std::monostate,         // No value (for non-literal tokens)
    std::int64_t,           // Integer literal
    double,                 // Float literal
    std::string,            // String literal (with escapes processed)
    char                    // Char literal
>;

/// A token produced by the lexer
struct Token {
    TokenKind kind;
    SourceSpan span;
    std::string_view lexeme;    // Raw text from source (zero-copy)
    LiteralValue value;         // Parsed literal value if applicable
    
    /// Check if this is a keyword token
    [[nodiscard]] constexpr bool is_keyword() const noexcept {
        return kind >= TokenKind::Use && kind <= TokenKind::Pub;
    }
    
    /// Check if this is a literal token
    [[nodiscard]] constexpr bool is_literal() const noexcept {
        return kind >= TokenKind::IntLiteral && kind <= TokenKind::CharLiteral;
    }
    
    /// Check if this is an operator token
    [[nodiscard]] constexpr bool is_operator() const noexcept {
        return kind >= TokenKind::Plus && kind <= TokenKind::ShrEq;
    }
    
    /// Check if this is a delimiter
    [[nodiscard]] constexpr bool is_delimiter() const noexcept {
        return kind >= TokenKind::LParen && kind <= TokenKind::RBracket;
    }
    
    /// Check if this is the end of file
    [[nodiscard]] constexpr bool is_eof() const noexcept {
        return kind == TokenKind::Eof;
    }
    
    /// Check if this is an error token
    [[nodiscard]] constexpr bool is_error() const noexcept {
        return kind == TokenKind::Error;
    }
};

} // namespace idotc

#endif // IDOTC_LEXER_TOKEN_HPP
