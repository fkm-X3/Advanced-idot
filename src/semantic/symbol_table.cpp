#include "idotc/semantic/symbol_table.hpp"
#include <algorithm>
#include <functional>

namespace idotc {

// --- Scope ---

Scope::Scope(Scope* parent)
    : parent_(parent)
{}

bool Scope::define(Symbol symbol) {
    auto [it, inserted] = symbols_.try_emplace(symbol.name, std::move(symbol));
    return inserted;
}

Symbol* Scope::lookup(std::string_view name) {
    if (auto* sym = lookup_local(name)) {
        return sym;
    }
    if (parent_) {
        return parent_->lookup(name);
    }
    return nullptr;
}

const Symbol* Scope::lookup(std::string_view name) const {
    if (auto* sym = lookup_local(name)) {
        return sym;
    }
    if (parent_) {
        return parent_->lookup(name);
    }
    return nullptr;
}

Symbol* Scope::lookup_local(std::string_view name) {
    auto it = symbols_.find(std::string(name));
    return it != symbols_.end() ? &it->second : nullptr;
}

const Symbol* Scope::lookup_local(std::string_view name) const {
    auto it = symbols_.find(std::string(name));
    return it != symbols_.end() ? &it->second : nullptr;
}

// --- SymbolTable ---

SymbolTable::SymbolTable() {
    // Create global scope
    scopes_.push_back(std::make_unique<Scope>(nullptr));
}

void SymbolTable::enter_scope() {
    scopes_.push_back(std::make_unique<Scope>(scopes_.back().get()));
}

void SymbolTable::exit_scope() {
    if (scopes_.size() > 1) {
        scopes_.pop_back();
    }
}

bool SymbolTable::define(Symbol symbol) {
    return current_scope().define(std::move(symbol));
}

Symbol* SymbolTable::lookup(std::string_view name) {
    return current_scope().lookup(name);
}

const Symbol* SymbolTable::lookup(std::string_view name) const {
    return current_scope().lookup(name);
}

Symbol* SymbolTable::lookup_local(std::string_view name) {
    return current_scope().lookup_local(name);
}

void SymbolTable::mark_used(std::string_view name) {
    if (auto* sym = lookup(name)) {
        sym->is_used = true;
    }
}

std::vector<const Symbol*> SymbolTable::get_unused_symbols() const {
    std::vector<const Symbol*> unused;
    
    // Helper to collect unused from a scope
    std::function<void(const Scope&)> collect = [&](const Scope& scope) {
        for (const auto& [name, sym] : scope.symbols()) {
            if (!sym.is_used && !sym.is_public) {
                // Only report user-defined symbols, not built-ins
                if (sym.kind == SymbolKind::Function || 
                    sym.kind == SymbolKind::Variable ||
                    sym.kind == SymbolKind::Struct ||
                    sym.kind == SymbolKind::Class) {
                    unused.push_back(&sym);
                }
            }
        }
    };
    
    for (const auto& scope : scopes_) {
        collect(*scope);
    }
    
    return unused;
}

// --- TypeResolver ---

bool TypeResolver::types_equal(const Type& a, const Type& b) {
    if (a.name != b.name) return false;
    if (a.is_reference != b.is_reference) return false;
    if (a.generics.size() != b.generics.size()) return false;
    
    for (std::size_t i = 0; i < a.generics.size(); ++i) {
        if (!types_equal(a.generics[i], b.generics[i])) return false;
    }
    
    return true;
}

bool TypeResolver::is_assignable(const Type& target, const Type& source) {
    // For now, require exact type match
    // Later: support coercion (int -> float), trait bounds, etc.
    return types_equal(target, source);
}

bool TypeResolver::is_numeric(const Type& type) {
    return type.name == "int" || type.name == "float";
}

bool TypeResolver::is_integral(const Type& type) {
    return type.name == "int";
}

bool TypeResolver::is_boolean(const Type& type) {
    return type.name == "bool";
}

std::optional<Type> TypeResolver::binary_result_type(
    BinaryOp op, const Type& left, const Type& right) 
{
    switch (op) {
        // Arithmetic: both numeric, result is wider type
        case BinaryOp::Add:
        case BinaryOp::Sub:
        case BinaryOp::Mul:
        case BinaryOp::Div:
        case BinaryOp::Mod:
            if (!is_numeric(left) || !is_numeric(right)) return std::nullopt;
            if (left.name == "float" || right.name == "float") {
                return Type::float_type();
            }
            return Type::int_type();
            
        // Comparison: both same type, result is bool
        case BinaryOp::Eq:
        case BinaryOp::NotEq:
        case BinaryOp::Lt:
        case BinaryOp::LtEq:
        case BinaryOp::Gt:
        case BinaryOp::GtEq:
            if (!types_equal(left, right)) return std::nullopt;
            return Type::bool_type();
            
        // Logical: both bool, result is bool
        case BinaryOp::And:
        case BinaryOp::Or:
            if (!is_boolean(left) || !is_boolean(right)) return std::nullopt;
            return Type::bool_type();
            
        // Bitwise: both int, result is int
        case BinaryOp::BitAnd:
        case BinaryOp::BitOr:
        case BinaryOp::BitXor:
        case BinaryOp::Shl:
        case BinaryOp::Shr:
            if (!is_integral(left) || !is_integral(right)) return std::nullopt;
            return Type::int_type();
    }
    
    return std::nullopt;
}

std::optional<Type> TypeResolver::unary_result_type(UnaryOp op, const Type& operand) {
    switch (op) {
        case UnaryOp::Neg:
            if (!is_numeric(operand)) return std::nullopt;
            return operand;
        case UnaryOp::Not:
            if (!is_boolean(operand)) return std::nullopt;
            return Type::bool_type();
        case UnaryOp::BitNot:
            if (!is_integral(operand)) return std::nullopt;
            return Type::int_type();
        case UnaryOp::Ref:
            // &x creates a reference to x
            return Type{operand.name, operand.generics, true, operand.is_mutable, operand.span};
        case UnaryOp::Deref:
            // *x dereferences a reference
            if (!operand.is_reference) return std::nullopt;
            return Type{operand.name, operand.generics, false, operand.is_mutable, operand.span};
    }
    return std::nullopt;
}

} // namespace idotc
