#include "idotc/codegen/compiler.hpp"

#ifdef IDOTC_HAS_LLVM
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Bitcode/BitcodeWriter.h>
#endif

namespace idotc {

#ifdef IDOTC_HAS_LLVM

Compiler::Compiler(std::string module_name)
    : context_(std::make_unique<llvm::LLVMContext>())
    , module_(std::make_unique<llvm::Module>(module_name, *context_))
    , builder_(std::make_unique<llvm::IRBuilder<>>(*context_))
{
    if (!initialize_target()) {
        // Error already set
    }
}

Compiler::~Compiler() = default;

Compiler::Compiler(Compiler&&) noexcept = default;
Compiler& Compiler::operator=(Compiler&&) noexcept = default;

bool Compiler::initialize_target() {
    // Initialize LLVM targets (only needs to happen once globally)
    static bool initialized = []() {
        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmParser();
        llvm::InitializeNativeTargetAsmPrinter();
        return true;
    }();
    (void)initialized;
    
    // Get the target triple for the current machine
    auto target_triple = llvm::sys::getDefaultTargetTriple();
    module_->setTargetTriple(target_triple);
    
    // Look up the target
    std::string error_str;
    auto target = llvm::TargetRegistry::lookupTarget(target_triple, error_str);
    if (!target) {
        error_ = "Failed to lookup target: " + error_str;
        return false;
    }
    
    // Create the target machine
    auto cpu = "generic";
    auto features = "";
    llvm::TargetOptions opt;
    target_machine_.reset(target->createTargetMachine(
        target_triple, cpu, features, opt, llvm::Reloc::PIC_));
    
    if (!target_machine_) {
        error_ = "Failed to create target machine";
        return false;
    }
    
    module_->setDataLayout(target_machine_->createDataLayout());
    return true;
}

bool Compiler::compile(const Program& program) {
    // TODO: Implement full compilation
    // For now, create a simple main function that returns 0
    
    auto* int32_ty = llvm::Type::getInt32Ty(*context_);
    auto* main_ty = llvm::FunctionType::get(int32_ty, false);
    auto* main_fn = llvm::Function::Create(
        main_ty, llvm::Function::ExternalLinkage, "main", module_.get());
    
    auto* entry = llvm::BasicBlock::Create(*context_, "entry", main_fn);
    builder_->SetInsertPoint(entry);
    builder_->CreateRet(llvm::ConstantInt::get(int32_ty, 0));
    
    // Verify the module
    std::string verify_error;
    llvm::raw_string_ostream verify_stream(verify_error);
    if (llvm::verifyModule(*module_, &verify_stream)) {
        error_ = "Module verification failed: " + verify_error;
        return false;
    }
    
    return true;
}

void Compiler::optimize(OptLevel level) {
    if (level == OptLevel::O0) {
        return; // No optimization
    }
    
    // Create the pass builder with optimization level
    llvm::PassBuilder pb(target_machine_.get());
    
    llvm::LoopAnalysisManager lam;
    llvm::FunctionAnalysisManager fam;
    llvm::CGSCCAnalysisManager cgam;
    llvm::ModuleAnalysisManager mam;
    
    pb.registerModuleAnalyses(mam);
    pb.registerCGSCCAnalyses(cgam);
    pb.registerFunctionAnalyses(fam);
    pb.registerLoopAnalyses(lam);
    pb.crossRegisterProxies(lam, fam, cgam, mam);
    
    // Build the optimization pipeline
    llvm::OptimizationLevel opt_level;
    switch (level) {
        case OptLevel::O1: opt_level = llvm::OptimizationLevel::O1; break;
        case OptLevel::O2: opt_level = llvm::OptimizationLevel::O2; break;
        case OptLevel::O3: opt_level = llvm::OptimizationLevel::O3; break;
        default: return;
    }
    
    auto mpm = pb.buildPerModuleDefaultPipeline(opt_level);
    mpm.run(*module_, mam);
}

bool Compiler::write_ir(const std::filesystem::path& path) const {
    std::error_code ec;
    llvm::raw_fd_ostream os(path.string(), ec, llvm::sys::fs::OF_Text);
    if (ec) {
        const_cast<Compiler*>(this)->error_ = "Failed to open file: " + ec.message();
        return false;
    }
    module_->print(os, nullptr);
    return true;
}

bool Compiler::write_object(const std::filesystem::path& path) const {
    std::error_code ec;
    llvm::raw_fd_ostream os(path.string(), ec, llvm::sys::fs::OF_None);
    if (ec) {
        const_cast<Compiler*>(this)->error_ = "Failed to open file: " + ec.message();
        return false;
    }
    
    llvm::legacy::PassManager pass;
    if (target_machine_->addPassesToEmitFile(pass, os, nullptr,
                                              llvm::CodeGenFileType::ObjectFile)) {
        const_cast<Compiler*>(this)->error_ = "Target doesn't support object file emission";
        return false;
    }
    
    pass.run(*module_);
    return true;
}

bool Compiler::write_bitcode(const std::filesystem::path& path) const {
    std::error_code ec;
    llvm::raw_fd_ostream os(path.string(), ec, llvm::sys::fs::OF_None);
    if (ec) {
        const_cast<Compiler*>(this)->error_ = "Failed to open file: " + ec.message();
        return false;
    }
    llvm::WriteBitcodeToFile(*module_, os);
    return true;
}

#endif // IDOTC_HAS_LLVM

} // namespace idotc
