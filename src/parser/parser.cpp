#include "idotc/parser/parser.hpp"

namespace idotc {

Parser::Parser(std::vector<Token> tokens)
    : tokens_(std::move(tokens))
{}

const Token& Parser::current() const {
    return tokens_[current_];
}

const Token& Parser::peek() const {
    if (current_ + 1 >= tokens_.size()) return tokens_.back();
    return tokens_[current_ + 1];
}

const Token& Parser::previous() const {
    return tokens_[current_ > 0 ? current_ - 1 : 0];
}

bool Parser::is_at_end() const {
    return current().kind == TokenKind::Eof;
}

const Token& Parser::advance() {
    if (!is_at_end()) ++current_;
    return previous();
}

bool Parser::check(TokenKind kind) const {
    return current().kind == kind;
}

bool Parser::match(TokenKind kind) {
    if (check(kind)) {
        advance();
        return true;
    }
    return false;
}

bool Parser::match(std::initializer_list<TokenKind> kinds) {
    for (TokenKind kind : kinds) {
        if (match(kind)) return true;
    }
    return false;
}

const Token& Parser::expect(TokenKind kind, std::string_view message) {
    if (check(kind)) return advance();
    report_error(message);
    return current();
}

void Parser::report_error(std::string_view message) {
    report_error(message, current());
}

void Parser::report_error(std::string_view message, const Token& token) {
    errors_.push_back(
        error("E0002", std::string(message))
            .with_label(token.span, std::string(token.lexeme))
    );
}

void Parser::synchronize() {
    advance();
    while (!is_at_end()) {
        if (previous().kind == TokenKind::Semicolon) return;
        switch (current().kind) {
            case TokenKind::Fn:
            case TokenKind::Let:
            case TokenKind::Struct:
            case TokenKind::Class:
            case TokenKind::Trait:
            case TokenKind::Impl:
            case TokenKind::If:
            case TokenKind::While:
            case TokenKind::For:
            case TokenKind::Return:
            case TokenKind::Use:
                return;
            default:
                advance();
        }
    }
}

Result<Program> Parser::parse_program() {
    Program program;
    program.span = current().span;
    
    while (!is_at_end()) {
        auto item = parse_item();
        if (!item) {
            errors_.push_back(item.error());
            synchronize();
        } else {
            program.items.push_back(std::move(*item));
        }
    }
    
    if (has_errors()) {
        return std::unexpected(errors_.front());
    }
    
    return program;
}

Result<ItemPtr> Parser::parse_item() {
    [[maybe_unused]] bool is_public = match(TokenKind::Pub);
    
    if (check(TokenKind::Fn)) return parse_function();
    if (check(TokenKind::Struct)) return parse_struct();
    if (check(TokenKind::Class)) return parse_class();
    if (check(TokenKind::Trait)) return parse_trait();
    if (check(TokenKind::Impl)) return parse_impl();
    if (check(TokenKind::Use)) return parse_use();
    
    return std::unexpected(error("E0002", "Expected item declaration")
        .with_label(current().span, "expected fn, struct, class, trait, impl, or use"));
}

Result<ItemPtr> Parser::parse_function() {
    SourceSpan start = current().span;
    expect(TokenKind::Fn, "Expected 'fn'");
    
    auto name_tok = expect(TokenKind::Identifier, "Expected function name");
    std::string name(name_tok.lexeme);
    
    expect(TokenKind::LParen, "Expected '(' after function name");
    
    std::vector<Parameter> params;
    if (!check(TokenKind::RParen)) {
        do {
            auto param = parse_parameter();
            if (!param) return std::unexpected(param.error());
            params.push_back(std::move(*param));
        } while (match(TokenKind::Comma));
    }
    expect(TokenKind::RParen, "Expected ')' after parameters");
    
    std::optional<Type> return_type;
    if (match(TokenKind::Arrow)) {
        auto type = parse_type();
        if (!type) return std::unexpected(type.error());
        return_type = std::move(*type);
    }
    
    auto body = parse_block();
    if (!body) return std::unexpected(body.error());
    
    FunctionItem fn;
    fn.name = std::move(name);
    fn.params = std::move(params);
    fn.return_type = std::move(return_type);
    fn.body = std::move(*body);
    fn.span = start;
    
    return Item::make(std::move(fn));
}

Result<ItemPtr> Parser::parse_struct() {
    SourceSpan start = current().span;
    expect(TokenKind::Struct, "Expected 'struct'");
    
    auto name_tok = expect(TokenKind::Identifier, "Expected struct name");
    std::string name(name_tok.lexeme);
    
    expect(TokenKind::LBrace, "Expected '{' after struct name");
    
    std::vector<Field> fields;
    while (!check(TokenKind::RBrace) && !is_at_end()) {
        auto field = parse_field();
        if (!field) return std::unexpected(field.error());
        fields.push_back(std::move(*field));
        
        if (!check(TokenKind::RBrace)) {
            expect(TokenKind::Comma, "Expected ',' between fields");
        }
    }
    expect(TokenKind::RBrace, "Expected '}' after struct fields");
    
    StructItem s;
    s.name = std::move(name);
    s.fields = std::move(fields);
    s.span = start;
    
    return Item::make(std::move(s));
}

Result<ItemPtr> Parser::parse_class() {
    SourceSpan start = current().span;
    expect(TokenKind::Class, "Expected 'class'");
    
    auto name_tok = expect(TokenKind::Identifier, "Expected class name");
    std::string name(name_tok.lexeme);
    
    expect(TokenKind::LBrace, "Expected '{' after class name");
    
    std::vector<Field> fields;
    std::vector<FunctionItem> methods;
    
    while (!check(TokenKind::RBrace) && !is_at_end()) {
        if (check(TokenKind::Fn)) {
            auto fn_item = parse_function();
            if (!fn_item) return std::unexpected(fn_item.error());
            auto* fn = std::get_if<FunctionItem>(&(*fn_item)->variant);
            if (fn) methods.push_back(std::move(*fn));
        } else {
            auto field = parse_field();
            if (!field) return std::unexpected(field.error());
            fields.push_back(std::move(*field));
            if (!check(TokenKind::RBrace) && !check(TokenKind::Fn)) {
                expect(TokenKind::Comma, "Expected ',' between fields");
            }
        }
    }
    expect(TokenKind::RBrace, "Expected '}' after class body");
    
    ClassItem c;
    c.name = std::move(name);
    c.fields = std::move(fields);
    c.methods = std::move(methods);
    c.span = start;
    
    return Item::make(std::move(c));
}

Result<ItemPtr> Parser::parse_trait() {
    SourceSpan start = current().span;
    expect(TokenKind::Trait, "Expected 'trait'");
    
    auto name_tok = expect(TokenKind::Identifier, "Expected trait name");
    std::string name(name_tok.lexeme);
    
    expect(TokenKind::LBrace, "Expected '{' after trait name");
    
    std::vector<FunctionItem> methods;
    while (!check(TokenKind::RBrace) && !is_at_end()) {
        auto fn_item = parse_function();
        if (!fn_item) return std::unexpected(fn_item.error());
        auto* fn = std::get_if<FunctionItem>(&(*fn_item)->variant);
        if (fn) methods.push_back(std::move(*fn));
    }
    expect(TokenKind::RBrace, "Expected '}' after trait body");
    
    TraitItem t;
    t.name = std::move(name);
    t.methods = std::move(methods);
    t.span = start;
    
    return Item::make(std::move(t));
}

Result<ItemPtr> Parser::parse_impl() {
    SourceSpan start = current().span;
    expect(TokenKind::Impl, "Expected 'impl'");
    
    auto type_name_tok = expect(TokenKind::Identifier, "Expected type name");
    std::string type_name(type_name_tok.lexeme);
    
    std::optional<std::string> trait_name;
    if (match(TokenKind::For)) {
        trait_name = std::move(type_name);
        auto target_tok = expect(TokenKind::Identifier, "Expected type name after 'for'");
        type_name = std::string(target_tok.lexeme);
    }
    
    expect(TokenKind::LBrace, "Expected '{' after impl header");
    
    std::vector<FunctionItem> methods;
    while (!check(TokenKind::RBrace) && !is_at_end()) {
        auto fn_item = parse_function();
        if (!fn_item) return std::unexpected(fn_item.error());
        auto* fn = std::get_if<FunctionItem>(&(*fn_item)->variant);
        if (fn) methods.push_back(std::move(*fn));
    }
    expect(TokenKind::RBrace, "Expected '}' after impl body");
    
    ImplItem impl;
    impl.type_name = std::move(type_name);
    impl.trait_name = std::move(trait_name);
    impl.methods = std::move(methods);
    impl.span = start;
    
    return Item::make(std::move(impl));
}

Result<ItemPtr> Parser::parse_use() {
    SourceSpan start = current().span;
    expect(TokenKind::Use, "Expected 'use'");
    
    std::optional<std::string> language;
    if (match(TokenKind::Py)) {
        language = "py";
        expect(TokenKind::Dot, "Expected '.' after 'py'");
    } else if (match(TokenKind::Js)) {
        language = "js";
        expect(TokenKind::Dot, "Expected '.' after 'js'");
    }
    
    auto path_tok = expect(TokenKind::StringLiteral, "Expected path string");
    std::string path;
    if (std::holds_alternative<std::string>(path_tok.value)) {
        path = std::get<std::string>(path_tok.value);
    } else {
        path = std::string(path_tok.lexeme);
    }
    
    expect(TokenKind::As, "Expected 'as' after path");
    
    auto alias_tok = expect(TokenKind::Identifier, "Expected alias name");
    std::string alias(alias_tok.lexeme);
    
    UseItem use_item;
    use_item.path = std::move(path);
    use_item.alias = std::move(alias);
    use_item.language = std::move(language);
    use_item.span = start;
    
    return Item::make(std::move(use_item));
}

Result<StmtPtr> Parser::parse_statement() {
    if (check(TokenKind::Let)) return parse_let_statement();
    if (check(TokenKind::Return)) return parse_return_statement();
    if (check(TokenKind::If)) return parse_if_statement();
    if (check(TokenKind::While)) return parse_while_statement();
    if (check(TokenKind::For)) return parse_for_statement();
    if (check(TokenKind::Match)) return parse_match_statement();
    if (check(TokenKind::Break)) {
        auto span = current().span;
        advance();
        return Statement::make(BreakStmt{span});
    }
    if (check(TokenKind::Continue)) {
        auto span = current().span;
        advance();
        return Statement::make(ContinueStmt{span});
    }
    
    return parse_expression_statement();
}

Result<StmtPtr> Parser::parse_let_statement() {
    SourceSpan start = current().span;
    expect(TokenKind::Let, "Expected 'let'");
    
    bool is_mutable = match(TokenKind::Mut);
    
    auto name_tok = expect(TokenKind::Identifier, "Expected variable name");
    std::string name(name_tok.lexeme);
    
    std::optional<Type> type;
    if (match(TokenKind::Colon)) {
        auto t = parse_type();
        if (!t) return std::unexpected(t.error());
        type = std::move(*t);
    }
    
    ExprPtr initializer;
    if (match(TokenKind::Eq)) {
        auto expr = parse_expression();
        if (!expr) return std::unexpected(expr.error());
        initializer = std::move(*expr);
    }
    
    LetStmt let;
    let.name = std::move(name);
    let.type = std::move(type);
    let.initializer = std::move(initializer);
    let.is_mutable = is_mutable;
    let.span = start;
    
    return Statement::make(std::move(let));
}

Result<StmtPtr> Parser::parse_return_statement() {
    SourceSpan start = current().span;
    expect(TokenKind::Return, "Expected 'return'");
    
    std::optional<ExprPtr> value;
    if (!check(TokenKind::RBrace) && !check(TokenKind::Semicolon) && !is_at_end()) {
        auto expr = parse_expression();
        if (!expr) return std::unexpected(expr.error());
        value = std::move(*expr);
    }
    
    ReturnStmt ret;
    ret.value = std::move(value);
    ret.span = start;
    
    return Statement::make(std::move(ret));
}

Result<StmtPtr> Parser::parse_if_statement() {
    SourceSpan start = current().span;
    expect(TokenKind::If, "Expected 'if'");
    
    auto condition = parse_expression();
    if (!condition) return std::unexpected(condition.error());
    
    auto then_block = parse_block();
    if (!then_block) return std::unexpected(then_block.error());
    
    std::vector<StmtPtr> else_block;
    if (match(TokenKind::Else)) {
        if (check(TokenKind::If)) {
            auto else_if = parse_if_statement();
            if (!else_if) return std::unexpected(else_if.error());
            else_block.push_back(std::move(*else_if));
        } else {
            auto block = parse_block();
            if (!block) return std::unexpected(block.error());
            else_block = std::move(*block);
        }
    }
    
    IfExpr if_expr;
    if_expr.condition = std::move(*condition);
    if_expr.then_block = std::move(*then_block);
    if_expr.else_block = std::move(else_block);
    if_expr.span = start;
    
    ExprStmt expr_stmt;
    expr_stmt.expression = Expression::make(std::move(if_expr));
    expr_stmt.span = start;
    
    return Statement::make(std::move(expr_stmt));
}

Result<StmtPtr> Parser::parse_while_statement() {
    SourceSpan start = current().span;
    expect(TokenKind::While, "Expected 'while'");
    
    auto condition = parse_expression();
    if (!condition) return std::unexpected(condition.error());
    
    auto body = parse_block();
    if (!body) return std::unexpected(body.error());
    
    WhileStmt while_stmt;
    while_stmt.condition = std::move(*condition);
    while_stmt.body = std::move(*body);
    while_stmt.span = start;
    
    return Statement::make(std::move(while_stmt));
}

Result<StmtPtr> Parser::parse_for_statement() {
    SourceSpan start = current().span;
    expect(TokenKind::For, "Expected 'for'");
    
    auto var_tok = expect(TokenKind::Identifier, "Expected loop variable");
    std::string variable(var_tok.lexeme);
    
    expect(TokenKind::In, "Expected 'in' after loop variable");
    
    auto iterator = parse_expression();
    if (!iterator) return std::unexpected(iterator.error());
    
    auto body = parse_block();
    if (!body) return std::unexpected(body.error());
    
    ForStmt for_stmt;
    for_stmt.variable = std::move(variable);
    for_stmt.iterator = std::move(*iterator);
    for_stmt.body = std::move(*body);
    for_stmt.span = start;
    
    return Statement::make(std::move(for_stmt));
}

Result<StmtPtr> Parser::parse_match_statement() {
    SourceSpan start = current().span;
    expect(TokenKind::Match, "Expected 'match'");
    
    auto scrutinee = parse_expression();
    if (!scrutinee) return std::unexpected(scrutinee.error());
    
    expect(TokenKind::LBrace, "Expected '{' after match expression");
    
    std::vector<MatchArm> arms;
    while (!check(TokenKind::RBrace) && !is_at_end()) {
        auto arm = parse_match_arm();
        if (!arm) return std::unexpected(arm.error());
        arms.push_back(std::move(*arm));
        (void)match(TokenKind::Comma); // Optional trailing comma
    }
    expect(TokenKind::RBrace, "Expected '}' after match arms");
    
    MatchExpr match_expr;
    match_expr.scrutinee = std::move(*scrutinee);
    match_expr.arms = std::move(arms);
    match_expr.span = start;
    
    ExprStmt expr_stmt;
    expr_stmt.expression = Expression::make(std::move(match_expr));
    expr_stmt.span = start;
    
    return Statement::make(std::move(expr_stmt));
}

Result<MatchArm> Parser::parse_match_arm() {
    SourceSpan start = current().span;
    
    auto pattern = parse_expression();
    if (!pattern) return std::unexpected(pattern.error());
    
    expect(TokenKind::FatArrow, "Expected '=>' after pattern");
    
    std::vector<StmtPtr> body;
    if (check(TokenKind::LBrace)) {
        auto block = parse_block();
        if (!block) return std::unexpected(block.error());
        body = std::move(*block);
    } else {
        auto expr = parse_expression();
        if (!expr) return std::unexpected(expr.error());
        ExprStmt expr_stmt;
        expr_stmt.expression = std::move(*expr);
        expr_stmt.span = start;
        body.push_back(Statement::make(std::move(expr_stmt)));
    }
    
    return MatchArm{std::move(*pattern), std::move(body), start};
}

Result<StmtPtr> Parser::parse_expression_statement() {
    SourceSpan start = current().span;
    
    auto expr = parse_expression();
    if (!expr) return std::unexpected(expr.error());
    
    // Check for assignment
    if (match(TokenKind::Eq)) {
        auto value = parse_expression();
        if (!value) return std::unexpected(value.error());
        
        AssignStmt assign;
        assign.target = std::move(*expr);
        assign.value = std::move(*value);
        assign.span = start;
        return Statement::make(std::move(assign));
    }
    
    ExprStmt expr_stmt;
    expr_stmt.expression = std::move(*expr);
    expr_stmt.span = start;
    
    return Statement::make(std::move(expr_stmt));
}

Result<std::vector<StmtPtr>> Parser::parse_block() {
    expect(TokenKind::LBrace, "Expected '{'");
    
    std::vector<StmtPtr> statements;
    while (!check(TokenKind::RBrace) && !is_at_end()) {
        auto stmt = parse_statement();
        if (!stmt) return std::unexpected(stmt.error());
        statements.push_back(std::move(*stmt));
    }
    
    expect(TokenKind::RBrace, "Expected '}'");
    return statements;
}

Result<ExprPtr> Parser::parse_expression() {
    return parse_assignment();
}

Result<ExprPtr> Parser::parse_assignment() {
    return parse_or();
}

Result<ExprPtr> Parser::parse_or() {
    auto left = parse_and();
    if (!left) return left;
    
    while (match(TokenKind::OrOr)) {
        SourceSpan start = previous().span;
        auto right = parse_and();
        if (!right) return right;
        
        BinaryExpr bin;
        bin.op = BinaryOp::Or;
        bin.left = std::move(*left);
        bin.right = std::move(*right);
        bin.span = start;
        left = Expression::make(std::move(bin));
    }
    
    return left;
}

Result<ExprPtr> Parser::parse_and() {
    auto left = parse_equality();
    if (!left) return left;
    
    while (match(TokenKind::AndAnd)) {
        SourceSpan start = previous().span;
        auto right = parse_equality();
        if (!right) return right;
        
        BinaryExpr bin;
        bin.op = BinaryOp::And;
        bin.left = std::move(*left);
        bin.right = std::move(*right);
        bin.span = start;
        left = Expression::make(std::move(bin));
    }
    
    return left;
}

Result<ExprPtr> Parser::parse_equality() {
    auto left = parse_comparison();
    if (!left) return left;
    
    while (match({TokenKind::EqEq, TokenKind::NotEq})) {
        BinaryOp op = previous().kind == TokenKind::EqEq ? BinaryOp::Eq : BinaryOp::NotEq;
        SourceSpan start = previous().span;
        auto right = parse_comparison();
        if (!right) return right;
        
        BinaryExpr bin;
        bin.op = op;
        bin.left = std::move(*left);
        bin.right = std::move(*right);
        bin.span = start;
        left = Expression::make(std::move(bin));
    }
    
    return left;
}

Result<ExprPtr> Parser::parse_comparison() {
    auto left = parse_bitwise_or();
    if (!left) return left;
    
    while (match({TokenKind::Lt, TokenKind::LtEq, TokenKind::Gt, TokenKind::GtEq})) {
        BinaryOp op;
        switch (previous().kind) {
            case TokenKind::Lt:   op = BinaryOp::Lt; break;
            case TokenKind::LtEq: op = BinaryOp::LtEq; break;
            case TokenKind::Gt:   op = BinaryOp::Gt; break;
            case TokenKind::GtEq: op = BinaryOp::GtEq; break;
            default: op = BinaryOp::Lt;
        }
        SourceSpan start = previous().span;
        auto right = parse_bitwise_or();
        if (!right) return right;
        
        BinaryExpr bin;
        bin.op = op;
        bin.left = std::move(*left);
        bin.right = std::move(*right);
        bin.span = start;
        left = Expression::make(std::move(bin));
    }
    
    return left;
}

Result<ExprPtr> Parser::parse_bitwise_or() {
    auto left = parse_bitwise_xor();
    if (!left) return left;
    
    while (match(TokenKind::Or)) {
        SourceSpan start = previous().span;
        auto right = parse_bitwise_xor();
        if (!right) return right;
        
        BinaryExpr bin;
        bin.op = BinaryOp::BitOr;
        bin.left = std::move(*left);
        bin.right = std::move(*right);
        bin.span = start;
        left = Expression::make(std::move(bin));
    }
    
    return left;
}

Result<ExprPtr> Parser::parse_bitwise_xor() {
    auto left = parse_bitwise_and();
    if (!left) return left;
    
    while (match(TokenKind::Xor)) {
        SourceSpan start = previous().span;
        auto right = parse_bitwise_and();
        if (!right) return right;
        
        BinaryExpr bin;
        bin.op = BinaryOp::BitXor;
        bin.left = std::move(*left);
        bin.right = std::move(*right);
        bin.span = start;
        left = Expression::make(std::move(bin));
    }
    
    return left;
}

Result<ExprPtr> Parser::parse_bitwise_and() {
    auto left = parse_shift();
    if (!left) return left;
    
    while (match(TokenKind::And)) {
        SourceSpan start = previous().span;
        auto right = parse_shift();
        if (!right) return right;
        
        BinaryExpr bin;
        bin.op = BinaryOp::BitAnd;
        bin.left = std::move(*left);
        bin.right = std::move(*right);
        bin.span = start;
        left = Expression::make(std::move(bin));
    }
    
    return left;
}

Result<ExprPtr> Parser::parse_shift() {
    auto left = parse_term();
    if (!left) return left;
    
    while (match({TokenKind::Shl, TokenKind::Shr})) {
        BinaryOp op = previous().kind == TokenKind::Shl ? BinaryOp::Shl : BinaryOp::Shr;
        SourceSpan start = previous().span;
        auto right = parse_term();
        if (!right) return right;
        
        BinaryExpr bin;
        bin.op = op;
        bin.left = std::move(*left);
        bin.right = std::move(*right);
        bin.span = start;
        left = Expression::make(std::move(bin));
    }
    
    return left;
}

Result<ExprPtr> Parser::parse_term() {
    auto left = parse_factor();
    if (!left) return left;
    
    while (match({TokenKind::Plus, TokenKind::Minus})) {
        BinaryOp op = previous().kind == TokenKind::Plus ? BinaryOp::Add : BinaryOp::Sub;
        SourceSpan start = previous().span;
        auto right = parse_factor();
        if (!right) return right;
        
        BinaryExpr bin;
        bin.op = op;
        bin.left = std::move(*left);
        bin.right = std::move(*right);
        bin.span = start;
        left = Expression::make(std::move(bin));
    }
    
    return left;
}

Result<ExprPtr> Parser::parse_factor() {
    auto left = parse_unary();
    if (!left) return left;
    
    while (match({TokenKind::Star, TokenKind::Slash, TokenKind::Percent})) {
        BinaryOp op;
        switch (previous().kind) {
            case TokenKind::Star:    op = BinaryOp::Mul; break;
            case TokenKind::Slash:   op = BinaryOp::Div; break;
            case TokenKind::Percent: op = BinaryOp::Mod; break;
            default: op = BinaryOp::Mul;
        }
        SourceSpan start = previous().span;
        auto right = parse_unary();
        if (!right) return right;
        
        BinaryExpr bin;
        bin.op = op;
        bin.left = std::move(*left);
        bin.right = std::move(*right);
        bin.span = start;
        left = Expression::make(std::move(bin));
    }
    
    return left;
}

Result<ExprPtr> Parser::parse_unary() {
    if (match({TokenKind::Bang, TokenKind::Minus, TokenKind::Tilde, TokenKind::And, TokenKind::Star})) {
        UnaryOp op;
        switch (previous().kind) {
            case TokenKind::Bang:  op = UnaryOp::Not; break;
            case TokenKind::Minus: op = UnaryOp::Neg; break;
            case TokenKind::Tilde: op = UnaryOp::BitNot; break;
            case TokenKind::And:   op = UnaryOp::Ref; break;
            case TokenKind::Star:  op = UnaryOp::Deref; break;
            default: op = UnaryOp::Not;
        }
        SourceSpan start = previous().span;
        auto operand = parse_unary();
        if (!operand) return operand;
        
        UnaryExpr un;
        un.op = op;
        un.operand = std::move(*operand);
        un.span = start;
        return Expression::make(std::move(un));
    }
    
    return parse_call();
}

Result<ExprPtr> Parser::parse_call() {
    auto expr = parse_primary();
    if (!expr) return expr;
    
    while (true) {
        if (match(TokenKind::LParen)) {
            SourceSpan start = previous().span;
            auto args = parse_arguments();
            if (!args) return std::unexpected(args.error());
            expect(TokenKind::RParen, "Expected ')' after arguments");
            
            // Get name from variable expression
            std::string name;
            if (auto* var = std::get_if<VariableExpr>(&(*expr)->variant)) {
                name = var->name;
            }
            
            CallExpr call;
            call.name = std::move(name);
            call.args = std::move(*args);
            call.span = start;
            expr = Expression::make(std::move(call));
        } else if (match(TokenKind::Dot)) {
            SourceSpan start = previous().span;
            auto member_tok = expect(TokenKind::Identifier, "Expected member name");
            
            MemberExpr member;
            member.object = std::move(*expr);
            member.member = std::string(member_tok.lexeme);
            member.span = start;
            expr = Expression::make(std::move(member));
        } else if (match(TokenKind::LBracket)) {
            SourceSpan start = previous().span;
            auto index = parse_expression();
            if (!index) return index;
            expect(TokenKind::RBracket, "Expected ']' after index");
            
            IndexExpr idx;
            idx.object = std::move(*expr);
            idx.index = std::move(*index);
            idx.span = start;
            expr = Expression::make(std::move(idx));
        } else {
            break;
        }
    }
    
    return expr;
}

Result<ExprPtr> Parser::parse_primary() {
    SourceSpan start = current().span;
    
    // Literals
    if (match(TokenKind::IntLiteral)) {
        LiteralExpr lit;
        lit.value = previous().value;
        lit.span = previous().span;
        return Expression::make(std::move(lit));
    }
    if (match(TokenKind::FloatLiteral)) {
        LiteralExpr lit;
        lit.value = previous().value;
        lit.span = previous().span;
        return Expression::make(std::move(lit));
    }
    if (match(TokenKind::StringLiteral)) {
        LiteralExpr lit;
        lit.value = previous().value;
        lit.span = previous().span;
        return Expression::make(std::move(lit));
    }
    if (match(TokenKind::CharLiteral)) {
        LiteralExpr lit;
        lit.value = previous().value;
        lit.span = previous().span;
        return Expression::make(std::move(lit));
    }
    if (match(TokenKind::True) || match(TokenKind::False)) {
        LiteralExpr lit;
        lit.value = std::int64_t{previous().kind == TokenKind::True ? 1 : 0};
        lit.span = previous().span;
        return Expression::make(std::move(lit));
    }
    
    // Identifier
    if (match(TokenKind::Identifier)) {
        VariableExpr var;
        var.name = std::string(previous().lexeme);
        var.span = previous().span;
        return Expression::make(std::move(var));
    }
    
    // Self
    if (match(TokenKind::Self)) {
        VariableExpr var;
        var.name = "self";
        var.span = previous().span;
        return Expression::make(std::move(var));
    }
    
    // Parenthesized expression
    if (match(TokenKind::LParen)) {
        auto expr = parse_expression();
        if (!expr) return expr;
        expect(TokenKind::RParen, "Expected ')' after expression");
        return expr;
    }
    
    // Array literal
    if (match(TokenKind::LBracket)) {
        std::vector<ExprPtr> elements;
        if (!check(TokenKind::RBracket)) {
            do {
                auto elem = parse_expression();
                if (!elem) return elem;
                elements.push_back(std::move(*elem));
            } while (match(TokenKind::Comma));
        }
        expect(TokenKind::RBracket, "Expected ']' after array elements");
        
        ArrayExpr arr;
        arr.elements = std::move(elements);
        arr.span = start;
        return Expression::make(std::move(arr));
    }
    
    // Block expression
    if (check(TokenKind::LBrace)) {
        auto block = parse_block();
        if (!block) return std::unexpected(block.error());
        
        BlockExpr blk;
        blk.statements = std::move(*block);
        blk.span = start;
        return Expression::make(std::move(blk));
    }
    
    return std::unexpected(error("E0002", "Expected expression")
        .with_label(current().span, "unexpected token"));
}

Result<std::vector<ExprPtr>> Parser::parse_arguments() {
    std::vector<ExprPtr> args;
    
    if (!check(TokenKind::RParen)) {
        do {
            auto arg = parse_expression();
            if (!arg) return std::unexpected(arg.error());
            args.push_back(std::move(*arg));
        } while (match(TokenKind::Comma));
    }
    
    return args;
}

Result<Type> Parser::parse_type() {
    SourceSpan start = current().span;
    
    bool is_reference = match(TokenKind::And);
    bool is_mutable = match(TokenKind::Mut);
    
    auto name_tok = expect(TokenKind::Identifier, "Expected type name");
    std::string name(name_tok.lexeme);
    
    std::vector<Type> generics;
    if (match(TokenKind::Lt)) {
        do {
            auto generic = parse_type();
            if (!generic) return generic;
            generics.push_back(std::move(*generic));
        } while (match(TokenKind::Comma));
        expect(TokenKind::Gt, "Expected '>' after generic parameters");
    }
    
    return Type{std::move(name), std::move(generics), is_reference, is_mutable, start};
}

Result<Parameter> Parser::parse_parameter() {
    SourceSpan start = current().span;
    
    // Handle 'self' parameter
    if (match(TokenKind::Self)) {
        return Parameter{"self", Type::primitive("Self", start), start};
    }
    
    auto name_tok = expect(TokenKind::Identifier, "Expected parameter name");
    std::string name(name_tok.lexeme);
    
    expect(TokenKind::Colon, "Expected ':' after parameter name");
    
    auto type = parse_type();
    if (!type) return std::unexpected(type.error());
    
    return Parameter{std::move(name), std::move(*type), start};
}

Result<Field> Parser::parse_field() {
    SourceSpan start = current().span;
    bool is_public = match(TokenKind::Pub);
    
    auto name_tok = expect(TokenKind::Identifier, "Expected field name");
    std::string name(name_tok.lexeme);
    
    expect(TokenKind::Colon, "Expected ':' after field name");
    
    auto type = parse_type();
    if (!type) return std::unexpected(type.error());
    
    return Field{std::move(name), std::move(*type), is_public, start};
}

} // namespace idotc
