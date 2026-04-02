#include <catch2/catch_test_macros.hpp>
#include "idotc/lexer/token.hpp"
#include "idotc/lexer/lexer.hpp"

using namespace idotc;

// ============================================================================
// TOKEN AND STRUCTURE TESTS
// ============================================================================

TEST_CASE("TokenKind names are correct", "[lexer][token]") {
    CHECK(token_kind_name(TokenKind::Eof) == "EOF");
    CHECK(token_kind_name(TokenKind::Fn) == "fn");
    CHECK(token_kind_name(TokenKind::Let) == "let");
    CHECK(token_kind_name(TokenKind::Plus) == "+");
    CHECK(token_kind_name(TokenKind::EqEq) == "==");
    CHECK(token_kind_name(TokenKind::Arrow) == "->");
}

TEST_CASE("Token classification", "[lexer][token]") {
    SECTION("Keywords are identified") {
        Token tok{TokenKind::Fn, {}, {}, {}};
        CHECK(tok.is_keyword());
        CHECK_FALSE(tok.is_literal());
        CHECK_FALSE(tok.is_operator());
    }
    
    SECTION("Literals are identified") {
        Token tok{TokenKind::IntLiteral, {}, {}, std::int64_t{42}};
        CHECK(tok.is_literal());
        CHECK_FALSE(tok.is_keyword());
        CHECK_FALSE(tok.is_operator());
    }
    
    SECTION("Operators are identified") {
        Token tok{TokenKind::Plus, {}, {}, {}};
        CHECK(tok.is_operator());
        CHECK_FALSE(tok.is_keyword());
        CHECK_FALSE(tok.is_literal());
    }
    
    SECTION("Delimiters are identified") {
        Token tok{TokenKind::LParen, {}, {}, {}};
        CHECK(tok.is_delimiter());
        CHECK_FALSE(tok.is_operator());
    }
    
    SECTION("EOF is identified") {
        Token tok{TokenKind::Eof, {}, {}, {}};
        CHECK(tok.is_eof());
        CHECK_FALSE(tok.is_keyword());
    }
    
    SECTION("Error tokens are identified") {
        Token tok{TokenKind::Error, {}, {}, {}};
        CHECK(tok.is_error());
    }
}

TEST_CASE("SourceSpan operations", "[lexer][span]") {
    SourceSpan span{10, 15, 1, 11};
    CHECK(span.length() == 5);
    CHECK(span.start == 10);
    CHECK(span.end == 15);
    CHECK(span.line == 1);
    CHECK(span.column == 11);
}

TEST_CASE("LiteralValue variants", "[lexer][token]") {
    SECTION("Integer literal") {
        LiteralValue val = std::int64_t{42};
        CHECK(std::holds_alternative<std::int64_t>(val));
        CHECK(std::get<std::int64_t>(val) == 42);
    }
    
    SECTION("Float literal") {
        LiteralValue val = 3.14;
        CHECK(std::holds_alternative<double>(val));
        CHECK(std::get<double>(val) == 3.14);
    }
    
    SECTION("String literal") {
        LiteralValue val = std::string{"hello"};
        CHECK(std::holds_alternative<std::string>(val));
        CHECK(std::get<std::string>(val) == "hello");
    }
    
    SECTION("Char literal") {
        LiteralValue val = 'x';
        CHECK(std::holds_alternative<char>(val));
        CHECK(std::get<char>(val) == 'x');
    }
    
    SECTION("No value") {
        LiteralValue val = std::monostate{};
        CHECK(std::holds_alternative<std::monostate>(val));
    }
}

// ============================================================================
// LEXER TESTS - KEYWORDS
// ============================================================================

TEST_CASE("Lexer tokenizes keywords", "[lexer][keywords]") {
    Lexer lexer("fn let struct class trait impl if else while for");
    auto tokens = lexer.tokenize();
    
    REQUIRE(tokens.size() == 10); // 10 keywords, no EOF needed
    CHECK(tokens[0].kind == TokenKind::Fn);
    CHECK(tokens[1].kind == TokenKind::Let);
    CHECK(tokens[2].kind == TokenKind::Struct);
    CHECK(tokens[3].kind == TokenKind::Class);
    CHECK(tokens[4].kind == TokenKind::Trait);
    CHECK(tokens[5].kind == TokenKind::Impl);
    CHECK(tokens[6].kind == TokenKind::If);
    CHECK(tokens[7].kind == TokenKind::Else);
    CHECK(tokens[8].kind == TokenKind::While);
    CHECK(tokens[9].kind == TokenKind::For);
}

TEST_CASE("Lexer tokenizes all keywords", "[lexer][keywords]") {
    Lexer lexer("use fn let mut struct class trait impl if else while for in match return break continue as true false legacy py js self pub");
    auto tokens = lexer.tokenize();
    
    // Should have 25 tokens (25 keywords, no EOF)
    REQUIRE(tokens.size() == 25);
    CHECK(tokens[0].kind == TokenKind::Use);
    CHECK(tokens[1].kind == TokenKind::Fn);
    CHECK(tokens[2].kind == TokenKind::Let);
    CHECK(tokens[3].kind == TokenKind::Mut);
    CHECK(tokens[4].kind == TokenKind::Struct);
    CHECK(tokens[5].kind == TokenKind::Class);
    CHECK(tokens[6].kind == TokenKind::Trait);
    CHECK(tokens[7].kind == TokenKind::Impl);
    CHECK(tokens[8].kind == TokenKind::If);
    CHECK(tokens[9].kind == TokenKind::Else);
    CHECK(tokens[10].kind == TokenKind::While);
    CHECK(tokens[11].kind == TokenKind::For);
    CHECK(tokens[12].kind == TokenKind::In);
    CHECK(tokens[13].kind == TokenKind::Match);
    CHECK(tokens[14].kind == TokenKind::Return);
    CHECK(tokens[15].kind == TokenKind::Break);
    CHECK(tokens[16].kind == TokenKind::Continue);
    CHECK(tokens[17].kind == TokenKind::As);
    CHECK(tokens[18].kind == TokenKind::True);
    CHECK(tokens[19].kind == TokenKind::False);
    CHECK(tokens[20].kind == TokenKind::Legacy);
    CHECK(tokens[21].kind == TokenKind::Py);
    CHECK(tokens[22].kind == TokenKind::Js);
    CHECK(tokens[23].kind == TokenKind::Self);
    CHECK(tokens[24].kind == TokenKind::Pub);
}

// ============================================================================
// LEXER TESTS - OPERATORS
// ============================================================================

TEST_CASE("Lexer tokenizes single-char operators", "[lexer][operators]") {
    Lexer lexer("+ - * / % & | ^ ~ ! = < >");
    auto tokens = lexer.tokenize();
    
    CHECK(tokens[0].kind == TokenKind::Plus);
    CHECK(tokens[1].kind == TokenKind::Minus);
    CHECK(tokens[2].kind == TokenKind::Star);
    CHECK(tokens[3].kind == TokenKind::Slash);
    CHECK(tokens[4].kind == TokenKind::Percent);
    CHECK(tokens[5].kind == TokenKind::And);
    CHECK(tokens[6].kind == TokenKind::Or);
    CHECK(tokens[7].kind == TokenKind::Xor);
    CHECK(tokens[8].kind == TokenKind::Tilde);
    CHECK(tokens[9].kind == TokenKind::Bang);
    CHECK(tokens[10].kind == TokenKind::Eq);
    CHECK(tokens[11].kind == TokenKind::Lt);
    CHECK(tokens[12].kind == TokenKind::Gt);
}

TEST_CASE("Lexer tokenizes comparison operators", "[lexer][operators]") {
    Lexer lexer("== != < > <= >=");
    auto tokens = lexer.tokenize();
    
    CHECK(tokens[0].kind == TokenKind::EqEq);
    CHECK(tokens[1].kind == TokenKind::NotEq);
    CHECK(tokens[2].kind == TokenKind::Lt);
    CHECK(tokens[3].kind == TokenKind::Gt);
    CHECK(tokens[4].kind == TokenKind::LtEq);
    CHECK(tokens[5].kind == TokenKind::GtEq);
}

TEST_CASE("Lexer tokenizes logical operators", "[lexer][operators]") {
    Lexer lexer("&& || !");
    auto tokens = lexer.tokenize();
    
    CHECK(tokens[0].kind == TokenKind::AndAnd);
    CHECK(tokens[1].kind == TokenKind::OrOr);
    CHECK(tokens[2].kind == TokenKind::Bang);
}

TEST_CASE("Lexer tokenizes assignment operators", "[lexer][operators]") {
    Lexer lexer("+= -= *= /= %= &= |= ^= <<= >>=");
    auto tokens = lexer.tokenize();
    
    CHECK(tokens[0].kind == TokenKind::PlusEq);
    CHECK(tokens[1].kind == TokenKind::MinusEq);
    CHECK(tokens[2].kind == TokenKind::StarEq);
    CHECK(tokens[3].kind == TokenKind::SlashEq);
    CHECK(tokens[4].kind == TokenKind::PercentEq);
    CHECK(tokens[5].kind == TokenKind::AndEq);
    CHECK(tokens[6].kind == TokenKind::OrEq);
    CHECK(tokens[7].kind == TokenKind::XorEq);
    CHECK(tokens[8].kind == TokenKind::ShlEq);
    CHECK(tokens[9].kind == TokenKind::ShrEq);
}

TEST_CASE("Lexer tokenizes bitwise shift operators", "[lexer][operators]") {
    Lexer lexer("<< >>");
    auto tokens = lexer.tokenize();
    
    CHECK(tokens[0].kind == TokenKind::Shl);
    CHECK(tokens[1].kind == TokenKind::Shr);
}

TEST_CASE("Lexer tokenizes multi-char special operators", "[lexer][operators]") {
    Lexer lexer("-> => << >> :: .. ? @ #");
    auto tokens = lexer.tokenize();
    
    CHECK(tokens[0].kind == TokenKind::Arrow);
    CHECK(tokens[1].kind == TokenKind::FatArrow);
    CHECK(tokens[2].kind == TokenKind::Shl);
    CHECK(tokens[3].kind == TokenKind::Shr);
    CHECK(tokens[4].kind == TokenKind::DoubleColon);
    CHECK(tokens[5].kind == TokenKind::DotDot);
    CHECK(tokens[6].kind == TokenKind::Question);
    CHECK(tokens[7].kind == TokenKind::At);
    CHECK(tokens[8].kind == TokenKind::Hash);
}

// ============================================================================
// LEXER TESTS - DELIMITERS
// ============================================================================

TEST_CASE("Lexer tokenizes delimiters", "[lexer][delimiters]") {
    Lexer lexer("( ) { } [ ] , ; : . ");
    auto tokens = lexer.tokenize();
    
    CHECK(tokens[0].kind == TokenKind::LParen);
    CHECK(tokens[1].kind == TokenKind::RParen);
    CHECK(tokens[2].kind == TokenKind::LBrace);
    CHECK(tokens[3].kind == TokenKind::RBrace);
    CHECK(tokens[4].kind == TokenKind::LBracket);
    CHECK(tokens[5].kind == TokenKind::RBracket);
    CHECK(tokens[6].kind == TokenKind::Comma);
    CHECK(tokens[7].kind == TokenKind::Semicolon);
    CHECK(tokens[8].kind == TokenKind::Colon);
    CHECK(tokens[9].kind == TokenKind::Dot);
}

// ============================================================================
// LEXER TESTS - INTEGER LITERALS
// ============================================================================

TEST_CASE("Lexer tokenizes decimal integers", "[lexer][literals][int]") {
    Lexer lexer("0 42 1000 12345");
    auto tokens = lexer.tokenize();
    
    REQUIRE(tokens[0].kind == TokenKind::IntLiteral);
    CHECK(std::get<std::int64_t>(tokens[0].value) == 0);
    
    REQUIRE(tokens[1].kind == TokenKind::IntLiteral);
    CHECK(std::get<std::int64_t>(tokens[1].value) == 42);
    
    REQUIRE(tokens[2].kind == TokenKind::IntLiteral);
    CHECK(std::get<std::int64_t>(tokens[2].value) == 1000);
    
    REQUIRE(tokens[3].kind == TokenKind::IntLiteral);
    CHECK(std::get<std::int64_t>(tokens[3].value) == 12345);
}

TEST_CASE("Lexer tokenizes hexadecimal integers", "[lexer][literals][int]") {
    Lexer lexer("0xFF 0x10 0xDEAD 0x0 0xffff");
    auto tokens = lexer.tokenize();
    
    REQUIRE(tokens[0].kind == TokenKind::IntLiteral);
    CHECK(std::get<std::int64_t>(tokens[0].value) == 255);
    
    REQUIRE(tokens[1].kind == TokenKind::IntLiteral);
    CHECK(std::get<std::int64_t>(tokens[1].value) == 16);
    
    REQUIRE(tokens[2].kind == TokenKind::IntLiteral);
    CHECK(std::get<std::int64_t>(tokens[2].value) == 0xDEAD);
    
    REQUIRE(tokens[3].kind == TokenKind::IntLiteral);
    CHECK(std::get<std::int64_t>(tokens[3].value) == 0);
    
    REQUIRE(tokens[4].kind == TokenKind::IntLiteral);
    CHECK(std::get<std::int64_t>(tokens[4].value) == 0xffff);
}

TEST_CASE("Lexer tokenizes binary integers", "[lexer][literals][int]") {
    Lexer lexer("0b101 0b1111 0b0 0b11111111");
    auto tokens = lexer.tokenize();
    
    REQUIRE(tokens[0].kind == TokenKind::IntLiteral);
    CHECK(std::get<std::int64_t>(tokens[0].value) == 5);
    
    REQUIRE(tokens[1].kind == TokenKind::IntLiteral);
    CHECK(std::get<std::int64_t>(tokens[1].value) == 15);
    
    REQUIRE(tokens[2].kind == TokenKind::IntLiteral);
    CHECK(std::get<std::int64_t>(tokens[2].value) == 0);
    
    REQUIRE(tokens[3].kind == TokenKind::IntLiteral);
    CHECK(std::get<std::int64_t>(tokens[3].value) == 255);
}

TEST_CASE("Lexer tokenizes integers with underscores", "[lexer][literals][int]") {
    Lexer lexer("1_000 0xFF_FF 0b1111_0000");
    auto tokens = lexer.tokenize();
    
    REQUIRE(tokens[0].kind == TokenKind::IntLiteral);
    CHECK(std::get<std::int64_t>(tokens[0].value) == 1000);
    
    REQUIRE(tokens[1].kind == TokenKind::IntLiteral);
    CHECK(std::get<std::int64_t>(tokens[1].value) == 0xFFFF);
    
    REQUIRE(tokens[2].kind == TokenKind::IntLiteral);
    CHECK(std::get<std::int64_t>(tokens[2].value) == 0b11110000);
}

// ============================================================================
// LEXER TESTS - FLOAT LITERALS
// ============================================================================

TEST_CASE("Lexer tokenizes float literals", "[lexer][literals][float]") {
    Lexer lexer("3.14 0.5 1.0 0.1");
    auto tokens = lexer.tokenize();
    
    REQUIRE(tokens[0].kind == TokenKind::FloatLiteral);
    CHECK(std::get<double>(tokens[0].value) == 3.14);
    
    REQUIRE(tokens[1].kind == TokenKind::FloatLiteral);
    CHECK(std::get<double>(tokens[1].value) == 0.5);
    
    REQUIRE(tokens[2].kind == TokenKind::FloatLiteral);
    CHECK(std::get<double>(tokens[2].value) == 1.0);
    
    REQUIRE(tokens[3].kind == TokenKind::FloatLiteral);
    CHECK(std::get<double>(tokens[3].value) == 0.1);
}

TEST_CASE("Lexer tokenizes float literals with exponents", "[lexer][literals][float]") {
    Lexer lexer("1.0e10 2.5e-3 1e5 1e-5 3.14e2");
    auto tokens = lexer.tokenize();
    
    REQUIRE(tokens[0].kind == TokenKind::FloatLiteral);
    CHECK(std::get<double>(tokens[0].value) == 1.0e10);
    
    REQUIRE(tokens[1].kind == TokenKind::FloatLiteral);
    CHECK(std::get<double>(tokens[1].value) == 2.5e-3);
    
    REQUIRE(tokens[2].kind == TokenKind::FloatLiteral);
    CHECK(std::get<double>(tokens[2].value) == 1e5);
    
    REQUIRE(tokens[3].kind == TokenKind::FloatLiteral);
    CHECK(std::get<double>(tokens[3].value) == 1e-5);
    
    REQUIRE(tokens[4].kind == TokenKind::FloatLiteral);
    CHECK(std::get<double>(tokens[4].value) == 3.14e2);
}

TEST_CASE("Lexer tokenizes floats with underscores", "[lexer][literals][float]") {
    Lexer lexer("1_000.5 3.14_159");
    auto tokens = lexer.tokenize();
    
    REQUIRE(tokens[0].kind == TokenKind::FloatLiteral);
    CHECK(std::get<double>(tokens[0].value) == 1000.5);
    
    REQUIRE(tokens[1].kind == TokenKind::FloatLiteral);
    CHECK(std::get<double>(tokens[1].value) == 3.14159);
}

// ============================================================================
// LEXER TESTS - STRING LITERALS
// ============================================================================

TEST_CASE("Lexer tokenizes simple string literals", "[lexer][literals][string]") {
    Lexer lexer("\"hello\" \"world\" \"foo bar\"");
    auto tokens = lexer.tokenize();
    
    REQUIRE(tokens[0].kind == TokenKind::StringLiteral);
    CHECK(std::get<std::string>(tokens[0].value) == "hello");
    
    REQUIRE(tokens[1].kind == TokenKind::StringLiteral);
    CHECK(std::get<std::string>(tokens[1].value) == "world");
    
    REQUIRE(tokens[2].kind == TokenKind::StringLiteral);
    CHECK(std::get<std::string>(tokens[2].value) == "foo bar");
}

TEST_CASE("Lexer tokenizes string literals with escape sequences", "[lexer][literals][string]") {
    Lexer lexer(R"("with\nnewline" "with\ttabs" "with\\backslash")");
    auto tokens = lexer.tokenize();
    
    REQUIRE(tokens[0].kind == TokenKind::StringLiteral);
    CHECK(std::get<std::string>(tokens[0].value) == "with\nnewline");
    
    REQUIRE(tokens[1].kind == TokenKind::StringLiteral);
    CHECK(std::get<std::string>(tokens[1].value) == "with\ttabs");
    
    REQUIRE(tokens[2].kind == TokenKind::StringLiteral);
    CHECK(std::get<std::string>(tokens[2].value) == "with\\backslash");
}

TEST_CASE("Lexer tokenizes string literals with quote escapes", "[lexer][literals][string]") {
    Lexer lexer(R"("with\"quotes" "with\\backslash")");
    auto tokens = lexer.tokenize();
    
    REQUIRE(tokens[0].kind == TokenKind::StringLiteral);
    CHECK(std::get<std::string>(tokens[0].value) == "with\"quotes");
    
    REQUIRE(tokens[1].kind == TokenKind::StringLiteral);
    CHECK(std::get<std::string>(tokens[1].value) == "with\\backslash");
}

TEST_CASE("Lexer handles unterminated strings", "[lexer][literals][string][error]") {
    Lexer lexer("\"unterminated");
    auto tokens = lexer.tokenize();
    CHECK(lexer.has_errors());
}

// ============================================================================
// LEXER TESTS - CHAR LITERALS
// ============================================================================

TEST_CASE("Lexer tokenizes simple char literals", "[lexer][literals][char]") {
    Lexer lexer("'a' 'Z' '0' ' '");
    auto tokens = lexer.tokenize();
    
    REQUIRE(tokens[0].kind == TokenKind::CharLiteral);
    CHECK(std::get<char>(tokens[0].value) == 'a');
    
    REQUIRE(tokens[1].kind == TokenKind::CharLiteral);
    CHECK(std::get<char>(tokens[1].value) == 'Z');
    
    REQUIRE(tokens[2].kind == TokenKind::CharLiteral);
    CHECK(std::get<char>(tokens[2].value) == '0');
    
    REQUIRE(tokens[3].kind == TokenKind::CharLiteral);
    CHECK(std::get<char>(tokens[3].value) == ' ');
}

TEST_CASE("Lexer tokenizes char literals with escape sequences", "[lexer][literals][char]") {
    Lexer lexer("'\\n' '\\t' '\\r' '\\\\' '\\0'");
    auto tokens = lexer.tokenize();
    
    REQUIRE(tokens[0].kind == TokenKind::CharLiteral);
    CHECK(std::get<char>(tokens[0].value) == '\n');
    
    REQUIRE(tokens[1].kind == TokenKind::CharLiteral);
    CHECK(std::get<char>(tokens[1].value) == '\t');
    
    REQUIRE(tokens[2].kind == TokenKind::CharLiteral);
    CHECK(std::get<char>(tokens[2].value) == '\r');
    
    REQUIRE(tokens[3].kind == TokenKind::CharLiteral);
    CHECK(std::get<char>(tokens[3].value) == '\\');
    
    REQUIRE(tokens[4].kind == TokenKind::CharLiteral);
    CHECK(std::get<char>(tokens[4].value) == '\0');
}

TEST_CASE("Lexer tokenizes char literal with single quote", "[lexer][literals][char]") {
    Lexer lexer("'\\''");
    auto tokens = lexer.tokenize();
    
    REQUIRE(tokens[0].kind == TokenKind::CharLiteral);
    CHECK(std::get<char>(tokens[0].value) == '\'');
}

TEST_CASE("Lexer handles unterminated char literals", "[lexer][literals][char][error]") {
    Lexer lexer("'a");
    auto tokens = lexer.tokenize();
    CHECK(lexer.has_errors());
}

// ============================================================================
// LEXER TESTS - IDENTIFIERS
// ============================================================================

TEST_CASE("Lexer tokenizes identifiers", "[lexer][identifiers]") {
    Lexer lexer("foo bar_baz _private CamelCase x1 y2z var");
    auto tokens = lexer.tokenize();
    
    CHECK(tokens[0].kind == TokenKind::Identifier);
    CHECK(tokens[0].lexeme == "foo");
    
    CHECK(tokens[1].kind == TokenKind::Identifier);
    CHECK(tokens[1].lexeme == "bar_baz");
    
    CHECK(tokens[2].kind == TokenKind::Identifier);
    CHECK(tokens[2].lexeme == "_private");
    
    CHECK(tokens[3].kind == TokenKind::Identifier);
    CHECK(tokens[3].lexeme == "CamelCase");
    
    CHECK(tokens[4].kind == TokenKind::Identifier);
    CHECK(tokens[4].lexeme == "x1");
    
    CHECK(tokens[5].kind == TokenKind::Identifier);
    CHECK(tokens[5].lexeme == "y2z");
    
    CHECK(tokens[6].kind == TokenKind::Identifier);
    CHECK(tokens[6].lexeme == "var");
}

TEST_CASE("Lexer distinguishes identifiers from keywords", "[lexer][identifiers]") {
    Lexer lexer("if iff ifx x_if");
    auto tokens = lexer.tokenize();
    
    CHECK(tokens[0].kind == TokenKind::If);
    
    CHECK(tokens[1].kind == TokenKind::Identifier);
    CHECK(tokens[1].lexeme == "iff");
    
    CHECK(tokens[2].kind == TokenKind::Identifier);
    CHECK(tokens[2].lexeme == "ifx");
    
    CHECK(tokens[3].kind == TokenKind::Identifier);
    CHECK(tokens[3].lexeme == "x_if");
}

// ============================================================================
// LEXER TESTS - COMMENTS
// ============================================================================

TEST_CASE("Lexer skips line comments", "[lexer][comments]") {
    Lexer lexer("foo // this is a comment\nbar");
    auto tokens = lexer.tokenize();
    
    REQUIRE(tokens.size() == 2); // foo, bar
    CHECK(tokens[0].lexeme == "foo");
    CHECK(tokens[1].lexeme == "bar");
}

TEST_CASE("Lexer skips block comments", "[lexer][comments]") {
    Lexer lexer("foo /* comment */ bar");
    auto tokens = lexer.tokenize();
    
    REQUIRE(tokens.size() == 2); // foo, bar
    CHECK(tokens[0].lexeme == "foo");
    CHECK(tokens[1].lexeme == "bar");
}

TEST_CASE("Lexer skips nested block comments", "[lexer][comments]") {
    Lexer lexer("foo /* outer /* inner */ outer */ bar");
    auto tokens = lexer.tokenize();
    
    REQUIRE(tokens.size() == 2); // foo, bar
    CHECK(tokens[0].lexeme == "foo");
    CHECK(tokens[1].lexeme == "bar");
}

TEST_CASE("Lexer skips multiple nested block comments", "[lexer][comments]") {
    Lexer lexer("a /* /* /* nested */ */ */ b");
    auto tokens = lexer.tokenize();
    
    REQUIRE(tokens.size() == 2); // a, b
    CHECK(tokens[0].lexeme == "a");
    CHECK(tokens[1].lexeme == "b");
}

TEST_CASE("Lexer skips comments in code", "[lexer][comments]") {
    Lexer lexer("fn foo() { // This is a function\n    return 42; /* answer */ }");
    auto tokens = lexer.tokenize();
    
    // Should parse correctly despite comments
    bool found_fn = false;
    bool found_return = false;
    for (const auto& tok : tokens) {
        if (tok.kind == TokenKind::Fn) found_fn = true;
        if (tok.kind == TokenKind::Return) found_return = true;
    }
    CHECK(found_fn);
    CHECK(found_return);
}

// ============================================================================
// LEXER TESTS - SOURCE POSITIONS
// ============================================================================

TEST_CASE("Lexer tracks source positions", "[lexer][positions]") {
    Lexer lexer("foo bar");
    auto tokens = lexer.tokenize();
    
    // First token should be at line 1, column 1
    CHECK(tokens[0].span.line == 1);
    CHECK(tokens[0].span.column == 1);
    
    // Second token should be after "foo " (after whitespace)
    CHECK(tokens[1].span.line == 1);
}

TEST_CASE("Lexer tracks line numbers in multiline code", "[lexer][positions]") {
    Lexer lexer("fn main() {\n    return 42\n}");
    auto tokens = lexer.tokenize();
    
    // fn is at line 1
    CHECK(tokens[0].kind == TokenKind::Fn);
    CHECK(tokens[0].span.line == 1);
    
    // return should be at line 2
    bool found_return = false;
    for (const auto& tok : tokens) {
        if (tok.kind == TokenKind::Return) {
            CHECK(tok.span.line == 2);
            found_return = true;
            break;
        }
    }
    CHECK(found_return);
    
    // closing brace should be at line 3
    bool found_rbrace_at_line3 = false;
    for (const auto& tok : tokens) {
        if (tok.kind == TokenKind::RBrace) {
            if (tok.span.line == 3) {
                found_rbrace_at_line3 = true;
                break;
            }
        }
    }
    CHECK(found_rbrace_at_line3);
}

// ============================================================================
// LEXER TESTS - COMPLETE PROGRAMS
// ============================================================================

TEST_CASE("Lexer handles function syntax", "[lexer][complex]") {
    Lexer lexer("fn add(a: int, b: int) -> int { return a + b }");
    auto tokens = lexer.tokenize();
    
    // Verify key tokens
    CHECK(tokens[0].kind == TokenKind::Fn);
    
    bool found_colon = false;
    bool found_arrow = false;
    bool found_plus = false;
    
    for (const auto& tok : tokens) {
        if (tok.kind == TokenKind::Colon) found_colon = true;
        if (tok.kind == TokenKind::Arrow) found_arrow = true;
        if (tok.kind == TokenKind::Plus) found_plus = true;
    }
    
    CHECK(found_colon);
    CHECK(found_arrow);
    CHECK(found_plus);
}

TEST_CASE("Lexer handles struct syntax", "[lexer][complex]") {
    Lexer lexer("struct Point { x: f64, y: f64 }");
    auto tokens = lexer.tokenize();
    
    CHECK(tokens[0].kind == TokenKind::Struct);
    
    bool found_lbrace = false;
    bool found_rbrace = false;
    bool found_comma = false;
    
    for (const auto& tok : tokens) {
        if (tok.kind == TokenKind::LBrace) found_lbrace = true;
        if (tok.kind == TokenKind::RBrace) found_rbrace = true;
        if (tok.kind == TokenKind::Comma) found_comma = true;
    }
    
    CHECK(found_lbrace);
    CHECK(found_rbrace);
    CHECK(found_comma);
}

TEST_CASE("Lexer handles expressions with operators", "[lexer][complex]") {
    Lexer lexer("x = (a + b) * c - d / e % f");
    auto tokens = lexer.tokenize();
    
    bool found_eq = false;
    bool found_plus = false;
    bool found_star = false;
    bool found_minus = false;
    bool found_slash = false;
    bool found_percent = false;
    
    for (const auto& tok : tokens) {
        if (tok.kind == TokenKind::Eq) found_eq = true;
        if (tok.kind == TokenKind::Plus) found_plus = true;
        if (tok.kind == TokenKind::Star) found_star = true;
        if (tok.kind == TokenKind::Minus) found_minus = true;
        if (tok.kind == TokenKind::Slash) found_slash = true;
        if (tok.kind == TokenKind::Percent) found_percent = true;
    }
    
    CHECK(found_eq);
    CHECK(found_plus);
    CHECK(found_star);
    CHECK(found_minus);
    CHECK(found_slash);
    CHECK(found_percent);
}

// ============================================================================
// LEXER TESTS - ERROR HANDLING
// ============================================================================

TEST_CASE("Lexer handles empty input", "[lexer][edge-cases]") {
    Lexer lexer("");
    auto tokens = lexer.tokenize();
    
    // Empty input produces no tokens (Eof is not added if source is empty)
    CHECK(tokens.empty());
}

TEST_CASE("Lexer handles only whitespace", "[lexer][edge-cases]") {
    Lexer lexer("   \n\t  \n  ");
    auto tokens = lexer.tokenize();
    
    // Whitespace-only input may produce EOF token
    // Just verify it doesn't produce meaningful tokens
    for (const auto& tok : tokens) {
        CHECK((tok.kind == TokenKind::Eof || tok.kind == TokenKind::Error));
    }
}

TEST_CASE("Lexer handles only comments", "[lexer][edge-cases]") {
    Lexer lexer("// comment\n/* block comment */");
    auto tokens = lexer.tokenize();
    
    // Comment-only input may produce EOF token
    // Just verify it doesn't produce meaningful tokens
    for (const auto& tok : tokens) {
        CHECK((tok.kind == TokenKind::Eof || tok.kind == TokenKind::Error));
    }
}

TEST_CASE("Lexer reports errors for invalid input", "[lexer][error]") {
    SECTION("Unterminated string") {
        Lexer lexer("\"hello");
        auto tokens = lexer.tokenize();
        CHECK(lexer.has_errors());
    }
    
    SECTION("Unterminated char literal") {
        Lexer lexer("'a");
        auto tokens = lexer.tokenize();
        CHECK(lexer.has_errors());
    }
}
