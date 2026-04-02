#include "idotc/semantic/type_checker.hpp"
#include <unordered_set>

namespace idotc {

TypeChecker::TypeChecker() = default;

bool TypeChecker::check(const Program& program) {
    // First pass: collect all type and function declarations
    for (const auto& item : program.items) {
        std::visit([this](const auto& i) {
            using T = std::decay_t<decltype(i)>;
            if constexpr (std::is_same_v<T, FunctionItem>) {
                Symbol sym;
                sym.name = i.name;
                sym.kind = SymbolKind::Function;
                sym.params = i.params;
                sym.return_type = i.return_type;
                sym.definition_span = i.span;
                symbols_.define(std::move(sym));
            } else if constexpr (std::is_same_v<T, StructItem>) {
                Symbol sym;
                sym.name = i.name;
                sym.kind = SymbolKind::Struct;
                sym.fields = i.fields;
                sym.definition_span = i.span;
                symbols_.define(std::move(sym));
            } else if constexpr (std::is_same_v<T, ClassItem>) {
                Symbol sym;
                sym.name = i.name;
                sym.kind = SymbolKind::Class;
                sym.fields = i.fields;
                for (const auto& m : i.methods) {
                    sym.methods.push_back(m.name);
                }
                sym.definition_span = i.span;
                symbols_.define(std::move(sym));
            }
        }, item->variant);
    }
    
    // Second pass: check implementations
    for (const auto& item : program.items) {
        check_item(*item);
    }
    
    return !has_errors();
}

void TypeChecker::check_item(const Item& item) {
    std::visit([this](const auto& i) {
        using T = std::decay_t<decltype(i)>;
        if constexpr (std::is_same_v<T, FunctionItem>) {
            check_function(i);
        } else if constexpr (std::is_same_v<T, StructItem>) {
            check_struct(i);
        } else if constexpr (std::is_same_v<T, ClassItem>) {
            check_class(i);
        } else if constexpr (std::is_same_v<T, TraitItem>) {
            check_trait(i);
        } else if constexpr (std::is_same_v<T, ImplItem>) {
            check_impl(i);
        }
    }, item.variant);
}

void TypeChecker::check_function(const FunctionItem& fn) {
    symbols_.enter_scope();
    
    // Add parameters to scope
    for (const auto& param : fn.params) {
        Symbol sym;
        sym.name = param.name;
        sym.kind = SymbolKind::Parameter;
        sym.type = param.type;
        sym.definition_span = param.span;
        symbols_.define(std::move(sym));
    }
    
    current_return_type_ = fn.return_type;
    check_block(fn.body);
    current_return_type_ = std::nullopt;
    
    symbols_.exit_scope();
}

void TypeChecker::check_struct(const StructItem& s) {
    // Check for duplicate field names
    std::unordered_set<std::string> seen;
    for (const auto& field : s.fields) {
        if (!seen.insert(field.name).second) {
            report_error("Duplicate field: " + field.name, field.span);
        }
    }
}

void TypeChecker::check_class(const ClassItem& c) {
    // Check fields
    std::unordered_set<std::string> seen;
    for (const auto& field : c.fields) {
        if (!seen.insert(field.name).second) {
            report_error("Duplicate field: " + field.name, field.span);
        }
    }
    
    // Check methods
    for (const auto& method : c.methods) {
        check_function(method);
    }
}

void TypeChecker::check_trait(const TraitItem& /*t*/) {
    // Traits just declare method signatures, minimal checking
}

void TypeChecker::check_impl(const ImplItem& impl) {
    // Check that the type exists
    if (!symbols_.lookup(impl.type_name)) {
        report_undefined("type", impl.type_name, impl.span);
        return;
    }
    
    // Check methods
    for (const auto& method : impl.methods) {
        check_function(method);
    }
}

void TypeChecker::check_statement(const Statement& stmt) {
    std::visit([this](const auto& s) {
        using T = std::decay_t<decltype(s)>;
        if constexpr (std::is_same_v<T, LetStmt>) {
            check_let(s);
        } else if constexpr (std::is_same_v<T, ReturnStmt>) {
            check_return(s);
        } else if constexpr (std::is_same_v<T, WhileStmt>) {
            check_while(s);
        } else if constexpr (std::is_same_v<T, ForStmt>) {
            check_for(s);
        } else if constexpr (std::is_same_v<T, AssignStmt>) {
            check_assign(s);
        } else if constexpr (std::is_same_v<T, ExprStmt>) {
            (void)check_expression(*s.expression);
        }
    }, stmt.variant);
}

void TypeChecker::check_let(const LetStmt& let) {
    std::optional<Type> init_type;
    if (let.initializer) {
        init_type = check_expression(*let.initializer);
    }
    
    Type var_type;
    if (let.type) {
        var_type = *let.type;
        if (init_type && !TypeResolver::is_assignable(var_type, *init_type)) {
            report_type_error(var_type, *init_type, let.span);
        }
    } else if (init_type) {
        var_type = *init_type;
    } else {
        report_error("Cannot infer type without initializer", let.span);
        return;
    }
    
    Symbol sym;
    sym.name = let.name;
    sym.kind = SymbolKind::Variable;
    sym.type = var_type;
    sym.is_mutable = let.is_mutable;
    sym.definition_span = let.span;
    symbols_.define(std::move(sym));
}

void TypeChecker::check_return(const ReturnStmt& ret) {
    if (ret.value) {
        auto val_type = check_expression(**ret.value);
        if (val_type && current_return_type_) {
            if (!TypeResolver::is_assignable(*current_return_type_, *val_type)) {
                report_type_error(*current_return_type_, *val_type, ret.span);
            }
        }
    } else if (current_return_type_ && current_return_type_->name != "void") {
        report_error("Expected return value", ret.span);
    }
}

void TypeChecker::check_while(const WhileStmt& loop) {
    auto cond_type = check_expression(*loop.condition);
    if (cond_type && !TypeResolver::is_boolean(*cond_type)) {
        report_error("While condition must be boolean", loop.condition->span);
    }
    
    symbols_.enter_scope();
    check_block(loop.body);
    symbols_.exit_scope();
}

void TypeChecker::check_for(const ForStmt& loop) {
    auto iter_type = check_expression(*loop.iterator);
    
    symbols_.enter_scope();
    
    // Add loop variable
    Symbol sym;
    sym.name = loop.variable;
    sym.kind = SymbolKind::Variable;
    // For now, assume element type is int
    sym.type = Type::int_type();
    sym.definition_span = loop.span;
    symbols_.define(std::move(sym));
    
    check_block(loop.body);
    symbols_.exit_scope();
}

void TypeChecker::check_assign(const AssignStmt& assign) {
    auto target_type = check_expression(*assign.target);
    auto value_type = check_expression(*assign.value);
    
    if (auto* var = std::get_if<VariableExpr>(&assign.target->variant)) {
        auto* sym = symbols_.lookup(var->name);
        if (sym && !sym->is_mutable) {
            report_error("Cannot assign to immutable variable: " + var->name, assign.span);
        }
    }
    
    if (target_type && value_type) {
        if (!TypeResolver::is_assignable(*target_type, *value_type)) {
            report_type_error(*target_type, *value_type, assign.span);
        }
    }
}

void TypeChecker::check_block(const std::vector<StmtPtr>& stmts) {
    for (const auto& stmt : stmts) {
        check_statement(*stmt);
    }
}

std::optional<Type> TypeChecker::check_expression(const Expression& expr) {
    return std::visit([this](const auto& e) -> std::optional<Type> {
        using T = std::decay_t<decltype(e)>;
        if constexpr (std::is_same_v<T, LiteralExpr>) {
            return check_literal(e);
        } else if constexpr (std::is_same_v<T, VariableExpr>) {
            return check_variable(e);
        } else if constexpr (std::is_same_v<T, BinaryExpr>) {
            return check_binary(e);
        } else if constexpr (std::is_same_v<T, UnaryExpr>) {
            return check_unary(e);
        } else if constexpr (std::is_same_v<T, CallExpr>) {
            return check_call(e);
        } else if constexpr (std::is_same_v<T, MemberExpr>) {
            return check_member(e);
        } else if constexpr (std::is_same_v<T, IndexExpr>) {
            return check_index(e);
        } else if constexpr (std::is_same_v<T, IfExpr>) {
            return check_if(e);
        } else if constexpr (std::is_same_v<T, MatchExpr>) {
            return check_match(e);
        } else if constexpr (std::is_same_v<T, BlockExpr>) {
            return check_block_expr(e);
        } else if constexpr (std::is_same_v<T, ArrayExpr>) {
            return check_array(e);
        } else {
            return std::nullopt;
        }
    }, expr.variant);
}

std::optional<Type> TypeChecker::check_literal(const LiteralExpr& lit) {
    return std::visit([](const auto& v) -> std::optional<Type> {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::int64_t>) {
            return Type::int_type();
        } else if constexpr (std::is_same_v<T, double>) {
            return Type::float_type();
        } else if constexpr (std::is_same_v<T, bool>) {
            return Type::bool_type();
        } else if constexpr (std::is_same_v<T, char>) {
            return Type::char_type();
        } else if constexpr (std::is_same_v<T, std::string>) {
            return Type::string_type();
        } else {
            return std::nullopt;
        }
    }, lit.value);
}

std::optional<Type> TypeChecker::check_variable(const VariableExpr& var) {
    auto* sym = symbols_.lookup(var.name);
    if (!sym) {
        report_undefined("variable", var.name, var.span);
        return std::nullopt;
    }
    symbols_.mark_used(var.name);
    return sym->type;
}

std::optional<Type> TypeChecker::check_binary(const BinaryExpr& bin) {
    auto left_type = check_expression(*bin.left);
    auto right_type = check_expression(*bin.right);
    
    if (!left_type || !right_type) return std::nullopt;
    
    auto result = TypeResolver::binary_result_type(bin.op, *left_type, *right_type);
    if (!result) {
        report_error("Invalid operand types for binary operator", bin.span);
    }
    return result;
}

std::optional<Type> TypeChecker::check_unary(const UnaryExpr& un) {
    auto operand_type = check_expression(*un.operand);
    if (!operand_type) return std::nullopt;
    
    auto result = TypeResolver::unary_result_type(un.op, *operand_type);
    if (!result) {
        report_error("Invalid operand type for unary operator", un.span);
    }
    return result;
}

std::optional<Type> TypeChecker::check_call(const CallExpr& call) {
    auto* sym = symbols_.lookup(call.name);
    if (!sym) {
        report_undefined("function", call.name, call.span);
        return std::nullopt;
    }
    
    if (sym->kind != SymbolKind::Function) {
        report_error("Not a function: " + call.name, call.span);
        return std::nullopt;
    }
    
    if (call.args.size() != sym->params.size()) {
        report_error("Wrong number of arguments", call.span);
        return std::nullopt;
    }
    
    for (size_t i = 0; i < call.args.size(); ++i) {
        auto arg_type = check_expression(*call.args[i]);
        if (arg_type && !TypeResolver::is_assignable(sym->params[i].type, *arg_type)) {
            report_type_error(sym->params[i].type, *arg_type, call.args[i]->span);
        }
    }
    
    symbols_.mark_used(call.name);
    return sym->return_type;
}

std::optional<Type> TypeChecker::check_member(const MemberExpr& member) {
    auto obj_type = check_expression(*member.object);
    if (!obj_type) return std::nullopt;
    
    auto* sym = symbols_.lookup(obj_type->name);
    if (!sym) return std::nullopt;
    
    for (const auto& field : sym->fields) {
        if (field.name == member.member) {
            return field.type;
        }
    }
    
    report_error("Unknown member: " + member.member, member.span);
    return std::nullopt;
}

std::optional<Type> TypeChecker::check_index(const IndexExpr& index) {
    auto obj_type = check_expression(*index.object);
    auto idx_type = check_expression(*index.index);
    
    if (idx_type && !TypeResolver::is_integral(*idx_type)) {
        report_error("Index must be an integer", index.index->span);
    }
    
    // TODO: Handle array element type
    return Type::int_type();
}

std::optional<Type> TypeChecker::check_if(const IfExpr& if_expr) {
    auto cond_type = check_expression(*if_expr.condition);
    if (cond_type && !TypeResolver::is_boolean(*cond_type)) {
        report_error("If condition must be boolean", if_expr.condition->span);
    }
    
    symbols_.enter_scope();
    check_block(if_expr.then_block);
    symbols_.exit_scope();
    
    if (!if_expr.else_block.empty()) {
        symbols_.enter_scope();
        check_block(if_expr.else_block);
        symbols_.exit_scope();
    }
    
    return Type::void_type();
}

std::optional<Type> TypeChecker::check_match(const MatchExpr& match) {
    (void)check_expression(*match.scrutinee);
    
    for (const auto& arm : match.arms) {
        (void)check_expression(*arm.pattern);
        symbols_.enter_scope();
        check_block(arm.body);
        symbols_.exit_scope();
    }
    
    return Type::void_type();
}

std::optional<Type> TypeChecker::check_block_expr(const BlockExpr& block) {
    symbols_.enter_scope();
    check_block(block.statements);
    symbols_.exit_scope();
    return Type::void_type();
}

std::optional<Type> TypeChecker::check_array(const ArrayExpr& arr) {
    if (arr.elements.empty()) {
        return Type::void_type(); // Unknown element type
    }
    
    auto first_type = check_expression(*arr.elements[0]);
    for (size_t i = 1; i < arr.elements.size(); ++i) {
        auto elem_type = check_expression(*arr.elements[i]);
        if (first_type && elem_type && !TypeResolver::types_equal(*first_type, *elem_type)) {
            report_error("Mismatched array element types", arr.elements[i]->span);
        }
    }
    
    return first_type;
}

void TypeChecker::report_error(const std::string& message, SourceSpan span) {
    errors_.push_back(
        error("E0003", message)
            .with_label(span, "")
    );
}

void TypeChecker::report_type_error(const Type& expected, const Type& got, SourceSpan span) {
    errors_.push_back(
        error("E0004", "Type mismatch")
            .with_label(span, "expected " + expected.name + ", got " + got.name)
    );
}

void TypeChecker::report_undefined(const std::string& kind, const std::string& name, SourceSpan span) {
    errors_.push_back(
        error("E0005", "Undefined " + kind + ": " + name)
            .with_label(span, "not found in this scope")
    );
}

} // namespace idotc
