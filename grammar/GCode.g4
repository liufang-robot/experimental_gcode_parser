grammar GCode;

program
    : line* line_no_eol? EOF
    ;

line
    : block_delete? line_number? statement? EOL
    | block_delete? line_number? EOL
    ;

line_no_eol
    : block_delete? line_number? statement
    | block_delete line_number?
    | line_number
    ;

statement
    : assignment_stmt
    | item+
    ;

assignment_stmt
    : WORD ASSIGN expr
    ;

expr
    : additive_expr
    ;

additive_expr
    : first=multiplicative_expr (ops+=(PLUS | MINUS) rest+=multiplicative_expr)*
    ;

multiplicative_expr
    : first=unary_expr (ops+=(STAR | SLASH) rest+=unary_expr)*
    ;

unary_expr
    : op=(PLUS | MINUS) unary_expr
    | primary_expr
    ;

primary_expr
    : NUMBER
    | WORD
    | SYSTEM_VAR
    ;

block_delete
    : SLASH
    ;

line_number
    : LINE_NUMBER
    ;

item
    : WORD
    | COMMENT
    ;

ASSIGN
    : '='
    ;

PLUS
    : '+'
    ;

MINUS
    : '-'
    ;

STAR
    : '*'
    ;

SLASH
    : '/'
    ;

LINE_NUMBER
    : [Nn] DIGIT+
    ;

WORD
    : WORD_HEAD (EQUAL WORD_VALUE | WORD_VALUE)?
    ;

SYSTEM_VAR
    : '$' [A-Za-z_] [A-Za-z0-9_]*
    ;

NUMBER
    : UNSIGNED_REAL
    | UNSIGNED_INTEGER
    ;

COMMENT
    : '(' ~(')')* ')'
    | ';' ~([\r\n])*
    ;

EOL
    : '\r\n'
    | '\r'
    | '\n'
    ;

WS
    : [ \t]+ -> skip
    ;

fragment WORD_HEAD
    : [A-MO-Za-mo-z] [A-Za-z0-9]*
    ;

fragment WORD_VALUE
    : AC_NUMBER
    | REAL_NUMBER
    | INTEGER
    ;

fragment EQUAL
    : '='
    ;

fragment UNSIGNED_REAL
    : DIGIT+ DECIMAL_POINT DIGIT*
    | DECIMAL_POINT DIGIT+
    ;

fragment UNSIGNED_INTEGER
    : DIGIT+
    ;

fragment REAL_NUMBER
    : SIGN? DIGIT+ DECIMAL_POINT DIGIT*
    | SIGN? DECIMAL_POINT DIGIT+
    ;

fragment AC_NUMBER
    : [Aa][Cc] '(' SIGN? DIGIT+ DECIMAL_POINT DIGIT* ')'
    | [Aa][Cc] '(' SIGN? DIGIT+ ')'
    | [Aa][Cc] '(' SIGN? DECIMAL_POINT DIGIT+ ')'
    ;

fragment INTEGER
    : SIGN? DIGIT+
    ;

fragment SIGN
    : [+-]
    ;

fragment DECIMAL_POINT
    : '.'
    ;

fragment DIGIT
    : [0-9]
    ;

