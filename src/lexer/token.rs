use logos::Logos;

#[derive(Logos, Debug, Clone, PartialEq)]
#[logos(skip r"[ \t\n\f]+")]
pub enum TokenKind {
    // Keywords
    #[token("use")]
    Use,
    #[token("fn")]
    Fn,
    #[token("let")]
    Let,
    #[token("mut")]
    Mut,
    #[token("struct")]
    Struct,
    #[token("class")]
    Class,
    #[token("trait")]
    Trait,
    #[token("impl")]
    Impl,
    #[token("if")]
    If,
    #[token("else")]
    Else,
    #[token("while")]
    While,
    #[token("for")]
    For,
    #[token("in")]
    In,
    #[token("match")]
    Match,
    #[token("return")]
    Return,
    #[token("break")]
    Break,
    #[token("continue")]
    Continue,
    #[token("as")]
    As,
    #[token("true")]
    True,
    #[token("false")]
    False,
    #[token("legacy")]
    Legacy,
    #[token("py")]
    Py,
    #[token("js")]
    Js,

    // Operators
    #[token("+")]
    Plus,
    #[token("-")]
    Minus,
    #[token("*")]
    Star,
    #[token("/")]
    Slash,
    #[token("%")]
    Percent,
    #[token("==")]
    EqEq,
    #[token("!=")]
    NotEq,
    #[token("<")]
    Lt,
    #[token(">")]
    Gt,
    #[token("<=")]
    LtEq,
    #[token(">=")]
    GtEq,
    #[token("&&")]
    AndAnd,
    #[token("||")]
    OrOr,
    #[token("!")]
    Bang,
    #[token("&")]
    And,
    #[token("|")]
    Or,
    #[token("^")]
    Xor,
    #[token("<<")]
    Shl,
    #[token(">>")]
    Shr,
    #[token("=")]
    Eq,
    #[token("+=")]
    PlusEq,
    #[token("-=")]
    MinusEq,
    #[token("*=")]
    StarEq,
    #[token("/=")]
    SlashEq,

    // Delimiters
    #[token("(")]
    LParen,
    #[token(")")]
    RParen,
    #[token("{")]
    LBrace,
    #[token("}")]
    RBrace,
    #[token("[")]
    LBracket,
    #[token("]")]
    RBracket,
    #[token(",")]
    Comma,
    #[token(";")]
    Semicolon,
    #[token(":")]
    Colon,
    #[token("::")]
    ColonColon,
    #[token(".")]
    Dot,
    #[token("->")]
    Arrow,
    #[token("=>")]
    FatArrow,

    // Literals
    #[regex(r"[0-9]+", priority = 2)]
    Integer,
    #[regex(r"[0-9]+\.[0-9]+")]
    Float,
    #[regex(r#""([^"\\]|\\.)*""#)]
    String,
    #[regex(r"'([^'\\]|\\.)'")]
    Char,

    // Identifiers
    #[regex(r"[a-zA-Z_][a-zA-Z0-9_]*")]
    Ident,

    // Comments (skip them)
    #[regex(r"//[^\n]*", logos::skip)]
    #[regex(r"/\*([^*]|\*[^/])*\*/", logos::skip)]
    Comment,

    // Error (removed - no longer needed in logos 0.14+)

}

#[derive(Debug, Clone, PartialEq)]
pub struct Token {
    pub kind: TokenKind,
    pub text: String,
    pub span: (usize, usize),
}

impl Token {
    pub fn new(kind: TokenKind, text: String, span: (usize, usize)) -> Self {
        Self { kind, text, span }
    }
}

pub struct Lexer<'source> {
    lexer: logos::Lexer<'source, TokenKind>,
}

impl<'source> Lexer<'source> {
    pub fn new(source: &'source str) -> Self {
        Self {
            lexer: TokenKind::lexer(source),
        }
    }

    pub fn tokenize(source: &'source str) -> Vec<Token> {
        let mut tokens = Vec::new();
        let mut lexer = Self::new(source);

        while let Some(token) = lexer.next() {
            tokens.push(token);
        }

        tokens
    }
}

impl<'source> Iterator for Lexer<'source> {
    type Item = Token;

    fn next(&mut self) -> Option<Self::Item> {
        self.lexer.next().map(|result| {
            let kind = result.ok()?;
            let text = self.lexer.slice().to_string();
            let span = self.lexer.span();
            Some(Token::new(kind, text, (span.start, span.end)))
        }).flatten()
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_keywords() {
        let source = "use fn let struct if else while";
        let tokens = Lexer::tokenize(source);
        assert_eq!(tokens[0].kind, TokenKind::Use);
        assert_eq!(tokens[1].kind, TokenKind::Fn);
        assert_eq!(tokens[2].kind, TokenKind::Let);
        assert_eq!(tokens[3].kind, TokenKind::Struct);
        assert_eq!(tokens[4].kind, TokenKind::If);
        assert_eq!(tokens[5].kind, TokenKind::Else);
        assert_eq!(tokens[6].kind, TokenKind::While);
    }

    #[test]
    fn test_operators() {
        let source = "+ - * / == != < > <= >=";
        let tokens = Lexer::tokenize(source);
        assert_eq!(tokens[0].kind, TokenKind::Plus);
        assert_eq!(tokens[1].kind, TokenKind::Minus);
        assert_eq!(tokens[2].kind, TokenKind::Star);
        assert_eq!(tokens[3].kind, TokenKind::Slash);
        assert_eq!(tokens[4].kind, TokenKind::EqEq);
        assert_eq!(tokens[5].kind, TokenKind::NotEq);
    }

    #[test]
    fn test_literals() {
        let source = r#"42 3.14 "hello" 'x'"#;
        let tokens = Lexer::tokenize(source);
        assert_eq!(tokens[0].kind, TokenKind::Integer);
        assert_eq!(tokens[1].kind, TokenKind::Float);
        assert_eq!(tokens[2].kind, TokenKind::String);
        assert_eq!(tokens[3].kind, TokenKind::Char);
    }

    #[test]
    fn test_use_statement() {
        let source = r#"use "example.idot" as example"#;
        let tokens = Lexer::tokenize(source);
        assert_eq!(tokens[0].kind, TokenKind::Use);
        assert_eq!(tokens[1].kind, TokenKind::String);
        assert_eq!(tokens[2].kind, TokenKind::As);
        assert_eq!(tokens[3].kind, TokenKind::Ident);
    }
}
