#ifndef IDOTC_PARSER_PARSER_HPP
#define IDOTC_PARSER_PARSER_HPP

#include "idotc/lexer/lexer.hpp"
#include "idotc/parser/ast.hpp"
#include "idotc/util/error.hpp"
#include <vector>
#include <optional>

namespace idotc {

/// Recursive descent parser for Advanced-idot
class Parser {
public:
    /// Create a parser from a list of tokens
    explicit Parser(std::vector<Token> tokens);
    
    /// Parse a complete program
    [[nodiscard]] Result<Program> parse_program();
    
    /// Get any parser errors that occurred
    [[nodiscard]] const std::vector<Diagnostic>& errors() const { return errors_; }
    
    /// Check if there were errors
    [[nodiscard]] bool has_errors() const { return !errors_.empty(); }
    
private:
    std::vector<Token> tokens_;
    std::size_t current_ = 0;
    std::vector<Diagnostic> errors_;
    
    // Token access
    [[nodiscard]] const Token& current() const;
    [[nodiscard]] const Token& peek() const;
    [[nodiscard]] const Token& previous() const;
    [[nodiscard]] bool is_at_end() const;
    const Token& advance();
    
    // Matching
    [[nodiscard]] bool check(TokenKind kind) const;
    [[nodiscard]] bool match(TokenKind kind);
    [[nodiscard]] bool match(std::initializer_list<TokenKind> kinds);
    const Token& expect(TokenKind kind, std::string_view message);
    
    // Error handling
    void report_error(std::string_view message);
    void report_error(std::string_view message, const Token& token);
    void synchronize();
    
    // Parsing methods
    [[nodiscard]] Result<ItemPtr> parse_item();
    [[nodiscard]] Result<ItemPtr> parse_function();
    [[nodiscard]] Result<ItemPtr> parse_struct();
    [[nodiscard]] Result<ItemPtr> parse_class();
    [[nodiscard]] Result<ItemPtr> parse_trait();
    [[nodiscard]] Result<ItemPtr> parse_impl();
    [[nodiscard]] Result<ItemPtr> parse_use();
    
    [[nodiscard]] Result<StmtPtr> parse_statement();
    [[nodiscard]] Result<StmtPtr> parse_let_statement();
    [[nodiscard]] Result<StmtPtr> parse_return_statement();
    [[nodiscard]] Result<StmtPtr> parse_if_statement();
    [[nodiscard]] Result<StmtPtr> parse_while_statement();
    [[nodiscard]] Result<StmtPtr> parse_for_statement();
    [[nodiscard]] Result<StmtPtr> parse_match_statement();
    [[nodiscard]] Result<StmtPtr> parse_expression_statement();
    [[nodiscard]] Result<std::vector<StmtPtr>> parse_block();
    
    [[nodiscard]] Result<ExprPtr> parse_expression();
    [[nodiscard]] Result<ExprPtr> parse_assignment();
    [[nodiscard]] Result<ExprPtr> parse_or();
    [[nodiscard]] Result<ExprPtr> parse_and();
    [[nodiscard]] Result<ExprPtr> parse_equality();
    [[nodiscard]] Result<ExprPtr> parse_comparison();
    [[nodiscard]] Result<ExprPtr> parse_bitwise_or();
    [[nodiscard]] Result<ExprPtr> parse_bitwise_xor();
    [[nodiscard]] Result<ExprPtr> parse_bitwise_and();
    [[nodiscard]] Result<ExprPtr> parse_shift();
    [[nodiscard]] Result<ExprPtr> parse_term();
    [[nodiscard]] Result<ExprPtr> parse_factor();
    [[nodiscard]] Result<ExprPtr> parse_unary();
    [[nodiscard]] Result<ExprPtr> parse_call();
    [[nodiscard]] Result<ExprPtr> parse_primary();
    
    [[nodiscard]] Result<Type> parse_type();
    [[nodiscard]] Result<Parameter> parse_parameter();
    [[nodiscard]] Result<Field> parse_field();
    [[nodiscard]] Result<MatchArm> parse_match_arm();
    [[nodiscard]] Result<std::vector<ExprPtr>> parse_arguments();
};

} // namespace idotc

#endif // IDOTC_PARSER_PARSER_HPP
