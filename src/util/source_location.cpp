#include "idotc/util/source_location.hpp"
#include <algorithm>

namespace idotc {

SourceFile::SourceFile(std::string name, std::string content)
    : name_(std::move(name))
    , content_(std::move(content))
{
    compute_line_starts();
}

void SourceFile::compute_line_starts() {
    line_starts_.push_back(0); // Line 1 starts at offset 0
    for (std::size_t i = 0; i < content_.size(); ++i) {
        if (content_[i] == '\n') {
            line_starts_.push_back(i + 1);
        }
    }
}

std::string_view SourceFile::get_line(std::size_t line_number) const {
    if (line_number == 0 || line_number > line_starts_.size()) {
        return {};
    }
    
    std::size_t start = line_starts_[line_number - 1];
    std::size_t end;
    
    if (line_number < line_starts_.size()) {
        end = line_starts_[line_number] - 1; // Exclude newline
    } else {
        end = content_.size();
    }
    
    // Handle \r\n
    if (end > start && content_[end - 1] == '\r') {
        --end;
    }
    
    return std::string_view(content_).substr(start, end - start);
}

std::size_t SourceFile::get_line_number(std::size_t byte_offset) const {
    auto it = std::upper_bound(line_starts_.begin(), line_starts_.end(), byte_offset);
    return static_cast<std::size_t>(std::distance(line_starts_.begin(), it));
}

std::size_t SourceFile::get_column(std::size_t byte_offset) const {
    std::size_t line = get_line_number(byte_offset);
    if (line == 0) return 1;
    return byte_offset - line_starts_[line - 1] + 1;
}

} // namespace idotc
