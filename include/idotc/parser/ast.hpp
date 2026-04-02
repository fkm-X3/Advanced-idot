#ifndef IDOTC_PARSER_AST_HPP
#define IDOTC_PARSER_AST_HPP

#include "idotc/lexer/token.hpp"
#include <memory>
#include <vector>
#include <string>
#include <variant>
#include <optional>

namespace idotc {

// Forward declarations
struct Expression;
struct Statement;
struct Item;

using ExprPtr = std::unique_ptr<Expression>;
using StmtPtr = std::unique_ptr<Statement>;
using ItemPtr = std::unique_ptr<Item>;

/// Type representation
struct Type {
    std::string name;           // Type name (int, float, MyStruct, etc.)
    std::vector<Type> generics; // Generic type parameters
    bool is_reference = false;  // &Type
    bool is_mutable = false;    // mut modifier
    SourceSpan span;
    
    static Type primitive(std::string name, SourceSpan span = {}) {
        return Type{std::move(name), {}, false, false, span};
    }
    
    static Type int_type(SourceSpan span = {}) { return primitive("int", span); }
    static Type float_type(SourceSpan span = {}) { return primitive("float", span); }
    static Type bool_type(SourceSpan span = {}) { return primitive("bool", span); }
    static Type string_type(SourceSpan span = {}) { return primitive("string", span); }
    static Type char_type(SourceSpan span = {}) { return primitive("char", span); }
    static Type void_type(SourceSpan span = {}) { return primitive("void", span); }
};

/// Binary operators
enum class BinaryOp {
    Add, Sub, Mul, Div, Mod,           // Arithmetic
    Eq, NotEq, Lt, LtEq, Gt, GtEq,     // Comparison
    And, Or,                            // Logical
    BitAnd, BitOr, BitXor, Shl, Shr,   // Bitwise
};

/// Unary operators
enum class UnaryOp {
    Neg,     // -x
    Not,     // !x
    BitNot,  // ~x
    Ref,     // &x
    Deref,   // *x
};

/// Literal value in AST
struct LiteralExpr {
    LiteralValue value;
    SourceSpan span;
};

/// Variable reference
struct VariableExpr {
    std::string name;
    SourceSpan span;
};

/// Binary operation: left op right
struct BinaryExpr {
    BinaryOp op;
    ExprPtr left;
    ExprPtr right;
    SourceSpan span;
};

/// Unary operation: op expr
struct UnaryExpr {
    UnaryOp op;
    ExprPtr operand;
    SourceSpan span;
};

/// Function call: name(args...)
struct CallExpr {
    std::string name;           // Function name (or expression later)
    std::vector<ExprPtr> args;
    SourceSpan span;
};

/// Member access: expr.member
struct MemberExpr {
    ExprPtr object;
    std::string member;
    SourceSpan span;
};

/// Index access: expr[index]
struct IndexExpr {
    ExprPtr object;
    ExprPtr index;
    SourceSpan span;
};

/// If expression: if cond { then } else { else }
struct IfExpr {
    ExprPtr condition;
    std::vector<StmtPtr> then_block;
    std::vector<StmtPtr> else_block; // Empty if no else
    SourceSpan span;
};

/// Match arm: pattern => body
struct MatchArm {
    ExprPtr pattern;            // Pattern (for now, just expressions)
    std::vector<StmtPtr> body;
    SourceSpan span;
};

/// Match expression: match expr { arms... }
struct MatchExpr {
    ExprPtr scrutinee;
    std::vector<MatchArm> arms;
    SourceSpan span;
};

/// Block expression: { statements... }
struct BlockExpr {
    std::vector<StmtPtr> statements;
    SourceSpan span;
};

/// Array literal: [a, b, c]
struct ArrayExpr {
    std::vector<ExprPtr> elements;
    SourceSpan span;
};

/// Expression node (sum type)
using ExpressionVariant = std::variant<
    LiteralExpr,
    VariableExpr,
    BinaryExpr,
    UnaryExpr,
    CallExpr,
    MemberExpr,
    IndexExpr,
    IfExpr,
    MatchExpr,
    BlockExpr,
    ArrayExpr
>;

struct Expression {
    ExpressionVariant variant;
    SourceSpan span;
    
    template<typename T>
    static ExprPtr make(T&& expr) {
        auto e = std::make_unique<Expression>();
        e->variant = std::forward<T>(expr);
        if constexpr (requires { expr.span; }) {
            e->span = expr.span;
        }
        return e;
    }
};

// --- Statements ---

/// Let binding: let name: type = value
struct LetStmt {
    std::string name;
    std::optional<Type> type;  // Optional type annotation
    ExprPtr initializer;       // Optional initializer
    bool is_mutable = false;
    SourceSpan span;
};

/// Expression statement
struct ExprStmt {
    ExprPtr expression;
    SourceSpan span;
};

/// Return statement: return expr?
struct ReturnStmt {
    std::optional<ExprPtr> value;
    SourceSpan span;
};

/// While loop: while cond { body }
struct WhileStmt {
    ExprPtr condition;
    std::vector<StmtPtr> body;
    SourceSpan span;
};

/// For loop: for name in iter { body }
struct ForStmt {
    std::string variable;
    ExprPtr iterator;
    std::vector<StmtPtr> body;
    SourceSpan span;
};

/// Break statement
struct BreakStmt {
    SourceSpan span;
};

/// Continue statement
struct ContinueStmt {
    SourceSpan span;
};

/// Assignment: target = value
struct AssignStmt {
    ExprPtr target;
    ExprPtr value;
    SourceSpan span;
};

/// Statement node (sum type)
using StatementVariant = std::variant<
    LetStmt,
    ExprStmt,
    ReturnStmt,
    WhileStmt,
    ForStmt,
    BreakStmt,
    ContinueStmt,
    AssignStmt
>;

struct Statement {
    StatementVariant variant;
    SourceSpan span;
    
    template<typename T>
    static StmtPtr make(T&& stmt) {
        auto s = std::make_unique<Statement>();
        s->variant = std::forward<T>(stmt);
        if constexpr (requires { stmt.span; }) {
            s->span = stmt.span;
        }
        return s;
    }
};

// --- Items (top-level declarations) ---

/// Function parameter
struct Parameter {
    std::string name;
    Type type;
    SourceSpan span;
};

/// Function definition
struct FunctionItem {
    std::string name;
    std::vector<Parameter> params;
    std::optional<Type> return_type;
    std::vector<StmtPtr> body;
    bool is_public = false;
    SourceSpan span;
};

/// Struct field
struct Field {
    std::string name;
    Type type;
    bool is_public = false;
    SourceSpan span;
};

/// Struct definition
struct StructItem {
    std::string name;
    std::vector<Field> fields;
    bool is_public = false;
    SourceSpan span;
};

/// Class definition (struct with methods)
struct ClassItem {
    std::string name;
    std::vector<Field> fields;
    std::vector<FunctionItem> methods;
    bool is_public = false;
    SourceSpan span;
};

/// Trait definition
struct TraitItem {
    std::string name;
    std::vector<FunctionItem> methods; // Method signatures
    bool is_public = false;
    SourceSpan span;
};

/// Impl block
struct ImplItem {
    std::string type_name;
    std::optional<std::string> trait_name; // impl Trait for Type
    std::vector<FunctionItem> methods;
    SourceSpan span;
};

/// Use/import statement
struct UseItem {
    std::string path;           // File path
    std::string alias;          // as name
    std::optional<std::string> language; // py, js, or none for .idot
    SourceSpan span;
};

/// Item node (sum type)
using ItemVariant = std::variant<
    FunctionItem,
    StructItem,
    ClassItem,
    TraitItem,
    ImplItem,
    UseItem
>;

struct Item {
    ItemVariant variant;
    SourceSpan span;
    
    template<typename T>
    static ItemPtr make(T&& item) {
        auto i = std::make_unique<Item>();
        i->variant = std::forward<T>(item);
        if constexpr (requires { item.span; }) {
            i->span = item.span;
        }
        return i;
    }
};

/// A complete program (compilation unit)
struct Program {
    std::vector<ItemPtr> items;
    SourceSpan span;
};

// Helper functions for binary operator precedence
[[nodiscard]] constexpr int binary_op_precedence(BinaryOp op) noexcept {
    switch (op) {
        case BinaryOp::Or:     return 1;
        case BinaryOp::And:    return 2;
        case BinaryOp::BitOr:  return 3;
        case BinaryOp::BitXor: return 4;
        case BinaryOp::BitAnd: return 5;
        case BinaryOp::Eq:
        case BinaryOp::NotEq:  return 6;
        case BinaryOp::Lt:
        case BinaryOp::LtEq:
        case BinaryOp::Gt:
        case BinaryOp::GtEq:   return 7;
        case BinaryOp::Shl:
        case BinaryOp::Shr:    return 8;
        case BinaryOp::Add:
        case BinaryOp::Sub:    return 9;
        case BinaryOp::Mul:
        case BinaryOp::Div:
        case BinaryOp::Mod:    return 10;
    }
    return 0;
}

[[nodiscard]] constexpr bool is_right_associative(BinaryOp op) noexcept {
    // All binary ops are left-associative in this language
    (void)op;
    return false;
}

} // namespace idotc

#endif // IDOTC_PARSER_AST_HPP
