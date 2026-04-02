#ifndef IDOTC_SEMANTIC_SYMBOL_TABLE_HPP
#define IDOTC_SEMANTIC_SYMBOL_TABLE_HPP

#include "idotc/parser/ast.hpp"
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <optional>
#include <memory>

namespace idotc {

/// Kind of symbol
enum class SymbolKind {
    Variable,
    Function,
    Parameter,
    Struct,
    Class,
    Trait,
    Field,
    Method,
};

/// A symbol entry in the symbol table
struct Symbol {
    std::string name;
    SymbolKind kind;
    Type type;
    bool is_mutable = false;
    bool is_public = false;
    bool is_used = false;         // For dead code detection
    SourceSpan definition_span;
    
    // For functions
    std::vector<Parameter> params;
    std::optional<Type> return_type;
    
    // For structs/classes
    std::vector<Field> fields;
    std::vector<std::string> methods;
};

/// A scope containing symbols
class Scope {
public:
    explicit Scope(Scope* parent = nullptr);
    
    /// Define a new symbol in this scope
    bool define(Symbol symbol);
    
    /// Look up a symbol by name (checks parent scopes too)
    [[nodiscard]] Symbol* lookup(std::string_view name);
    [[nodiscard]] const Symbol* lookup(std::string_view name) const;
    
    /// Look up only in this scope (not parents)
    [[nodiscard]] Symbol* lookup_local(std::string_view name);
    [[nodiscard]] const Symbol* lookup_local(std::string_view name) const;
    
    /// Get all symbols in this scope
    [[nodiscard]] const std::unordered_map<std::string, Symbol>& symbols() const { 
        return symbols_; 
    }
    
    /// Get parent scope
    [[nodiscard]] Scope* parent() { return parent_; }
    [[nodiscard]] const Scope* parent() const { return parent_; }
    
private:
    Scope* parent_;
    std::unordered_map<std::string, Symbol> symbols_;
};

/// Symbol table with nested scopes
class SymbolTable {
public:
    SymbolTable();
    
    /// Enter a new scope
    void enter_scope();
    
    /// Exit the current scope
    void exit_scope();
    
    /// Define a symbol in the current scope
    bool define(Symbol symbol);
    
    /// Look up a symbol
    [[nodiscard]] Symbol* lookup(std::string_view name);
    [[nodiscard]] const Symbol* lookup(std::string_view name) const;
    
    /// Look up only in current scope
    [[nodiscard]] Symbol* lookup_local(std::string_view name);
    
    /// Get the current scope depth
    [[nodiscard]] std::size_t depth() const { return scopes_.size(); }
    
    /// Get the current scope
    [[nodiscard]] Scope& current_scope() { return *scopes_.back(); }
    [[nodiscard]] const Scope& current_scope() const { return *scopes_.back(); }
    
    /// Get the global scope
    [[nodiscard]] Scope& global_scope() { return *scopes_.front(); }
    [[nodiscard]] const Scope& global_scope() const { return *scopes_.front(); }
    
    /// Mark a symbol as used (for dead code detection)
    void mark_used(std::string_view name);
    
    /// Get all unused symbols (for warnings)
    [[nodiscard]] std::vector<const Symbol*> get_unused_symbols() const;
    
private:
    std::vector<std::unique_ptr<Scope>> scopes_;
};

/// Type resolution and comparison utilities
class TypeResolver {
public:
    /// Check if two types are equal
    [[nodiscard]] static bool types_equal(const Type& a, const Type& b);
    
    /// Check if a type is assignable to another
    [[nodiscard]] static bool is_assignable(const Type& target, const Type& source);
    
    /// Check if a type is numeric (int or float)
    [[nodiscard]] static bool is_numeric(const Type& type);
    
    /// Check if a type is integral
    [[nodiscard]] static bool is_integral(const Type& type);
    
    /// Check if a type is boolean
    [[nodiscard]] static bool is_boolean(const Type& type);
    
    /// Get the result type of a binary operation
    [[nodiscard]] static std::optional<Type> binary_result_type(
        BinaryOp op, const Type& left, const Type& right);
    
    /// Get the result type of a unary operation
    [[nodiscard]] static std::optional<Type> unary_result_type(
        UnaryOp op, const Type& operand);
};

} // namespace idotc

#endif // IDOTC_SEMANTIC_SYMBOL_TABLE_HPP
