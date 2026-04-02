#ifndef IDOTC_SEMANTIC_TYPE_CHECKER_HPP
#define IDOTC_SEMANTIC_TYPE_CHECKER_HPP

#include "idotc/parser/ast.hpp"
#include "idotc/semantic/symbol_table.hpp"
#include "idotc/util/error.hpp"
#include <vector>

namespace idotc {

/// Performs type checking and semantic analysis on the AST
class TypeChecker {
public:
    TypeChecker();
    
    /// Check a complete program, returns true if no errors
    [[nodiscard]] bool check(const Program& program);
    
    /// Get type checking errors
    [[nodiscard]] const std::vector<Diagnostic>& errors() const { return errors_; }
    
    /// Check if there were errors
    [[nodiscard]] bool has_errors() const { return !errors_.empty(); }
    
    /// Get the symbol table after checking
    [[nodiscard]] const SymbolTable& symbols() const { return symbols_; }
    
private:
    SymbolTable symbols_;
    std::vector<Diagnostic> errors_;
    std::optional<Type> current_return_type_;
    
    // Item checking
    void check_item(const Item& item);
    void check_function(const FunctionItem& fn);
    void check_struct(const StructItem& s);
    void check_class(const ClassItem& c);
    void check_trait(const TraitItem& t);
    void check_impl(const ImplItem& impl);
    
    // Statement checking
    void check_statement(const Statement& stmt);
    void check_let(const LetStmt& let);
    void check_return(const ReturnStmt& ret);
    void check_while(const WhileStmt& loop);
    void check_for(const ForStmt& loop);
    void check_assign(const AssignStmt& assign);
    void check_block(const std::vector<StmtPtr>& stmts);
    
    // Expression checking (returns inferred type)
    [[nodiscard]] std::optional<Type> check_expression(const Expression& expr);
    [[nodiscard]] std::optional<Type> check_literal(const LiteralExpr& lit);
    [[nodiscard]] std::optional<Type> check_variable(const VariableExpr& var);
    [[nodiscard]] std::optional<Type> check_binary(const BinaryExpr& bin);
    [[nodiscard]] std::optional<Type> check_unary(const UnaryExpr& un);
    [[nodiscard]] std::optional<Type> check_call(const CallExpr& call);
    [[nodiscard]] std::optional<Type> check_member(const MemberExpr& member);
    [[nodiscard]] std::optional<Type> check_index(const IndexExpr& index);
    [[nodiscard]] std::optional<Type> check_if(const IfExpr& if_expr);
    [[nodiscard]] std::optional<Type> check_match(const MatchExpr& match);
    [[nodiscard]] std::optional<Type> check_block_expr(const BlockExpr& block);
    [[nodiscard]] std::optional<Type> check_array(const ArrayExpr& arr);
    
    // Error reporting
    void report_error(const std::string& message, SourceSpan span);
    void report_type_error(const Type& expected, const Type& got, SourceSpan span);
    void report_undefined(const std::string& kind, const std::string& name, SourceSpan span);
};

} // namespace idotc

#endif // IDOTC_SEMANTIC_TYPE_CHECKER_HPP
