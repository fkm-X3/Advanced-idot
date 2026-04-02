#ifndef IDOTC_CODEGEN_COMPILER_HPP
#define IDOTC_CODEGEN_COMPILER_HPP

#include "idotc/parser/ast.hpp"
#include <string>
#include <memory>
#include <filesystem>

#ifdef IDOTC_HAS_LLVM
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/Target/TargetMachine.h>
#endif

namespace idotc {

/// Optimization level
enum class OptLevel {
    O0 = 0,
    O1 = 1,
    O2 = 2,
    O3 = 3
};

#ifdef IDOTC_HAS_LLVM

/// LLVM-based code generator
class Compiler {
public:
    /// Create a new compiler for a module
    explicit Compiler(std::string module_name);
    
    ~Compiler();
    
    // Non-copyable, movable
    Compiler(const Compiler&) = delete;
    Compiler& operator=(const Compiler&) = delete;
    Compiler(Compiler&&) noexcept;
    Compiler& operator=(Compiler&&) noexcept;
    
    /// Compile a program to LLVM IR
    bool compile(const Program& program);
    
    /// Run optimization passes
    void optimize(OptLevel level = OptLevel::O3);
    
    /// Write LLVM IR to a text file (.ll)
    bool write_ir(const std::filesystem::path& path) const;
    
    /// Write object file (.o)
    bool write_object(const std::filesystem::path& path) const;
    
    /// Write bitcode file (.bc)
    bool write_bitcode(const std::filesystem::path& path) const;
    
    /// Get the LLVM module (for testing/inspection)
    [[nodiscard]] llvm::Module& module() { return *module_; }
    [[nodiscard]] const llvm::Module& module() const { return *module_; }
    
    /// Get the last error message
    [[nodiscard]] const std::string& error() const { return error_; }
    
private:
    std::unique_ptr<llvm::LLVMContext> context_;
    std::unique_ptr<llvm::Module> module_;
    std::unique_ptr<llvm::IRBuilder<>> builder_;
    std::unique_ptr<llvm::TargetMachine> target_machine_;
    std::string error_;
    
    bool initialize_target();
};

#else

/// Stub compiler when LLVM is not available
class Compiler {
public:
    explicit Compiler(std::string_view /*module_name*/) {
        error_ = "LLVM support not compiled in";
    }
    
    bool compile(const Program&) { return false; }
    void optimize(OptLevel = OptLevel::O3) {}
    bool write_ir(const std::filesystem::path&) const { return false; }
    bool write_object(const std::filesystem::path&) const { return false; }
    bool write_bitcode(const std::filesystem::path&) const { return false; }
    
    [[nodiscard]] const std::string& error() const { return error_; }
    
private:
    std::string error_;
};

#endif // IDOTC_HAS_LLVM

} // namespace idotc

#endif // IDOTC_CODEGEN_COMPILER_HPP
