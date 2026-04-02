#include "idotc/util/error.hpp"
#include "idotc/util/source_location.hpp"
#include <format>
#include <iomanip>

namespace idotc {

Diagnostic::Diagnostic(DiagnosticSeverity severity, std::string code, std::string message)
    : severity_(severity)
    , code_(std::move(code))
    , message_(std::move(message))
{}

Diagnostic& Diagnostic::with_label(SourceSpan span, std::string message) {
    labels_.push_back({span, std::move(message), severity_});
    return *this;
}

Diagnostic& Diagnostic::with_secondary_label(SourceSpan span, std::string message) {
    labels_.push_back({span, std::move(message), DiagnosticSeverity::Note});
    return *this;
}

Diagnostic& Diagnostic::with_help(std::string help) {
    helps_.push_back(std::move(help));
    return *this;
}

Diagnostic& Diagnostic::with_note(std::string note) {
    notes_.push_back(std::move(note));
    return *this;
}

namespace {
    const char* severity_color(DiagnosticSeverity sev) {
        switch (sev) {
            case DiagnosticSeverity::Error:   return "\033[1;31m"; // Bold red
            case DiagnosticSeverity::Warning: return "\033[1;33m"; // Bold yellow
            case DiagnosticSeverity::Note:    return "\033[1;36m"; // Bold cyan
            case DiagnosticSeverity::Help:    return "\033[1;32m"; // Bold green
        }
        return "";
    }
    
    const char* severity_name(DiagnosticSeverity sev) {
        switch (sev) {
            case DiagnosticSeverity::Error:   return "error";
            case DiagnosticSeverity::Warning: return "warning";
            case DiagnosticSeverity::Note:    return "note";
            case DiagnosticSeverity::Help:    return "help";
        }
        return "";
    }
    
    constexpr const char* RESET = "\033[0m";
    constexpr const char* BOLD = "\033[1m";
    constexpr const char* BLUE = "\033[1;34m";
}

void print_diagnostic(std::ostream& os, const Diagnostic& diag, const SourceFile& source) {
    // Header: error[E001]: message
    os << severity_color(diag.severity()) << severity_name(diag.severity());
    if (!diag.code().empty()) {
        os << "[" << diag.code() << "]";
    }
    os << RESET << BOLD << ": " << diag.message() << RESET << "\n";
    
    // Print labeled source snippets
    for (const auto& label : diag.labels()) {
        std::size_t line_num = label.span.line;
        std::string_view line = source.get_line(line_num);
        std::size_t col = label.span.column;
        std::size_t len = label.span.length();
        
        // Location line
        os << BLUE << "  --> " << RESET << source.name() << ":" 
           << line_num << ":" << col << "\n";
        
        // Line number gutter width
        int gutter_width = static_cast<int>(std::to_string(line_num).size());
        
        // Empty line before
        os << std::string(gutter_width + 1, ' ') << BLUE << "│" << RESET << "\n";
        
        // Source line
        os << BLUE << std::setw(gutter_width) << line_num << " │ " << RESET << line << "\n";
        
        // Underline
        os << std::string(gutter_width + 1, ' ') << BLUE << "│ " << RESET;
        os << std::string(col - 1, ' ');
        os << severity_color(label.severity);
        os << std::string(std::max(len, std::size_t{1}), '^');
        if (!label.message.empty()) {
            os << " " << label.message;
        }
        os << RESET << "\n";
    }
    
    // Print notes
    for (const auto& note : diag.notes()) {
        os << BLUE << "  = " << RESET << BOLD << "note" << RESET << ": " << note << "\n";
    }
    
    // Print help
    for (const auto& help : diag.helps()) {
        os << severity_color(DiagnosticSeverity::Help) << "  = help" << RESET << ": " << help << "\n";
    }
    
    os << "\n";
}

} // namespace idotc
