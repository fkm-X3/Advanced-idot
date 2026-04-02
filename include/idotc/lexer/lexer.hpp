#ifndef IDOTC_LEXER_LEXER_HPP
#define IDOTC_LEXER_LEXER_HPP

#include "idotc/lexer/token.hpp"
#include "idotc/util/error.hpp"
#include <string>
#include <string_view>
#include <vector>

namespace idotc {

class SourceFile;

/// Hand-written lexer with zero-copy string_view tokens
class Lexer {
public:
    /// Create a lexer for the given source
    explicit Lexer(std::string_view source);
    
    /// Tokenize the entire source, returning all tokens
    [[nodiscard]] std::vector<Token> tokenize();
    
    /// Get the next token (advances the lexer)
    [[nodiscard]] Token next_token();
    
    /// Peek at the next token without consuming it
    [[nodiscard]] Token peek() const;
    
    /// Check if we've reached the end
    [[nodiscard]] bool is_eof() const;
    
    /// Get any lexer errors that occurred
    [[nodiscard]] const std::vector<Diagnostic>& errors() const { return errors_; }
    
    /// Check if there were errors
    [[nodiscard]] bool has_errors() const { return !errors_.empty(); }
    
private:
    std::string_view source_;
    std::size_t pos_ = 0;
    std::size_t line_ = 1;
    std::size_t column_ = 1;
    std::size_t line_start_ = 0;
    std::vector<Diagnostic> errors_;
    
    // Character access
    [[nodiscard]] char current() const;
    [[nodiscard]] char peek_char() const;
    [[nodiscard]] char peek_char(std::size_t offset) const;
    void advance();
    void advance(std::size_t count);
    
    // Helpers
    [[nodiscard]] bool match(char expected);
    [[nodiscard]] bool match(std::string_view expected);
    void skip_whitespace();
    void skip_line_comment();
    bool skip_block_comment();
    
    // Token scanning
    [[nodiscard]] Token scan_token();
    [[nodiscard]] Token scan_identifier();
    [[nodiscard]] Token scan_number();
    [[nodiscard]] Token scan_string();
    [[nodiscard]] Token scan_char();
    
    // Keyword lookup
    [[nodiscard]] TokenKind check_keyword(std::string_view text) const;
    
    // Token creation
    [[nodiscard]] Token make_token(TokenKind kind, std::size_t start) const;
    [[nodiscard]] Token make_token(TokenKind kind, std::size_t start, LiteralValue value) const;
    [[nodiscard]] Token make_error(std::string_view message, std::size_t start);
    
    // Error reporting
    void report_error(std::string_view message, std::size_t start);
};

} // namespace idotc

#endif // IDOTC_LEXER_LEXER_HPP
