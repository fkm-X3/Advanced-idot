#include "idotc/lexer/lexer.hpp"
#include <cctype>
#include <charconv>
#include <unordered_map>

namespace idotc {

namespace {
    bool is_digit(char c) { return c >= '0' && c <= '9'; }
    bool is_alpha(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'; }
    bool is_alnum(char c) { return is_alpha(c) || is_digit(c); }
    bool is_hex_digit(char c) { 
        return is_digit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'); 
    }
    bool is_binary_digit(char c) { return c == '0' || c == '1'; }
}

Lexer::Lexer(std::string_view source)
    : source_(source)
{}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    tokens.reserve(source_.size() / 4); // Rough estimate
    
    while (!is_eof()) {
        Token tok = next_token();
        tokens.push_back(std::move(tok));
        if (tok.kind == TokenKind::Eof) break;
    }
    
    return tokens;
}

Token Lexer::next_token() {
    skip_whitespace();
    return scan_token();
}

Token Lexer::peek() const {
    Lexer copy = *this;
    return copy.next_token();
}

bool Lexer::is_eof() const {
    return pos_ >= source_.size();
}

char Lexer::current() const {
    return is_eof() ? '\0' : source_[pos_];
}

char Lexer::peek_char() const {
    return peek_char(1);
}

char Lexer::peek_char(std::size_t offset) const {
    std::size_t idx = pos_ + offset;
    return idx < source_.size() ? source_[idx] : '\0';
}

void Lexer::advance() {
    if (!is_eof()) {
        if (source_[pos_] == '\n') {
            ++line_;
            column_ = 1;
            line_start_ = pos_ + 1;
        } else {
            ++column_;
        }
        ++pos_;
    }
}

void Lexer::advance(std::size_t count) {
    for (std::size_t i = 0; i < count; ++i) advance();
}

bool Lexer::match(char expected) {
    if (current() == expected) {
        advance();
        return true;
    }
    return false;
}

bool Lexer::match(std::string_view expected) {
    if (source_.substr(pos_).starts_with(expected)) {
        advance(expected.size());
        return true;
    }
    return false;
}

void Lexer::skip_whitespace() {
    while (!is_eof()) {
        char c = current();
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            advance();
        } else if (c == '/' && peek_char() == '/') {
            skip_line_comment();
        } else if (c == '/' && peek_char() == '*') {
            if (!skip_block_comment()) return;
        } else {
            break;
        }
    }
}

void Lexer::skip_line_comment() {
    advance(2); // Skip //
    while (!is_eof() && current() != '\n') advance();
}

bool Lexer::skip_block_comment() {
    std::size_t start = pos_;
    advance(2); // Skip /*
    int depth = 1;
    
    while (!is_eof() && depth > 0) {
        if (current() == '/' && peek_char() == '*') {
            advance(2);
            ++depth;
        } else if (current() == '*' && peek_char() == '/') {
            advance(2);
            --depth;
        } else {
            advance();
        }
    }
    
    if (depth > 0) {
        report_error("Unterminated block comment", start);
        return false;
    }
    return true;
}

Token Lexer::scan_token() {
    if (is_eof()) {
        return make_token(TokenKind::Eof, pos_);
    }
    
    std::size_t start = pos_;
    char c = current();
    advance();
    
    // Identifiers and keywords
    if (is_alpha(c)) {
        --pos_; --column_; // Back up
        return scan_identifier();
    }
    
    // Numbers
    if (is_digit(c)) {
        --pos_; --column_; // Back up
        return scan_number();
    }
    
    // Strings
    if (c == '"') {
        return scan_string();
    }
    
    // Characters
    if (c == '\'') {
        return scan_char();
    }
    
    // Operators and punctuation
    switch (c) {
        // Single character tokens
        case '(': return make_token(TokenKind::LParen, start);
        case ')': return make_token(TokenKind::RParen, start);
        case '{': return make_token(TokenKind::LBrace, start);
        case '}': return make_token(TokenKind::RBrace, start);
        case '[': return make_token(TokenKind::LBracket, start);
        case ']': return make_token(TokenKind::RBracket, start);
        case ',': return make_token(TokenKind::Comma, start);
        case ';': return make_token(TokenKind::Semicolon, start);
        case '?': return make_token(TokenKind::Question, start);
        case '@': return make_token(TokenKind::At, start);
        case '#': return make_token(TokenKind::Hash, start);
        case '~': return make_token(TokenKind::Tilde, start);
        
        // Potentially multi-character tokens
        case '+':
            if (match('=')) return make_token(TokenKind::PlusEq, start);
            return make_token(TokenKind::Plus, start);
        case '-':
            if (match('>')) return make_token(TokenKind::Arrow, start);
            if (match('=')) return make_token(TokenKind::MinusEq, start);
            return make_token(TokenKind::Minus, start);
        case '*':
            if (match('=')) return make_token(TokenKind::StarEq, start);
            return make_token(TokenKind::Star, start);
        case '/':
            if (match('=')) return make_token(TokenKind::SlashEq, start);
            return make_token(TokenKind::Slash, start);
        case '%':
            if (match('=')) return make_token(TokenKind::PercentEq, start);
            return make_token(TokenKind::Percent, start);
        case '=':
            if (match('>')) return make_token(TokenKind::FatArrow, start);
            if (match('=')) return make_token(TokenKind::EqEq, start);
            return make_token(TokenKind::Eq, start);
        case '!':
            if (match('=')) return make_token(TokenKind::NotEq, start);
            return make_token(TokenKind::Bang, start);
        case '<':
            if (match('<')) {
                if (match('=')) return make_token(TokenKind::ShlEq, start);
                return make_token(TokenKind::Shl, start);
            }
            if (match('=')) return make_token(TokenKind::LtEq, start);
            return make_token(TokenKind::Lt, start);
        case '>':
            if (match('>')) {
                if (match('=')) return make_token(TokenKind::ShrEq, start);
                return make_token(TokenKind::Shr, start);
            }
            if (match('=')) return make_token(TokenKind::GtEq, start);
            return make_token(TokenKind::Gt, start);
        case '&':
            if (match('&')) return make_token(TokenKind::AndAnd, start);
            if (match('=')) return make_token(TokenKind::AndEq, start);
            return make_token(TokenKind::And, start);
        case '|':
            if (match('|')) return make_token(TokenKind::OrOr, start);
            if (match('=')) return make_token(TokenKind::OrEq, start);
            return make_token(TokenKind::Or, start);
        case '^':
            if (match('=')) return make_token(TokenKind::XorEq, start);
            return make_token(TokenKind::Xor, start);
        case '.':
            if (match('.')) return make_token(TokenKind::DotDot, start);
            return make_token(TokenKind::Dot, start);
        case ':':
            if (match(':')) return make_token(TokenKind::DoubleColon, start);
            return make_token(TokenKind::Colon, start);
    }
    
    return make_error("Unexpected character", start);
}

Token Lexer::scan_identifier() {
    std::size_t start = pos_;
    
    while (!is_eof() && is_alnum(current())) {
        advance();
    }
    
    std::string_view text = source_.substr(start, pos_ - start);
    TokenKind kind = check_keyword(text);
    
    if (kind == TokenKind::True) {
        return make_token(kind, start, std::int64_t{1});
    } else if (kind == TokenKind::False) {
        return make_token(kind, start, std::int64_t{0});
    }
    
    return make_token(kind, start);
}

TokenKind Lexer::check_keyword(std::string_view text) const {
    static const std::unordered_map<std::string_view, TokenKind> keywords = {
        {"use", TokenKind::Use},
        {"fn", TokenKind::Fn},
        {"let", TokenKind::Let},
        {"mut", TokenKind::Mut},
        {"struct", TokenKind::Struct},
        {"class", TokenKind::Class},
        {"trait", TokenKind::Trait},
        {"impl", TokenKind::Impl},
        {"if", TokenKind::If},
        {"else", TokenKind::Else},
        {"while", TokenKind::While},
        {"for", TokenKind::For},
        {"in", TokenKind::In},
        {"match", TokenKind::Match},
        {"return", TokenKind::Return},
        {"break", TokenKind::Break},
        {"continue", TokenKind::Continue},
        {"as", TokenKind::As},
        {"true", TokenKind::True},
        {"false", TokenKind::False},
        {"legacy", TokenKind::Legacy},
        {"py", TokenKind::Py},
        {"js", TokenKind::Js},
        {"self", TokenKind::Self},
        {"pub", TokenKind::Pub},
    };
    
    auto it = keywords.find(text);
    return it != keywords.end() ? it->second : TokenKind::Identifier;
}

Token Lexer::scan_number() {
    std::size_t start = pos_;
    
    // Check for hex (0x), binary (0b), or octal (0o)
    if (current() == '0') {
        advance();
        if (current() == 'x' || current() == 'X') {
            advance();
            while (!is_eof() && (is_hex_digit(current()) || current() == '_')) {
                advance();
            }
            std::string_view text = source_.substr(start, pos_ - start);
            std::int64_t value = 0;
            for (std::size_t i = 2; i < text.size(); ++i) {
                if (text[i] == '_') continue;
                value *= 16;
                char c = text[i];
                if (c >= '0' && c <= '9') value += c - '0';
                else if (c >= 'a' && c <= 'f') value += 10 + c - 'a';
                else if (c >= 'A' && c <= 'F') value += 10 + c - 'A';
            }
            return make_token(TokenKind::IntLiteral, start, value);
        }
        if (current() == 'b' || current() == 'B') {
            advance();
            while (!is_eof() && (is_binary_digit(current()) || current() == '_')) {
                advance();
            }
            std::string_view text = source_.substr(start, pos_ - start);
            std::int64_t value = 0;
            for (std::size_t i = 2; i < text.size(); ++i) {
                if (text[i] == '_') continue;
                value = value * 2 + (text[i] - '0');
            }
            return make_token(TokenKind::IntLiteral, start, value);
        }
    }
    
    // Regular decimal number
    while (!is_eof() && (is_digit(current()) || current() == '_')) {
        advance();
    }
    
    // Check for float
    bool is_float = false;
    if (current() == '.' && is_digit(peek_char())) {
        is_float = true;
        advance(); // Consume '.'
        while (!is_eof() && (is_digit(current()) || current() == '_')) {
            advance();
        }
    }
    
    // Scientific notation
    if (current() == 'e' || current() == 'E') {
        is_float = true;
        advance();
        if (current() == '+' || current() == '-') advance();
        while (!is_eof() && is_digit(current())) {
            advance();
        }
    }
    
    std::string_view text = source_.substr(start, pos_ - start);
    
    // Remove underscores for parsing
    std::string clean;
    clean.reserve(text.size());
    for (char c : text) {
        if (c != '_') clean += c;
    }
    
    if (is_float) {
        double value = std::stod(clean);
        return make_token(TokenKind::FloatLiteral, start, value);
    } else {
        std::int64_t value = std::stoll(clean);
        return make_token(TokenKind::IntLiteral, start, value);
    }
}

Token Lexer::scan_string() {
    std::size_t start = pos_ - 1; // Include opening quote
    std::string value;
    
    while (!is_eof() && current() != '"') {
        if (current() == '\n') {
            return make_error("Unterminated string literal", start);
        }
        if (current() == '\\') {
            advance();
            if (is_eof()) {
                return make_error("Unterminated string literal", start);
            }
            switch (current()) {
                case 'n': value += '\n'; break;
                case 't': value += '\t'; break;
                case 'r': value += '\r'; break;
                case '\\': value += '\\'; break;
                case '"': value += '"'; break;
                case '0': value += '\0'; break;
                default:
                    return make_error("Invalid escape sequence", pos_ - 1);
            }
        } else {
            value += current();
        }
        advance();
    }
    
    if (is_eof()) {
        return make_error("Unterminated string literal", start);
    }
    
    advance(); // Closing quote
    return make_token(TokenKind::StringLiteral, start, std::move(value));
}

Token Lexer::scan_char() {
    std::size_t start = pos_ - 1; // Include opening quote
    
    if (is_eof() || current() == '\'') {
        return make_error("Empty character literal", start);
    }
    
    char value;
    if (current() == '\\') {
        advance();
        if (is_eof()) {
            return make_error("Unterminated character literal", start);
        }
        switch (current()) {
            case 'n': value = '\n'; break;
            case 't': value = '\t'; break;
            case 'r': value = '\r'; break;
            case '\\': value = '\\'; break;
            case '\'': value = '\''; break;
            case '0': value = '\0'; break;
            default:
                return make_error("Invalid escape sequence", pos_ - 1);
        }
    } else {
        value = current();
    }
    advance();
    
    if (current() != '\'') {
        return make_error("Unterminated character literal", start);
    }
    advance(); // Closing quote
    
    return make_token(TokenKind::CharLiteral, start, value);
}

Token Lexer::make_token(TokenKind kind, std::size_t start) const {
    return Token{
        kind,
        SourceSpan{start, pos_, line_, start - line_start_ + 1},
        source_.substr(start, pos_ - start),
        std::monostate{}
    };
}

Token Lexer::make_token(TokenKind kind, std::size_t start, LiteralValue value) const {
    return Token{
        kind,
        SourceSpan{start, pos_, line_, start - line_start_ + 1},
        source_.substr(start, pos_ - start),
        std::move(value)
    };
}

Token Lexer::make_error(std::string_view message, std::size_t start) {
    report_error(message, start);
    return Token{
        TokenKind::Error,
        SourceSpan{start, pos_, line_, start - line_start_ + 1},
        source_.substr(start, pos_ - start),
        std::monostate{}
    };
}

void Lexer::report_error(std::string_view message, std::size_t start) {
    SourceSpan span{start, pos_, line_, start - line_start_ + 1};
    errors_.push_back(
        error("E0001", std::string(message))
            .with_label(span, "")
    );
}

} // namespace idotc
