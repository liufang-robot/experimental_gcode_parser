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
    | if_goto_stmt
    | if_block_start_stmt
    | else_stmt
    | endif_stmt
    | while_stmt
    | endwhile_stmt
    | for_stmt
    | endfor_stmt
    | repeat_stmt
    | until_stmt
    | loop_stmt
    | endloop_stmt
    | goto_stmt
    | label_stmt
    | item+
    ;

assignment_stmt
    : WORD ASSIGN expr
    ;

label_stmt
    : WORD COLON
    ;

goto_stmt
    : jump_keyword goto_target
    ;

if_goto_stmt
    : IF_KW condition THEN_KW? goto_stmt (ELSE_KW goto_stmt)?
    ;

if_block_start_stmt
    : IF_KW condition
    ;

else_stmt
    : ELSE_KW
    ;

endif_stmt
    : ENDIF_KW
    ;

while_stmt
    : WHILE_KW condition
    ;

endwhile_stmt
    : ENDWHILE_KW
    ;

for_stmt
    : FOR_KW WORD ASSIGN expr TO_KW expr
    ;

endfor_stmt
    : ENDFOR_KW
    ;

repeat_stmt
    : REPEAT_KW
    ;

until_stmt
    : UNTIL_KW condition
    ;

loop_stmt
    : LOOP_KW
    ;

endloop_stmt
    : ENDLOOP_KW
    ;

condition
    : first=condition_term (ops+=AND_KW rest+=condition_term)*
    ;

condition_term
    : lhs=expr (op=comparison_op rhs=expr)?
    | COMMENT
    ;

comparison_op
    : EQ_OP
    | NE_OP
    | GE_OP
    | LE_OP
    | GT_OP
    | LT_OP
    ;

jump_keyword
    : GOTO_KW
    | GOTOF_KW
    | GOTOB_KW
    | GOTOC_KW
    ;

goto_target
    : WORD
    | LINE_NUMBER
    | NUMBER
    | SYSTEM_VAR
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

COLON
    : ':'
    ;

EQ_OP
    : '=='
    ;

NE_OP
    : '<>'
    ;

GE_OP
    : '>='
    ;

LE_OP
    : '<='
    ;

GT_OP
    : '>'
    ;

LT_OP
    : '<'
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

IF_KW
    : [Ii] [Ff]
    ;

THEN_KW
    : [Tt] [Hh] [Ee] [Nn]
    ;

ELSE_KW
    : [Ee] [Ll] [Ss] [Ee]
    ;

AND_KW
    : [Aa] [Nn] [Dd]
    ;

GOTO_KW
    : [Gg] [Oo] [Tt] [Oo]
    ;

GOTOF_KW
    : [Gg] [Oo] [Tt] [Oo] [Ff]
    ;

GOTOB_KW
    : [Gg] [Oo] [Tt] [Oo] [Bb]
    ;

GOTOC_KW
    : [Gg] [Oo] [Tt] [Oo] [Cc]
    ;

ENDIF_KW
    : [Ee] [Nn] [Dd] [Ii] [Ff]
    ;

WHILE_KW
    : [Ww] [Hh] [Ii] [Ll] [Ee]
    ;

ENDWHILE_KW
    : [Ee] [Nn] [Dd] [Ww] [Hh] [Ii] [Ll] [Ee]
    ;

FOR_KW
    : [Ff] [Oo] [Rr]
    ;

TO_KW
    : [Tt] [Oo]
    ;

ENDFOR_KW
    : [Ee] [Nn] [Dd] [Ff] [Oo] [Rr]
    ;

REPEAT_KW
    : [Rr] [Ee] [Pp] [Ee] [Aa] [Tt]
    ;

UNTIL_KW
    : [Uu] [Nn] [Tt] [Ii] [Ll]
    ;

LOOP_KW
    : [Ll] [Oo] [Oo] [Pp]
    ;

ENDLOOP_KW
    : [Ee] [Nn] [Dd] [Ll] [Oo] [Oo] [Pp]
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
    : [A-MO-Za-mo-z_] [A-Za-z0-9_]*
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
