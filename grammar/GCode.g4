grammar GCode;

program
    : line* line_no_eol? EOF
    ;

line
    : block_delete? line_number? item* EOL
    | block_delete? line_number? EOL
    ;

line_no_eol
    : block_delete? line_number? item*
    ;

block_delete
    : BLOCK_DELETE
    ;

line_number
    : LINE_NUMBER
    ;

item
    : WORD
    | COMMENT
    ;

BLOCK_DELETE
    : '/'
    ;

LINE_NUMBER
    : [Nn] DIGIT+
    ;

WORD
    : WORD_HEAD (EQUAL WORD_VALUE | WORD_VALUE)?
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
