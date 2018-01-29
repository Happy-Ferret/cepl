# C-to-English Grammar
# Pragmatic Software
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

use strict;
use warnings;

STARTRULE:
      TRANSLATION_UNIT
      STARTRULE(?)

TRANSLATION_UNIT:
      COMMENT
    | EXTERNAL_DECLARATION
    | FUNCTION_DEFINITION
    | PREPROC[MATCHRULE => 'translation_unit']

PREPROC:
      '#' (DEFINITION
        | UNDEFINITION
        | INCLUSION
        | LINE
        | ERROR
        | PRAGMA
        | PREPROC_CONDITIONAL[MATCHRULE => $ARG{MATCHRULE}])

DEFINITION:
      MACRO_DEFINITION
    | 'define' IDENTIFIER TOKEN_SEQUENCE(?) <SKIP: '[ \t]*'> "\N"

MACRO_DEFINITION:
      'define' IDENTIFIER '(' <LEFTOP: IDENTIFIER ',' IDENTIFIER> ')' TOKEN_SEQUENCE <SKIP: '[ \t]*'> "\N"

UNDEFINITION:
      'undef' IDENTIFIER <SKIP: '[ \t]*'> "\N"

INCLUSION:
      'include' '<' FILENAME '>' <SKIP: '[ \t]*'> "\N"
    | 'include' '"' FILENAME '"' <SKIP: '[ \t]*'> "\N"
    | 'include' TOKEN

FILENAME:
      /[_\.\-\W\/]+/

LINE:
      'line' CONSTANT ('"' FILENAME '"'
      )(?) <SKIP: '[ \t]*'> "\N"

ERROR:
      'error' TOKEN_SEQUENCE(?) <SKIP: '[ \t]*'> "\N"

PRAGMA:
      'pragma' TOKEN_SEQUENCE(?) <SKIP: '[ \t]*'> "\N"

PREPROC_CONDITIONAL:
      IF_LINE[MATCHRULE => $ARG{MATCHRULE}]
      <MATCHRULE: $RULE_NAME>(S?)
      (ELIF_PARTS[MATCHRULE => $RULE_NAME])(?)
      (ELSE_PARTS[MATCHRULE => $RULE_NAME])(?)
      '#' 'endif'

IF_LINE:
      'ifdef' IDENTIFIER <SKIP: '[ \t]*'> "\N"
    | 'ifndef' IDENTIFIER <SKIP: '[ \t]*'> "\N"
    | 'if' CONSTANT_EXPRESSION <SKIP: '[ \t]*'> "\N"

ELIF_PARTS:
      ('#' 'elif' CONSTANT_EXPRESSION
      (<MATCHRULE: $RULE_NAME> )[MATCHRULE => $ARG{MATCHRULE}](S?)
      )(S)

ELSE_PARTS:
      '#' 'else'
      (<MATCHRULE: $RULE_NAME>)[MATCHRULE => $ARG{MATCHRULE}](S?)

TOKEN_SEQUENCE:
      TOKEN(S)

TOKEN:
      <SKIP: '[ \t]*'> /\S+/

EXTERNAL_DECLARATION:
      DECLARATION[CONTEXT => 'external declaration']

FUNCTION_DEFINITION:
      DECLARATION_SPECIFIERS[CONTEXT => 'function definition'](?)
    + DECLARATOR[CONTEXT => 'function definition']
    + COMPOUND_STATEMENT[CONTEXT => 'function definition statement'](?)

BLOCK_ITEM_LIST:
      BLOCK_ITEM(S)

BLOCK_ITEM:
      DECLARATION
    | STATEMENT[CONTEXT => "$ARG{CONTEXT}|BLOCK ITEM"]
    | PREPROC
    | COMMENT

COMPOUND_STATEMENT:
      '{' BLOCK_ITEM_LIST(S?) '}'

STATEMENT_LIST:
      COMMENT(?)
    + PREPROC[MATCHRULE => 'statement'](?)
    + STATEMENT[CONTEXT => UNDEF]
    + STATEMENT_LIST(?)

STATEMENT:
      JUMP_STATEMENT
    | COMPOUND_STATEMENT
    | ITERATION_STATEMENT
    | SELECTION_STATEMENT
    | LABELED_STATEMENT
    | EXPRESSION_STATEMENT

ITERATION_STATEMENT:
      'for' '(' <COMMIT> FOR_INITIALIZATION(?)
    + FOR_EXPRESSION(?)
    + FOR_INCREMENT(?)
    + ')'
    + STATEMENT[CONTEXT => 'for loop']
    | 'while' '(' <COMMIT> EXPRESSION[CONTEXT => 'while conditional']  ')'
    +  STATEMENT[CONTEXT => 'while loop']
    | 'do' STATEMENT[CONTEXT => 'do loop'] 'while' '(' EXPRESSION[CONTEXT => 'do while conditional'] ')' ';'

FOR_INITIALIZATION:
      DECLARATION[CONTEXT => 'for init']
    | EXPRESSION_STATEMENT[CONTEXT => 'for init']

FOR_EXPRESSION:
      EXPRESSION_STATEMENT[CONTEXT => 'for conditional']

FOR_INCREMENT:
      EXPRESSION[CONTEXT => 'for increment statement']

SELECTION_STATEMENT:
      'if' <COMMIT> '(' EXPRESSION[CONTEXT => 'if conditional']
    + ')' STATEMENT[CONTEXT => "$ARG{CONTEXT}|if statement"]
    + ('else' STATEMENT[CONTEXT => "$ARG{CONTEXT}|else block"])(?)
    | 'switch'  '(' EXPRESSION[CONTEXT => 'switch conditional']
    + ')'  STATEMENT[CONTEXT => "$ARG{CONTEXT}|switch"]

JUMP_STATEMENT:
      'break' ';'
    | 'continue' ';'
    | 'return' <COMMIT> EXPRESSION[CONTEXT => "$ARG{CONTEXT}|RETURN EXPRESSION"](?) ';'
    | 'goto' <COMMIT> IDENTIFIER ';' COMMENT(?)

EXPRESSION_STATEMENT:
      EXPRESSION[CONTEXT => "$ARG{CONTEXT}|STATEMENT"](?) ';'

LABELED_STATEMENT:
      IDENTIFIER ':' STATEMENT[CONTEXT => 'label'] (';')(?)
    | ('case' CONSTANT_EXPRESSION
        ':')(S)
        (STATEMENT[CONTEXT => "$ARG{CONTEXT}|CASE"])(S)
    | 'default' ':' STATEMENT

EXPRESSION:
      <LEFTOP: ASSIGNMENT_EXPRESSION ',' ASSIGNMENT_EXPRESSION>

ASSIGNMENT_EXPRESSION:
      UNARY_EXPRESSION[CONTEXT => 'assignment expression']
        ASSIGNMENT_OPERATOR
        ASSIGNMENT_EXPRESSION[CONTEXT => 'assignment expression']
    | CONDITIONAL_EXPRESSION

CONDITIONAL_EXPRESSION:
      LOGICAL_OR_AND_EXPRESSION CONDITIONAL_TERNARY_EXPRESSION

CONDITIONAL_TERNARY_EXPRESSION:
      '?' EXPRESSION ':' CONDITIONAL_EXPRESSION
    | {""}

ASSIGNMENT_OPERATOR:
      '='
    | '+='
    | '-='
    | '*='
    | '/='
    | '%='
    | '<<='
    | '>>='
    | '&='
    | '^='
    | '|='

CONSTANT_EXPRESSION:
      CONDITIONAL_EXPRESSION

LOGICAL_OR_AND_EXPRESSION:
      <LEFTOP:
        REL_ADD_MUL_SHIFT_EXPRESSION
        LOG_OR_AND_BIT_OR_AND_EQ
        REL_ADD_MUL_SHIFT_EXPRESSION[CONTEXT => 'logical_or_and_expression']>

LOG_OR_AND_BIT_OR_AND_EQ:
      '||'
      | '&&'
      | '|'
      | '&'
      | '^'
      | '=='
      | '!='

REL_MUL_ADD_EX_OP:
      '+'
      | '-'
      | '*'
      | '/'
      | '%'
      | '<<'
      | '>>'
      | '>='
      | "<="
      | '>'
      | '<'

UNARY_OPERATOR:
      '&'
      | '*'
      | '+'
      | '-' ...IDENTIFIER
      | '-'
      | '~'
      | '!' '!'
      | '!'

REL_ADD_MUL_SHIFT_EXPRESSION:
      CAST_EXPRESSION ...';'
    | <LEFTOP: CAST_EXPRESSION REL_MUL_ADD_EX_OP CAST_EXPRESSION>

CLOSURE:
      ',' | ';' | ')'

CAST_EXPRESSION:
      '(' TYPE_NAME ')' CAST_EXPRESSION[CONTEXT => 'recast']
    | UNARY_EXPRESSION

STATIC_ASSERT:
      '_Static_assert'
    | 'static_assert'

STATIC_ASSERT_DECLARATION:
      STATIC_ASSERT '(' CONSTANT_EXPRESSION[CONTEXT => 'static assert'] ',' STRING ')' ';'

DECLARATION_LIST:
      PREPROC[CONTEXT => 'statement'](?) DECLARATION(S)

DECLARATION:
      DECLARATION_SPECIFIERS INIT_DECLARATOR_LIST(?) ';'
    | STATIC_ASSERT_DECLARATION

INIT_DECLARATOR_LIST:
      <LEFTOP: INIT_DECLARATOR ',' INIT_DECLARATOR>

INIT_DECLARATOR:
      DECLARATOR[CONTEXT => "$ARG{CONTEXT}|INIT_DECLARATOR"]
      ('=' INITIALIZER)(?)

INITIALIZER:
      DESIGNATION INITIALIZER
    | COMMENT(?)
    + ASSIGNMENT_EXPRESSION[CONTEXT => "$ARG{CONTEXT}|INITIALIZER EXPRESSION"]
    + COMMENT(?)
    | '{' COMMENT(?) INITIALIZER_LIST (',' )(?) '}'

INITIALIZER_LIST:
      <LEFTOP: INITIALIZER ',' INITIALIZER >

DESIGNATION:
      DESIGNATOR_LIST '='

DESIGNATOR_LIST:
      DESIGNATOR(S)

DESIGNATOR:
      '[' CONSTANT_EXPRESSION ']'
    | '.' IDENTIFIER

UNARY_EXPRESSION:
      POSTFIX_EXPRESSION
    | '++' UNARY_EXPRESSION
    | '--' UNARY_EXPRESSION
    | UNARY_OPERATOR CAST_EXPRESSION
    | 'sizeof' UNARY_EXPRESSION[CONTEXT => 'sizeof']
    | 'sizeof' '(' TYPE_NAME[CONTEXT => 'sizeof'] ')'
    | 'sizeof' '(' ASSIGNMENT_EXPRESSION[CONTEXT => 'sizeof'] ')'
    | ALIGNOF '(' TYPE_NAME ')'
    | 'offsetof' '(' TYPE_NAME[CONTEXT => 'offsetof'] ',' IDENTIFIER ')'

ALIGNOF:
      '_Alignof'
    | 'alignof'

POSTFIX_PRODUCTIONS:
      '(' ARGUMENT_EXPRESSION_LIST(?)
    + ')' POSTFIX_PRODUCTIONS[CONTEXT => 'function call'](?)
    | '[' EXPRESSION[CONTEXT => 'array address']
    + ']' POSTFIX_PRODUCTIONS[CONTEXT => "$ARG{CONTEXT}|ARRAY ADDRESS"](?)
    | '.' IDENTIFIER POSTFIX_PRODUCTIONS[CONTEXT => "$ARG{CONTEXT}|STRUCT ACCESS"](?)
    | '->' IDENTIFIER POSTFIX_PRODUCTIONS[CONTEXT => "$ARG{CONTEXT}|STRUCT ACCESS"](?)
    | ('++')(S)
    | ('--')(S)
    | {""}

POSTFIX_EXPRESSION:
      '(' TYPE_NAME ')' '{' INITIALIZER_LIST '}'
    + POSTFIX_PRODUCTIONS[CONTEXT => "$ARG{CONTEXT}|COMPOUND LITERAL"](?)
    | PRIMARY_EXPRESSION POSTFIX_PRODUCTIONS[PRIMARY_EXPRESSION => $ITEM[1], CONTEXT => $ARG{CONTEXT}]

POSTFIX_SUFFIX:
      '[' EXPRESSION ']'
    | '.' IDENTIFIER
    | '->' IDENTIFIER
    | '++'
    | '--'

ARGUMENT_EXPRESSION_LIST:
      <LEFTOP: ASSIGNMENT_EXPRESSION[CONTEXT => 'function argument']
    + ',' ASSIGNMENT_EXPRESSION[CONTEXT => 'function argument']>

NARROW_CLOSURE:
      ';' | ',' | '->'

PRIMARY_EXPRESSION:
      '(' EXPRESSION[CONTEXT => "$ARG{CONTEXT}|EXPRESSION"]
    + ')' (...NARROW_CLOSURE)(?)
    | CONSTANT
    | STRING
    | IDENTIFIER
    | GENERIC_SELECTION
    | {} # NOTHING

GENERIC_SELECTION:
      '_Generic' '(' ASSIGNMENT_EXPRESSION ',' GENERIC_ASSOC_LIST ')'

GENERIC_ASSOC_LIST:
      <LEFTOP: GENERIC_ASSOCIATION ',' GENERIC_ASSOCIATION>

GENERIC_ASSOCIATION:
      TYPE_NAME ':' ASSIGNMENT_EXPRESSION
    | 'default' ':' ASSIGNMENT_EXPRESSION

ALIGNAS:
      '_Alignas'
    | 'alignas'

ALIGNMENT_SPECIFIER:
      ALIGNAS '(' TYPE_NAME ')'
    | ALIGNAS '(' CONSTANT_EXPRESSION ')'

DECLARATOR:
      DIRECT_DECLARATOR(S)
    | POINTER DIRECT_DECLARATOR(S)

DIRECT_DECLARATOR:
      IDENTIFIER ':' CONSTANT
    | IDENTIFIER[CONTEXT => 'direct_declarator'] ARRAY_DECLARATOR(S?)
    | '(' DECLARATOR ')' ARRAY_DECLARATOR(S)
    | '(' PARAMETER_TYPE_LIST ')'
    | '(' DECLARATOR ARRAY_DECLARATOR(S) ')'
    | '(' DECLARATOR ')'

ARRAY_QUALIFIERS:
      TYPE_QUALIFIER_LIST ARRAY_QUALIFIERS(?)
    | 'static' ARRAY_QUALIFIERS(?)

ARRAY_DECLARATOR:
      '[' ARRAY_QUALIFIERS(?) ASSIGNMENT_EXPRESSION(?) ']'
    | '[' '*' ']'

IDENTIFIER_LIST:
      (IDENTIFIER ',')(S?) IDENTIFIER

PARAMETER_TYPE_LIST:
      PARAMETER_LIST

PARAMETER_LIST:
      <LEFTOP: PARAMETER_DECLARATION ',' PARAMETER_DECLARATION>

PARAMETER_DECLARATION:
      DECLARATION_SPECIFIERS DECLARATOR
    | '...'
    | DECLARATION_SPECIFIERS ABSTRACT_DECLARATOR(?)
    | ''

ABSTRACT_DECLARATOR:
      POINTER(?) DIRECT_ABSTRACT_DECLARATOR(S)
    | POINTER

DIRECT_ABSTRACT_DECLARATOR:
      '(' ABSTRACT_DECLARATOR ')'
    | '[' ']'
    | '[' '*' ']'
    | '[' ARRAY_QUALIFIERS(?) ASSIGNMENT_EXPRESSION(?) ']'
    | DAD '[' ']'
    | DAD '[' ARRAY_QUALIFIERS(?) ASSIGNMENT_EXPRESSION(?) ']'
    | '(' ')'
    | '(' PARAMETER_TYPE_LIST ')'
    | DAD '(' ')'
    | DAD '(' PARAMETER_TYPE_LIST ')'

DAD: # MACRO FOR DIRECT_ABSTRACT_DECLARATOR
      ( '(' ABSTRACT_DECLARATOR ')' )(S?)
      ( '[' ']' )(S?)
      ( '[' ASSIGNMENT_EXPRESSION ']' )(S?)
      ( '(' ')' )(S?)
      ( '(' PARAMETER_TYPE_LIST ')' )(S?)

IDENTIFIER:
      ...!RESERVED IDENTIFIER_WORD

POINTER:
      '*' TYPE_QUALIFIER_LIST(S) POINTER(?)
    | '*' POINTER(?)

TYPE_QUALIFIER_LIST:
      TYPE_QUALIFIER(S)

FUNCTION_SPECIFIER:
      'inline'
    | '_Noreturn'
    | 'noreturn'

DECLARATION_SPECIFIERS:
      COMMENT[CONTEXT => 'declaration_specifiers'] DECLARATION_SPECIFIERS(S)
    | TYPE_SPECIFIER ...IDENTIFIER
    | STORAGE_CLASS_SPECIFIER DECLARATION_SPECIFIERS(?)
    | TYPE_SPECIFIER(S) DECLARATION_SPECIFIERS(?)
    | TYPE_QUALIFIER DECLARATION_SPECIFIERS(?)
    | FUNCTION_SPECIFIER DECLARATION_SPECIFIERS(?)
    | ALIGNMENT_SPECIFIER(S) DECLARATION_SPECIFIERS(?)

STORAGE_CLASS_SPECIFIER:
      'auto'
    | 'extern'
    | 'static'
    | 'register'
    | 'typedef'

TYPE_QUALIFIER:
      'const'
    | 'volatile'
    | 'restrict'
    | '_Atomic'

ATOMIC_TYPE_SPECIFIER:
      '_Atomic' '(' TYPE_NAME ')'

TYPE_SPECIFIER:
      <SKIP:''> /\S*/
        ('void'
          | 'signed' | 'unsigned'
          | 'file' | 'fpos_t'
          | 'bool' | '_Bool'
          | '_Complex' | '_Imaginary'
          | 'int_fast8_t'   | 'int_fast16_t'   | 'int_fast24_t'
          | 'int_fast32_t'   | 'int_fast64_t'   | 'int_fast128_t'
          | 'uint_fast8_t'  | 'uint_fast16_t'  | 'uint_fast24_t'
          | 'uint_fast32_t'  | 'uint_fast64_t'  | 'uint_fast128_t'
          | 'int_least8_t'  | 'int_least16_t'  | 'int_least24_t'
          | 'int_least32_t'  | 'int_least64_t'  | 'int_least128_t'
          | 'uint_least8_t' | 'uint_least16_t' | 'uint_least24_t'
          | 'uint_least32_t' | 'uint_least64_t' | 'uint_least128_t'
          | 'int8_t'   | 'int16_t'  | 'int24_t'  | 'int32_t'
          | 'int64_t'  | 'int128_t' | 'uint8_t'  | 'uint16_t'
          | 'uint24_t' | 'uint32_t' | 'uint64_t' | 'uint128_t'
          | 'intmax_t' | 'uintmax_t'
          | 'intptr_t' | 'uintptr_t' | 'ptrdiff_t'
          | 'sig_atomic_t'
          | 'wint_t' | 'wchar_t'
          | 'size_t' | 'rsize_t' | 'max_align_t'
          | 'mbstate_t' | 'char16_t' | 'char32_t'
          | 'fenv_t' | 'fexcept_t'
          | 'div_t' | 'ldiv_t' | 'lldiv_t' | 'imaxdiv_t'
          | 'cnd_t' | 'thrd_t' | 'tss_t' | 'mtx_t' | 'tss_dtor_t'
          | 'thrd_start_t' | 'once_flag'
          | 'clock_t' | 'time_t'
          | <SKIP:'[\s]*'> STRUCT_OR_UNION_SPECIFIER
          | <SKIP:'[\s]*'> ENUM_SPECIFIER
          | <SKIP:'[\s]*'> ATOMIC_TYPE_SPECIFIER | TYPEDEF_NAME
          | 'double' | 'float' | 'char' | 'short' | 'int' | 'long'
        ) .../\W/

TYPEDEF_NAME:
      IDENTIFIER

STRUCT_OR_UNION_SPECIFIER:
      COMMENT(?) STRUCT_OR_UNION IDENTIFIER(?) '{' STRUCT_DECLARATION_LIST '}'
    | STRUCT_OR_UNION IDENTIFIER

STRUCT_DECLARATION_LIST:
      STRUCT_DECLARATION(S)

STRUCT_DECLARATION:
      COMMENT(S?) DECLARATION[CONTEXT => 'struct member'] COMMENT(S?)

TYPE_NAME:
      SPECIFIER_QUALIFIER_LIST
    + ABSTRACT_DECLARATOR(?)

SPECIFIER_QUALIFIER_LIST:
      TYPE_SPECIFIER
    + SPECIFIER_QUALIFIER_LIST(?)
    | TYPE_QUALIFIER
    + SPECIFIER_QUALIFIER_LIST(?)

STRUCT_OR_UNION:
      COMMENT(?)
    + ('struct' | 'union')
    + COMMENT(?)

ENUM_SPECIFIER:
      'enum' IDENTIFIER(?) '{' ENUMERATOR_LIST '}'
    | 'enum' IDENTIFIER

ENUMERATOR_LIST:
      <LEFTOP:ENUMERATOR ',' ENUMERATOR>

ENUMERATOR:
      IDENTIFIER ( '=' CONSTANT_EXPRESSION )(?)

COMMENT:
      COMMENT_C
    | COMMENT_CXX

COMMENT_C:
      M{/\*[^*]*\*+([^/*][^*]*\*+)*/}S

COMMENT_CXX:
      M{//(.*?)\N}

CONSTANT:
      /-?[0-9]*\.[0-9]*[LF]{0,2}/I
    | /0X[0-9A-F]+[LU]{0,3}/I
    | /0\D+[LU]{0,3}/I
    | /-?[0-9]+[LU]{0,3}/I # INTEGER CONSTANT
    | /[LUu]?(?:\'(?:\\\'|(?!\').)*\')/ # CHARACTER CONSTANT

IDENTIFIER_WORD:
      /[A-Z_\$][A-Z0-9_]*/I

STRING:
      (/(U8|U|u|L)?(?:\"(?:\\\"|(?!\").)*\")/)(S)

RESERVED:
    /(auto|break|case|char|const|continue|default|do|double|else|enum|extern|float|for|goto
       |if|inline|int|long|register|restrict|return|short|signed|sizeof|static|struct|switch|typedef
       |union|unsigned|void|volatile|while|_Alignas|alignas|_Alignof|alignof|_Atomic|_Bool|_Complex|_Generic
       |_Imaginary|_Noreturn|noreturn|_Static_Assert|static_Assert|_Thread_Local|offsetof)\b/x

