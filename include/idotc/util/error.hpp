#ifndef IDOTC_UTIL_ERROR_HPP
#define IDOTC_UTIL_ERROR_HPP

#include "idotc/lexer/token.hpp"
#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <optional>
#include <expected>
#include <iostream>

namespace idotc {

class SourceFile;

/// Severity level for diagnostics
enum class DiagnosticSeverity {
    Error,
    Warning,
    Note,
    Help
};

/// A label pointing to a specific source span
struct DiagnosticLabel {
    SourceSpan span;
    std::string message;
    DiagnosticSeverity severity;
};

/// A diagnostic message (error, warning, note)
class Diagnostic {
public:
    Diagnostic(DiagnosticSeverity severity, std::string code, std::string message);
    
    /// Add a primary label at a source location
    Diagnostic& with_label(SourceSpan span, std::string message);
    
    /// Add a secondary label
    Diagnostic& with_secondary_label(SourceSpan span, std::string message);
    
    /// Add a help/suggestion message
    Diagnostic& with_help(std::string help);
    
    /// Add a note
    Diagnostic& with_note(std::string note);
    
    [[nodiscard]] DiagnosticSeverity severity() const noexcept { return severity_; }
    [[nodiscard]] const std::string& code() const noexcept { return code_; }
    [[nodiscard]] const std::string& message() const noexcept { return message_; }
    [[nodiscard]] const std::vector<DiagnosticLabel>& labels() const noexcept { return labels_; }
    [[nodiscard]] const std::vector<std::string>& helps() const noexcept { return helps_; }
    [[nodiscard]] const std::vector<std::string>& notes() const noexcept { return notes_; }
    
private:
    DiagnosticSeverity severity_;
    std::string code_;
    std::string message_;
    std::vector<DiagnosticLabel> labels_;
    std::vector<std::string> helps_;
    std::vector<std::string> notes_;
};

/// Pretty-print a diagnostic to an output stream
void print_diagnostic(std::ostream& os, const Diagnostic& diag, const SourceFile& source);

/// Error result type using C++23 std::expected
template<typename T>
using Result = std::expected<T, Diagnostic>;

/// Create an error diagnostic
inline Diagnostic error(std::string code, std::string message) {
    return Diagnostic(DiagnosticSeverity::Error, std::move(code), std::move(message));
}

/// Create a warning diagnostic
inline Diagnostic warning(std::string code, std::string message) {
    return Diagnostic(DiagnosticSeverity::Warning, std::move(code), std::move(message));
}

} // namespace idotc

#endif
