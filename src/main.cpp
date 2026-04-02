#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>
#include <filesystem>
#include <chrono>
#include <cstdlib>

#include "idotc/lexer/lexer.hpp"
#include "idotc/parser/parser.hpp"
#include "idotc/semantic/type_checker.hpp"
#include "idotc/util/source_location.hpp"
#include "idotc/util/error.hpp"

#ifdef IDOTC_HAS_LLVM
#include "idotc/codegen/compiler.hpp"
#include "idotc/codegen/ir_builder.hpp"
#endif

namespace fs = std::filesystem;

namespace {

constexpr std::string_view VERSION = "0.1.0";
constexpr std::string_view PROGRAM_NAME = "idotc";

void print_version() {
    std::cout << PROGRAM_NAME << " " << VERSION << "\n";
    std::cout << "Advanced-idot compiler (C++ implementation)\n";
#ifdef IDOTC_HAS_LLVM
    std::cout << "LLVM codegen: enabled\n";
#else
    std::cout << "LLVM codegen: disabled\n";
#endif
}

void print_help() {
    std::cout << "Usage: " << PROGRAM_NAME << " <COMMAND> [OPTIONS]\n\n";
    std::cout << "Commands:\n";
    std::cout << "  build <file>    Compile the source file\n";
    std::cout << "  run <file>      Compile and execute the source file\n";
    std::cout << "  check <file>    Check syntax and types without codegen\n";
    std::cout << "  help            Show this help message\n";
    std::cout << "  version         Show version information\n";
    std::cout << "\n";
    std::cout << "Build Options:\n";
    std::cout << "  -o, --output <file>   Output file name\n";
    std::cout << "  --emit-llvm           Emit LLVM IR instead of object file\n";
    std::cout << "  -O0                   No optimization (default)\n";
    std::cout << "  -O1                   Basic optimization\n";
    std::cout << "  -O2                   Standard optimization\n";
    std::cout << "  -O3                   Aggressive optimization\n";
    std::cout << "  --time                Print compilation timing\n";
    std::cout << "\n";
}

struct BuildOptions {
    std::string input_file;
    std::string output_file;
    bool emit_llvm = false;
    int opt_level = 0;
    bool show_timing = false;
};

std::string read_file(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        std::cerr << "error: could not open file '" << path << "'\n";
        std::exit(1);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void print_diagnostics(const std::vector<idotc::Diagnostic>& diagnostics, const idotc::SourceFile& source) {
    for (const auto& diag : diagnostics) {
        idotc::print_diagnostic(std::cerr, diag, source);
    }
}

int cmd_build(const BuildOptions& opts) {
    using namespace idotc;
    
    if (opts.input_file.empty()) {
        std::cerr << "error: no input file specified\n";
        return 1;
    }
    
    if (!fs::exists(opts.input_file)) {
        std::cerr << "error: file not found: " << opts.input_file << "\n";
        return 1;
    }
    
    auto total_start = std::chrono::high_resolution_clock::now();
    
    std::cout << "Building " << opts.input_file << "...\n";
    
    std::string source_text = read_file(opts.input_file);
    SourceFile source(opts.input_file, source_text);
    
    // Phase 1: Lexing
    auto lex_start = std::chrono::high_resolution_clock::now();
    Lexer lexer(source_text);
    auto tokens = lexer.tokenize();
    auto lex_end = std::chrono::high_resolution_clock::now();
    
    if (lexer.has_errors()) {
        print_diagnostics(lexer.errors(), source);
        return 1;
    }
    
    // Phase 2: Parsing
    auto parse_start = std::chrono::high_resolution_clock::now();
    Parser parser(std::move(tokens));
    auto program_result = parser.parse_program();
    auto parse_end = std::chrono::high_resolution_clock::now();
    
    if (!program_result || parser.has_errors()) {
        print_diagnostics(parser.errors(), source);
        return 1;
    }
    
    // Phase 3: Type checking
    auto check_start = std::chrono::high_resolution_clock::now();
    TypeChecker checker;
    if (!checker.check(*program_result)) {
        print_diagnostics(checker.errors(), source);
        return 1;
    }
    auto check_end = std::chrono::high_resolution_clock::now();
    
#ifdef IDOTC_HAS_LLVM
    // Phase 4: Code generation
    auto codegen_start = std::chrono::high_resolution_clock::now();
    
    // Determine output file
    std::string output = opts.output_file;
    if (output.empty()) {
        fs::path input_path(opts.input_file);
        if (opts.emit_llvm) {
            output = input_path.stem().string() + ".ll";
        } else {
            output = input_path.stem().string() + ".o";
        }
    }
    
    Compiler compiler(fs::path(opts.input_file).stem().string());
    if (!compiler.compile(*program_result)) {
        std::cerr << "error: " << compiler.error() << "\n";
        return 1;
    }
    
    // Optimize
    if (opts.opt_level > 0) {
        compiler.optimize(static_cast<OptLevel>(opts.opt_level));
    }
    
    // Output
    bool success;
    if (opts.emit_llvm) {
        success = compiler.write_ir(output);
    } else {
        success = compiler.write_object(output);
    }
    
    auto codegen_end = std::chrono::high_resolution_clock::now();
    
    if (!success) {
        std::cerr << "error: " << compiler.error() << "\n";
        return 1;
    }
    
    auto total_end = std::chrono::high_resolution_clock::now();
    
    std::cout << "Build successful: " << output << "\n";
    
    if (opts.show_timing) {
        auto lex_ms = std::chrono::duration<double, std::milli>(lex_end - lex_start).count();
        auto parse_ms = std::chrono::duration<double, std::milli>(parse_end - parse_start).count();
        auto check_ms = std::chrono::duration<double, std::milli>(check_end - check_start).count();
        auto codegen_ms = std::chrono::duration<double, std::milli>(codegen_end - codegen_start).count();
        auto total_ms = std::chrono::duration<double, std::milli>(total_end - total_start).count();
        
        std::cout << "\nTiming:\n";
        std::cout << "  Lexing:     " << lex_ms << " ms\n";
        std::cout << "  Parsing:    " << parse_ms << " ms\n";
        std::cout << "  Checking:   " << check_ms << " ms\n";
        std::cout << "  Codegen:    " << codegen_ms << " ms\n";
        std::cout << "  Total:      " << total_ms << " ms\n";
    }
    
    return 0;
#else
    std::cerr << "error: LLVM codegen not available\n";
    std::cerr << "hint: rebuild idotc with LLVM support\n";
    return 1;
#endif
}

int cmd_run(const std::string& /*input_file*/) {
    std::cerr << "error: run command not yet implemented\n";
    std::cerr << "hint: use 'build' to compile, then run the output\n";
    return 1;
}

int cmd_check(const std::string& input_file) {
    using namespace idotc;
    
    if (!fs::exists(input_file)) {
        std::cerr << "error: file not found: " << input_file << "\n";
        return 1;
    }
    
    std::cout << "Checking " << input_file << "...\n";
    
    std::string source_text = read_file(input_file);
    SourceFile source(input_file, source_text);
    
    // Phase 1: Lexing
    Lexer lexer(source_text);
    auto tokens = lexer.tokenize();
    
    if (lexer.has_errors()) {
        print_diagnostics(lexer.errors(), source);
        return 1;
    }
    std::cout << "  Lexed " << tokens.size() << " tokens\n";
    
    // Phase 2: Parsing
    Parser parser(std::move(tokens));
    auto program_result = parser.parse_program();
    
    if (!program_result || parser.has_errors()) {
        print_diagnostics(parser.errors(), source);
        return 1;
    }
    std::cout << "  Parsed " << program_result->items.size() << " items\n";
    
    // Phase 3: Type checking
    TypeChecker checker;
    if (!checker.check(*program_result)) {
        print_diagnostics(checker.errors(), source);
        return 1;
    }
    
    std::cout << "Check passed! No errors found.\n";
    return 0;
}

} // anonymous namespace

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_help();
        return 0;
    }
    
    std::string_view command = argv[1];
    
    if (command == "help" || command == "-h" || command == "--help") {
        print_help();
        return 0;
    }
    
    if (command == "version" || command == "-v" || command == "--version") {
        print_version();
        return 0;
    }
    
    if (command == "build") {
        BuildOptions opts;
        
        for (int i = 2; i < argc; ++i) {
            std::string_view arg = argv[i];
            
            if (arg == "-o" || arg == "--output") {
                if (i + 1 >= argc) {
                    std::cerr << "error: -o requires an argument\n";
                    return 1;
                }
                opts.output_file = argv[++i];
            } else if (arg == "--emit-llvm") {
                opts.emit_llvm = true;
            } else if (arg == "-O0") {
                opts.opt_level = 0;
            } else if (arg == "-O1") {
                opts.opt_level = 1;
            } else if (arg == "-O2") {
                opts.opt_level = 2;
            } else if (arg == "-O3" || arg == "-O" || arg == "--optimize") {
                opts.opt_level = 3;
            } else if (arg == "--time") {
                opts.show_timing = true;
            } else if (arg.starts_with("-")) {
                std::cerr << "error: unknown option: " << arg << "\n";
                return 1;
            } else {
                if (opts.input_file.empty()) {
                    opts.input_file = std::string(arg);
                } else {
                    std::cerr << "error: multiple input files not supported\n";
                    return 1;
                }
            }
        }
        
        return cmd_build(opts);
    }
    
    if (command == "run") {
        if (argc < 3) {
            std::cerr << "error: no input file specified\n";
            return 1;
        }
        return cmd_run(argv[2]);
    }
    
    if (command == "check") {
        if (argc < 3) {
            std::cerr << "error: no input file specified\n";
            return 1;
        }
        return cmd_check(argv[2]);
    }
    
    std::cerr << "error: unknown command: " << command << "\n";
    std::cerr << "run '" << PROGRAM_NAME << " help' for usage\n";
    return 1;
}
