#ifndef IDOTC_UTIL_SOURCE_LOCATION_HPP
#define IDOTC_UTIL_SOURCE_LOCATION_HPP

#include <string>
#include <string_view>
#include <vector>
#include <memory>

namespace idotc {

/// A source file with its content
class SourceFile {
public:
    SourceFile(std::string name, std::string content);
    
    [[nodiscard]] std::string_view name() const noexcept { return name_; }
    [[nodiscard]] std::string_view content() const noexcept { return content_; }
    
    /// Get the line containing the given byte offset
    [[nodiscard]] std::string_view get_line(std::size_t line_number) const;
    
    /// Get line number for a byte offset (1-indexed)
    [[nodiscard]] std::size_t get_line_number(std::size_t byte_offset) const;
    
    /// Get column for a byte offset (1-indexed)
    [[nodiscard]] std::size_t get_column(std::size_t byte_offset) const;
    
private:
    std::string name_;
    std::string content_;
    std::vector<std::size_t> line_starts_; // Byte offset of each line start
    
    void compute_line_starts();
};

} // namespace idotc

#endif
