#include <catch2/catch_test_macros.hpp>
#include "idotc/lexer/lexer.hpp"
#include "idotc/parser/parser.hpp"

using namespace idotc;

// Helper to parse source code
Parser make_parser(std::string_view source) {
    Lexer lexer(source);
    return Parser(lexer.tokenize());
}

TEST_CASE("Parser parses function declarations", "[parser]") {
    SECTION("Simple function") {
        auto parser = make_parser("fn main() { }");
        auto result = parser.parse_program();
        
        REQUIRE(result.has_value());
        REQUIRE(result->items.size() == 1);
        
        auto* fn = std::get_if<FunctionItem>(&result->items[0]->variant);
        REQUIRE(fn != nullptr);
        CHECK(fn->name == "main");
        CHECK(fn->params.empty());
        CHECK(!fn->return_type.has_value());
    }
    
    SECTION("Function with parameters") {
        auto parser = make_parser("fn add(a: int, b: int) -> int { return a + b }");
        auto result = parser.parse_program();
        
        REQUIRE(result.has_value());
        auto* fn = std::get_if<FunctionItem>(&result->items[0]->variant);
        REQUIRE(fn != nullptr);
        CHECK(fn->name == "add");
        REQUIRE(fn->params.size() == 2);
        CHECK(fn->params[0].name == "a");
        CHECK(fn->params[0].type.name == "int");
        CHECK(fn->params[1].name == "b");
        REQUIRE(fn->return_type.has_value());
        CHECK(fn->return_type->name == "int");
    }
}

TEST_CASE("Parser parses struct declarations", "[parser]") {
    auto parser = make_parser(R"(
        struct Point {
            x: float,
            y: float,
        }
    )");
    auto result = parser.parse_program();
    
    REQUIRE(result.has_value());
    auto* s = std::get_if<StructItem>(&result->items[0]->variant);
    REQUIRE(s != nullptr);
    CHECK(s->name == "Point");
    REQUIRE(s->fields.size() == 2);
    CHECK(s->fields[0].name == "x");
    CHECK(s->fields[0].type.name == "float");
    CHECK(s->fields[1].name == "y");
}

TEST_CASE("Parser parses class declarations", "[parser]") {
    auto parser = make_parser(R"(
        class Circle {
            radius: float,
            
            fn area(self) -> float {
                return 3.14159 * self.radius * self.radius
            }
        }
    )");
    auto result = parser.parse_program();
    
    REQUIRE(result.has_value());
    auto* c = std::get_if<ClassItem>(&result->items[0]->variant);
    REQUIRE(c != nullptr);
    CHECK(c->name == "Circle");
    REQUIRE(c->fields.size() == 1);
    CHECK(c->fields[0].name == "radius");
    REQUIRE(c->methods.size() == 1);
    CHECK(c->methods[0].name == "area");
}

TEST_CASE("Parser parses use statements", "[parser]") {
    SECTION("Normal import") {
        auto parser = make_parser(R"(use "utils/math.idot" as math)");
        auto result = parser.parse_program();
        
        REQUIRE(result.has_value());
        auto* u = std::get_if<UseItem>(&result->items[0]->variant);
        REQUIRE(u != nullptr);
        CHECK(u->path == "utils/math.idot");
        CHECK(u->alias == "math");
        CHECK(!u->language.has_value());
    }
    
    SECTION("Python import") {
        auto parser = make_parser(R"(use py."script.py" as py_module)");
        auto result = parser.parse_program();
        
        REQUIRE(result.has_value());
        auto* u = std::get_if<UseItem>(&result->items[0]->variant);
        REQUIRE(u != nullptr);
        CHECK(u->path == "script.py");
        CHECK(u->alias == "py_module");
        REQUIRE(u->language.has_value());
        CHECK(*u->language == "py");
    }
}

TEST_CASE("Parser parses let statements", "[parser]") {
    SECTION("With type annotation") {
        auto parser = make_parser("fn test() { let x: int = 42 }");
        auto result = parser.parse_program();
        
        REQUIRE(result.has_value());
        auto* fn = std::get_if<FunctionItem>(&result->items[0]->variant);
        REQUIRE(fn != nullptr);
        REQUIRE(!fn->body.empty());
        
        auto* let = std::get_if<LetStmt>(&fn->body[0]->variant);
        REQUIRE(let != nullptr);
        CHECK(let->name == "x");
        REQUIRE(let->type.has_value());
        CHECK(let->type->name == "int");
    }
    
    SECTION("Mutable variable") {
        auto parser = make_parser("fn test() { let mut counter = 0 }");
        auto result = parser.parse_program();
        
        REQUIRE(result.has_value());
        auto* fn = std::get_if<FunctionItem>(&result->items[0]->variant);
        auto* let = std::get_if<LetStmt>(&fn->body[0]->variant);
        REQUIRE(let != nullptr);
        CHECK(let->is_mutable);
    }
}

TEST_CASE("Parser parses expressions", "[parser]") {
    SECTION("Binary arithmetic") {
        auto parser = make_parser("fn test() { return 1 + 2 * 3 }");
        auto result = parser.parse_program();
        
        REQUIRE(result.has_value());
        // The expression should respect precedence: 1 + (2 * 3)
    }
    
    SECTION("Function call") {
        auto parser = make_parser("fn test() { return add(1, 2) }");
        auto result = parser.parse_program();
        
        REQUIRE(result.has_value());
        auto* fn = std::get_if<FunctionItem>(&result->items[0]->variant);
        auto* ret = std::get_if<ReturnStmt>(&fn->body[0]->variant);
        REQUIRE(ret != nullptr);
        REQUIRE(ret->value.has_value());
        
        auto* call = std::get_if<CallExpr>(&(*ret->value)->variant);
        REQUIRE(call != nullptr);
        CHECK(call->name == "add");
        CHECK(call->args.size() == 2);
    }
    
    SECTION("Member access") {
        auto parser = make_parser("fn test() { return point.x }");
        auto result = parser.parse_program();
        
        REQUIRE(result.has_value());
    }
}

TEST_CASE("Parser parses control flow", "[parser]") {
    SECTION("If statement") {
        auto parser = make_parser(R"(
            fn test(x: int) {
                if x > 0 {
                    return 1
                } else {
                    return 0
                }
            }
        )");
        auto result = parser.parse_program();
        REQUIRE(result.has_value());
    }
    
    SECTION("While loop") {
        auto parser = make_parser(R"(
            fn count() {
                let mut i = 0
                while i < 10 {
                    i = i + 1
                }
            }
        )");
        auto result = parser.parse_program();
        REQUIRE(result.has_value());
    }
    
    SECTION("For loop") {
        auto parser = make_parser(R"(
            fn sum() {
                let mut total = 0
                for x in items {
                    total = total + x
                }
            }
        )");
        auto result = parser.parse_program();
        REQUIRE(result.has_value());
    }
    
    SECTION("Match expression") {
        auto parser = make_parser(R"(
            fn describe(n: int) {
                match n {
                    0 => return 0,
                    1 => return 1,
                    _ => return n,
                }
            }
        )");
        auto result = parser.parse_program();
        REQUIRE(result.has_value());
    }
}

TEST_CASE("Parser reports errors", "[parser]") {
    SECTION("Missing function name") {
        auto parser = make_parser("fn () { }");
        auto result = parser.parse_program();
        CHECK(parser.has_errors());
    }
    
    SECTION("Missing closing brace") {
        auto parser = make_parser("fn main() {");
        auto result = parser.parse_program();
        CHECK(parser.has_errors());
    }
    
    SECTION("Invalid expression") {
        auto parser = make_parser("fn test() { return + }");
        auto result = parser.parse_program();
        CHECK(parser.has_errors());
    }
}

TEST_CASE("Parser handles complete programs", "[parser]") {
    auto parser = make_parser(R"(
        use "utils/math.idot" as math
        
        struct Point {
            x: float,
            y: float,
        }
        
        fn distance(p1: Point, p2: Point) -> float {
            let dx = p2.x - p1.x
            let dy = p2.y - p1.y
            return math.sqrt(dx * dx + dy * dy)
        }
        
        fn main() -> int {
            let p1 = Point { x: 0.0, y: 0.0 }
            let p2 = Point { x: 3.0, y: 4.0 }
            let d = distance(p1, p2)
            return 0
        }
    )");
    auto result = parser.parse_program();
    
    // Should have 4 items: use, struct, distance fn, main fn
    REQUIRE(result.has_value());
    CHECK(result->items.size() == 4);
}
