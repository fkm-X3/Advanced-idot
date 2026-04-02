#include "idotc/codegen/ir_builder.hpp"

#ifdef IDOTC_HAS_LLVM

namespace idotc {

IRGenerator::IRGenerator(llvm::LLVMContext& context, llvm::Module& module, llvm::IRBuilder<>& builder)
    : context_(context)
    , module_(module)
    , builder_(builder)
{
    scopes_.emplace_back(); // Global scope
}

llvm::Type* IRGenerator::get_llvm_type(const Type& type) {
    if (type.name == "int") return get_int_type();
    if (type.name == "float") return get_float_type();
    if (type.name == "bool") return get_bool_type();
    if (type.name == "char") return get_char_type();
    if (type.name == "string") return get_string_type();
    if (type.name == "void") return get_void_type();
    
    // Check for struct type
    if (auto* st = get_struct_type(type.name)) {
        if (type.is_reference) {
            return st->getPointerTo();
        }
        return st;
    }
    
    // Unknown type, default to i64
    return llvm::Type::getInt64Ty(context_);
}

llvm::Type* IRGenerator::get_int_type() {
    return llvm::Type::getInt64Ty(context_);
}

llvm::Type* IRGenerator::get_float_type() {
    return llvm::Type::getDoubleTy(context_);
}

llvm::Type* IRGenerator::get_bool_type() {
    return llvm::Type::getInt1Ty(context_);
}

llvm::Type* IRGenerator::get_char_type() {
    return llvm::Type::getInt8Ty(context_);
}

llvm::Type* IRGenerator::get_string_type() {
    // String as pointer to i8 (C-style string)
    return llvm::PointerType::get(llvm::Type::getInt8Ty(context_), 0);
}

llvm::Type* IRGenerator::get_void_type() {
    return llvm::Type::getVoidTy(context_);
}

llvm::StructType* IRGenerator::create_struct_type(const std::string& name, const std::vector<Field>& fields) {
    std::vector<llvm::Type*> member_types;
    for (const auto& field : fields) {
        member_types.push_back(get_llvm_type(field.type));
    }
    
    auto* struct_type = llvm::StructType::create(context_, member_types, name);
    struct_types_[name] = struct_type;
    return struct_type;
}

llvm::StructType* IRGenerator::get_struct_type(const std::string& name) {
    auto it = struct_types_.find(name);
    return it != struct_types_.end() ? it->second : nullptr;
}

void IRGenerator::push_scope() {
    scopes_.emplace_back();
}

void IRGenerator::pop_scope() {
    if (scopes_.size() > 1) {
        scopes_.pop_back();
    }
}

void IRGenerator::define_variable(const std::string& name, llvm::AllocaInst* alloca) {
    scopes_.back()[name] = alloca;
}

llvm::AllocaInst* IRGenerator::lookup_variable(const std::string& name) {
    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
        auto var_it = it->find(name);
        if (var_it != it->end()) {
            return var_it->second;
        }
    }
    return nullptr;
}

llvm::AllocaInst* IRGenerator::create_entry_block_alloca(llvm::Function* fn, llvm::Type* type, const std::string& name) {
    llvm::IRBuilder<> tmp_builder(&fn->getEntryBlock(), fn->getEntryBlock().begin());
    return tmp_builder.CreateAlloca(type, nullptr, name);
}

llvm::Value* IRGenerator::codegen_expression(const Expression& expr) {
    return std::visit([this](const auto& e) -> llvm::Value* {
        using T = std::decay_t<decltype(e)>;
        if constexpr (std::is_same_v<T, LiteralExpr>) {
            return codegen_literal(e);
        } else if constexpr (std::is_same_v<T, VariableExpr>) {
            return codegen_variable(e);
        } else if constexpr (std::is_same_v<T, BinaryExpr>) {
            return codegen_binary(e);
        } else if constexpr (std::is_same_v<T, UnaryExpr>) {
            return codegen_unary(e);
        } else if constexpr (std::is_same_v<T, CallExpr>) {
            return codegen_call(e);
        } else if constexpr (std::is_same_v<T, MemberExpr>) {
            return codegen_member(e);
        } else if constexpr (std::is_same_v<T, IndexExpr>) {
            return codegen_index(e);
        } else {
            error_ = "Unsupported expression type";
            return nullptr;
        }
    }, expr.variant);
}

llvm::Value* IRGenerator::codegen_literal(const LiteralExpr& lit) {
    return std::visit([this](const auto& v) -> llvm::Value* {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::int64_t>) {
            return llvm::ConstantInt::get(get_int_type(), v, true);
        } else if constexpr (std::is_same_v<T, double>) {
            return llvm::ConstantFP::get(get_float_type(), v);
        } else if constexpr (std::is_same_v<T, bool>) {
            return llvm::ConstantInt::get(get_bool_type(), v ? 1 : 0);
        } else if constexpr (std::is_same_v<T, char>) {
            return llvm::ConstantInt::get(get_char_type(), static_cast<uint8_t>(v));
        } else if constexpr (std::is_same_v<T, std::string>) {
            return builder_.CreateGlobalStringPtr(v);
        } else {
            return llvm::ConstantInt::get(get_int_type(), 0);
        }
    }, lit.value);
}

llvm::Value* IRGenerator::codegen_variable(const VariableExpr& var) {
    auto* alloca = lookup_variable(var.name);
    if (!alloca) {
        error_ = "Undefined variable: " + var.name;
        return nullptr;
    }
    return builder_.CreateLoad(alloca->getAllocatedType(), alloca, var.name);
}

llvm::Value* IRGenerator::codegen_binary(const BinaryExpr& bin) {
    auto* left = codegen_expression(*bin.left);
    auto* right = codegen_expression(*bin.right);
    if (!left || !right) return nullptr;
    
    bool is_float = left->getType()->isDoubleTy();
    
    switch (bin.op) {
        case BinaryOp::Add:
            return is_float ? builder_.CreateFAdd(left, right, "addtmp")
                           : builder_.CreateAdd(left, right, "addtmp");
        case BinaryOp::Sub:
            return is_float ? builder_.CreateFSub(left, right, "subtmp")
                           : builder_.CreateSub(left, right, "subtmp");
        case BinaryOp::Mul:
            return is_float ? builder_.CreateFMul(left, right, "multmp")
                           : builder_.CreateMul(left, right, "multmp");
        case BinaryOp::Div:
            return is_float ? builder_.CreateFDiv(left, right, "divtmp")
                           : builder_.CreateSDiv(left, right, "divtmp");
        case BinaryOp::Mod:
            return builder_.CreateSRem(left, right, "modtmp");
        case BinaryOp::Eq:
            return is_float ? builder_.CreateFCmpOEQ(left, right, "eqtmp")
                           : builder_.CreateICmpEQ(left, right, "eqtmp");
        case BinaryOp::NotEq:
            return is_float ? builder_.CreateFCmpONE(left, right, "netmp")
                           : builder_.CreateICmpNE(left, right, "netmp");
        case BinaryOp::Lt:
            return is_float ? builder_.CreateFCmpOLT(left, right, "lttmp")
                           : builder_.CreateICmpSLT(left, right, "lttmp");
        case BinaryOp::LtEq:
            return is_float ? builder_.CreateFCmpOLE(left, right, "letmp")
                           : builder_.CreateICmpSLE(left, right, "letmp");
        case BinaryOp::Gt:
            return is_float ? builder_.CreateFCmpOGT(left, right, "gttmp")
                           : builder_.CreateICmpSGT(left, right, "gttmp");
        case BinaryOp::GtEq:
            return is_float ? builder_.CreateFCmpOGE(left, right, "getmp")
                           : builder_.CreateICmpSGE(left, right, "getmp");
        case BinaryOp::And:
            return builder_.CreateAnd(left, right, "andtmp");
        case BinaryOp::Or:
            return builder_.CreateOr(left, right, "ortmp");
        case BinaryOp::BitAnd:
            return builder_.CreateAnd(left, right, "bandtmp");
        case BinaryOp::BitOr:
            return builder_.CreateOr(left, right, "bortmp");
        case BinaryOp::BitXor:
            return builder_.CreateXor(left, right, "xortmp");
        case BinaryOp::Shl:
            return builder_.CreateShl(left, right, "shltmp");
        case BinaryOp::Shr:
            return builder_.CreateAShr(left, right, "shrtmp");
    }
    return nullptr;
}

llvm::Value* IRGenerator::codegen_unary(const UnaryExpr& un) {
    auto* operand = codegen_expression(*un.operand);
    if (!operand) return nullptr;
    
    switch (un.op) {
        case UnaryOp::Neg:
            return operand->getType()->isDoubleTy() 
                ? builder_.CreateFNeg(operand, "negtmp")
                : builder_.CreateNeg(operand, "negtmp");
        case UnaryOp::Not:
            return builder_.CreateNot(operand, "nottmp");
        case UnaryOp::BitNot:
            return builder_.CreateNot(operand, "bnottmp");
        default:
            error_ = "Unsupported unary operator";
            return nullptr;
    }
}

llvm::Value* IRGenerator::codegen_call(const CallExpr& call) {
    auto* callee = module_.getFunction(call.name);
    if (!callee) {
        error_ = "Unknown function: " + call.name;
        return nullptr;
    }
    
    std::vector<llvm::Value*> args;
    for (const auto& arg : call.args) {
        auto* val = codegen_expression(*arg);
        if (!val) return nullptr;
        args.push_back(val);
    }
    
    return builder_.CreateCall(callee, args, "calltmp");
}

llvm::Value* IRGenerator::codegen_member(const MemberExpr& member) {
    // TODO: Implement struct member access
    error_ = "Member access not yet implemented";
    return nullptr;
}

llvm::Value* IRGenerator::codegen_index(const IndexExpr& index) {
    // TODO: Implement array indexing
    error_ = "Array indexing not yet implemented";
    return nullptr;
}

void IRGenerator::codegen_statement(const Statement& stmt) {
    std::visit([this](const auto& s) {
        using T = std::decay_t<decltype(s)>;
        if constexpr (std::is_same_v<T, LetStmt>) {
            codegen_let(s);
        } else if constexpr (std::is_same_v<T, ReturnStmt>) {
            codegen_return(s);
        } else if constexpr (std::is_same_v<T, WhileStmt>) {
            codegen_while(s);
        } else if constexpr (std::is_same_v<T, ForStmt>) {
            codegen_for(s);
        } else if constexpr (std::is_same_v<T, AssignStmt>) {
            codegen_assign(s);
        } else if constexpr (std::is_same_v<T, ExprStmt>) {
            codegen_expression(*s.expression);
        }
    }, stmt.variant);
}

void IRGenerator::codegen_let(const LetStmt& let) {
    auto* fn = builder_.GetInsertBlock()->getParent();
    llvm::Type* type = let.type ? get_llvm_type(*let.type) : get_int_type();
    
    auto* alloca = create_entry_block_alloca(fn, type, let.name);
    
    if (let.initializer) {
        auto* init_val = codegen_expression(*let.initializer);
        if (init_val) {
            builder_.CreateStore(init_val, alloca);
        }
    }
    
    define_variable(let.name, alloca);
}

void IRGenerator::codegen_return(const ReturnStmt& ret) {
    if (ret.value) {
        auto* val = codegen_expression(**ret.value);
        if (val) {
            builder_.CreateRet(val);
        }
    } else {
        builder_.CreateRetVoid();
    }
}

void IRGenerator::codegen_while(const WhileStmt& loop) {
    auto* fn = builder_.GetInsertBlock()->getParent();
    
    auto* cond_bb = llvm::BasicBlock::Create(context_, "while.cond", fn);
    auto* body_bb = llvm::BasicBlock::Create(context_, "while.body", fn);
    auto* end_bb = llvm::BasicBlock::Create(context_, "while.end", fn);
    
    builder_.CreateBr(cond_bb);
    builder_.SetInsertPoint(cond_bb);
    
    auto* cond_val = codegen_expression(*loop.condition);
    builder_.CreateCondBr(cond_val, body_bb, end_bb);
    
    builder_.SetInsertPoint(body_bb);
    push_scope();
    codegen_block(loop.body);
    pop_scope();
    builder_.CreateBr(cond_bb);
    
    builder_.SetInsertPoint(end_bb);
}

void IRGenerator::codegen_for(const ForStmt& loop) {
    // TODO: Implement for loop codegen
}

void IRGenerator::codegen_assign(const AssignStmt& assign) {
    auto* val = codegen_expression(*assign.value);
    if (!val) return;
    
    if (auto* var = std::get_if<VariableExpr>(&assign.target->variant)) {
        auto* alloca = lookup_variable(var->name);
        if (alloca) {
            builder_.CreateStore(val, alloca);
        }
    }
}

void IRGenerator::codegen_block(const std::vector<StmtPtr>& stmts) {
    for (const auto& stmt : stmts) {
        codegen_statement(*stmt);
    }
}

llvm::Function* IRGenerator::declare_function(const FunctionItem& fn) {
    std::vector<llvm::Type*> param_types;
    for (const auto& param : fn.params) {
        param_types.push_back(get_llvm_type(param.type));
    }
    
    llvm::Type* ret_type = fn.return_type ? get_llvm_type(*fn.return_type) : get_void_type();
    auto* fn_type = llvm::FunctionType::get(ret_type, param_types, false);
    
    auto* func = llvm::Function::Create(fn_type, llvm::Function::ExternalLinkage, fn.name, module_);
    
    // Set parameter names
    size_t i = 0;
    for (auto& arg : func->args()) {
        arg.setName(fn.params[i++].name);
    }
    
    functions_[fn.name] = func;
    return func;
}

llvm::Function* IRGenerator::codegen_function(const FunctionItem& fn) {
    auto* func = functions_[fn.name];
    if (!func) {
        func = declare_function(fn);
    }
    
    auto* entry = llvm::BasicBlock::Create(context_, "entry", func);
    builder_.SetInsertPoint(entry);
    
    push_scope();
    
    // Create allocas for parameters
    for (auto& arg : func->args()) {
        auto* alloca = create_entry_block_alloca(func, arg.getType(), std::string(arg.getName()));
        builder_.CreateStore(&arg, alloca);
        define_variable(std::string(arg.getName()), alloca);
    }
    
    codegen_block(fn.body);
    
    // Add implicit return if needed
    if (!builder_.GetInsertBlock()->getTerminator()) {
        if (func->getReturnType()->isVoidTy()) {
            builder_.CreateRetVoid();
        } else {
            builder_.CreateRet(llvm::ConstantInt::get(func->getReturnType(), 0));
        }
    }
    
    pop_scope();
    return func;
}

} // namespace idotc

#endif // IDOTC_HAS_LLVM
