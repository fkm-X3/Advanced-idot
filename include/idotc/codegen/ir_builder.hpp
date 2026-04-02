#ifndef IDOTC_CODEGEN_IR_BUILDER_HPP
#define IDOTC_CODEGEN_IR_BUILDER_HPP

#include "idotc/parser/ast.hpp"
#include "idotc/semantic/symbol_table.hpp"
#include <string>
#include <unordered_map>

#ifdef IDOTC_HAS_LLVM
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#endif

namespace idotc {

#ifdef IDOTC_HAS_LLVM

/// Maps Advanced-idot types to LLVM types and generates IR
class IRGenerator {
public:
    IRGenerator(llvm::LLVMContext& context, llvm::Module& module, llvm::IRBuilder<>& builder);
    
    // Type conversion
    [[nodiscard]] llvm::Type* get_llvm_type(const Type& type);
    [[nodiscard]] llvm::Type* get_int_type();
    [[nodiscard]] llvm::Type* get_float_type();
    [[nodiscard]] llvm::Type* get_bool_type();
    [[nodiscard]] llvm::Type* get_char_type();
    [[nodiscard]] llvm::Type* get_string_type();
    [[nodiscard]] llvm::Type* get_void_type();
    
    // Struct type registration
    llvm::StructType* create_struct_type(const std::string& name, const std::vector<Field>& fields);
    [[nodiscard]] llvm::StructType* get_struct_type(const std::string& name);
    
    // Expression codegen
    [[nodiscard]] llvm::Value* codegen_expression(const Expression& expr);
    [[nodiscard]] llvm::Value* codegen_literal(const LiteralExpr& lit);
    [[nodiscard]] llvm::Value* codegen_variable(const VariableExpr& var);
    [[nodiscard]] llvm::Value* codegen_binary(const BinaryExpr& bin);
    [[nodiscard]] llvm::Value* codegen_unary(const UnaryExpr& un);
    [[nodiscard]] llvm::Value* codegen_call(const CallExpr& call);
    [[nodiscard]] llvm::Value* codegen_member(const MemberExpr& member);
    [[nodiscard]] llvm::Value* codegen_index(const IndexExpr& index);
    
    // Statement codegen
    void codegen_statement(const Statement& stmt);
    void codegen_let(const LetStmt& let);
    void codegen_return(const ReturnStmt& ret);
    void codegen_while(const WhileStmt& loop);
    void codegen_for(const ForStmt& loop);
    void codegen_assign(const AssignStmt& assign);
    void codegen_block(const std::vector<StmtPtr>& stmts);
    
    // Function codegen
    llvm::Function* codegen_function(const FunctionItem& fn);
    llvm::Function* declare_function(const FunctionItem& fn);
    
    // Variable management
    void push_scope();
    void pop_scope();
    void define_variable(const std::string& name, llvm::AllocaInst* alloca);
    [[nodiscard]] llvm::AllocaInst* lookup_variable(const std::string& name);
    
    // Error access
    [[nodiscard]] const std::string& error() const { return error_; }
    [[nodiscard]] bool has_error() const { return !error_.empty(); }
    
private:
    llvm::LLVMContext& context_;
    llvm::Module& module_;
    llvm::IRBuilder<>& builder_;
    
    std::unordered_map<std::string, llvm::StructType*> struct_types_;
    std::vector<std::unordered_map<std::string, llvm::AllocaInst*>> scopes_;
    std::unordered_map<std::string, llvm::Function*> functions_;
    
    std::string error_;
    
    llvm::AllocaInst* create_entry_block_alloca(llvm::Function* fn, llvm::Type* type, const std::string& name);
};

#endif // IDOTC_HAS_LLVM

} // namespace idotc

#endif // IDOTC_CODEGEN_IR_BUILDER_HPP
