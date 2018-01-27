/*
 * parser.c - c11 parsing and lexing functions
 *
 * AUTHOR: Joey Pabalinas <alyptik@protonmail.com>
 * See LICENSE.md file for copyright and license details.
 */

#include "parser.h"
#include <stdlib.h>
#include <string.h>

/* symbol types */
enum sym_type {
	START_RULE = 0,
	TRANSLATION_UNIT, COMMENT, EXTERNAL_DECLARATION,
	FUNCTION_DEFINITION, DECLARATION, DECLARATION_SPECIFIERS,
	DECLARATOR, COMPOUND_STATEMENT, BLOCK_ITEM_LIST, BLOCK_ITEM,
	STATEMENT, STATEMENT_LIST, JUMP_STATEMENT, ITERATION_STATEMENT,
	SELECTION_STATEMENT, LABELED_STATEMENT, EXPRESSION_STATEMENT,
	FOR_EXPRESSION, FOR_INCREMENT, FOR_INITIALIZATION,
	CONDITIONAL_EXPRESSION, CONDITIONAL_TERNARY_EXPRESSION,
	ASSIGNMENT_OPERATOR, CONSTANT_EXPRESSION, LOGICAL_OR_AND_EXPRESSION,
	LOG_OR_AND_BIT_OR_AND_EQ, REL_MUL_ADD_EX_OP, UNARY_OPERATOR,
	CLOSURE, STATIC_ASSERT, STATIC_ASSERT_DECLARATION,
	DECLARATION_LIST, INIT_DECLARATOR_LIST, INIT_DECLARATOR,
	INITIALIZER, INITIALIZER_LIST, DESIGNATION, DESIGNATOR_LIST,
	DESIGNATOR, UNARY_EXPRESSION, ALIGNOF, POSTFIX_PRODUCTIONS,
	POSTFIX_EXPRESSION, POSTFIX_SUFFIX, ARGUMENT_EXPRESSION_LIST,
	NARROW_CLOSURE, PRIMARY_EXPRESSION, GENERIC_SELECTION,
	GENERIC_ASSOC_LIST, GENERIC_ASSOCIATION, ALIGNAS, ALIGNMENT_SPECIFIER,
	ARRAY_QUALIFIERS, ARRAY_DECLARATOR, IDENTIFIER_LIST,
	PARAMETER_TYPE_LIST, PARAMETER_LIST, PARAMETER_DECLARATION,
	ABSTRACT_DECLARATOR, DIRECT_ABSTRACT_DECLARATOR, DAD, IDENTIFIER,
	POINTER, TYPE_QUALIFIER_LIST, FUNCTION_SPECIFIER,
	STORAGE_CLASS_SPECIFIER, TYPE_QUALIFIER, ATOMIC_TYPE_SPECIFIER,
	TYPE_SPECIFIER, TYPEDEF_NAME, STRUCT_OR_UNION_SPECIFIER,
	STRUCT_DECLARATION_LIST, STRUCT_DECLARATION, TYPE_NAME,
	SPECIFIER_QUALIFIER_LIST, STRUCT_OR_UNION, ENUM_SPECIFIER,
	ENUMERATOR_LIST, ENUMERATOR, COMMENT_C, COMMENT_CXX,
	CONSTANT, IDENTIFIER_WORD, RESERVED,
};

/* prototypes */
void start_rule(void);
void translation_unit_rule(void);
void comment_rule(void);
void external_declaration_rule(void);
void function_definition_rule(void);
void function_definition_rule(void);
void declaration_specifiers_rule(void);
void declarator_rule(void);
void compound_statement_rule(void);
void declaration_rule(void);
void block_item_list_rule(void);
void block_item_rule(void);
void statement_rule(void);
void statement_list_rule(void);
void jump_statement_rule(void);
void iteration_statement_rule(void);
void selection_statement_rule(void);
void labeled_statement_rule(void);
void expression_statement_rule(void);
void for_expression_rule(void);
void for_increment_rule(void);
void for_initialization_rule(void);
void conditional_expression_rule(void);
void conditional_ternary_expression_rule(void);
void assignment_operator_rule(void);
void constant_expression_rule(void);
void logical_or_and_expression_rule(void);
void log_or_and_bit_or_and_eq_rule(void);
void rel_mul_add_ex_op_rule(void);
void unary_operator_rule(void);
void closure_rule(void);
void static_assert_rule(void);
void static_assert_declaration_rule(void);
void declaration_list_rule(void);
void init_declarator_list_rule(void);
void init_declarator_rule(void);
void initializer_rule(void);
void initializer_list_rule(void);
void designation_rule(void);
void designator_list_rule(void);
void designator_rule(void);
void unary_expression_rule(void);
void alignof_rule(void);
void postfix_productions_rule(void);
void postfix_expression_rule(void);
void postfix_suffix_rule(void);
void argument_expression_list_rule(void);
void narrow_closure_rule(void);
void primary_expression_rule(void);
void generic_selection_rule(void);
void generic_assoc_list_rule(void);
void generic_association_rule(void);
void alignas_rule(void);
void alignment_specifier_rule(void);
void array_qualifiers_rule(void);
void array_declarator_rule(void);
void identifier_list_rule(void);
void parameter_type_list_rule(void);
void parameter_list_rule(void);
void parameter_declaration_rule(void);
void abstract_declarator_rule(void);
void direct_abstract_declarator_rule(void);
void dad_rule(void);
void identifier_rule(void);
void pointer_rule(void);
void type_qualifier_list_rule(void);
void function_specifier_rule(void);
void storage_class_specifier_rule(void);
void type_qualifier_rule(void);
void atomic_type_specifier_rule(void);
void type_specifier_rule(void);
void typedef_name_rule(void);
void struct_or_union_specifier_rule(void);
void struct_declaration_list_rule(void);
void struct_declaration_rule(void);
void type_name_rule(void);
void specifier_qualifier_list_rule(void);
void struct_or_union_rule(void);
void enum_specifier_rule(void);
void enumerator_list_rule(void);
void enumerator_rule(void);
void comment_c_rule(void);
void comment_cxx_rule(void);
void constant_rule(void);
void identifier_word_rule(void);
void reserved_rule(void);

struct prod_rule {
	char *name;
	void (*action)(void);
};

/* globals */
struct var_table objects;

/* tables */
struct prod_rule symbols[] = {
	[START_RULE] = {"START_RULE", start_rule},
	/*
	 *   TRANSLATION_UNIT
	 * + START_RULE (?)
	 */
	[TRANSLATION_UNIT] = {"TRANSLATION_UNIT", translation_unit_rule},
	/*
	 *   COMMENT
	 * | EXTERNAL_DECLARATION
	 * | FUNCTION_DEFINITION
	 */
	[COMMENT] = {"COMMENT", comment_rule},
	[EXTERNAL_DECLARATION] = {"EXTERNAL_DECLARATION", external_declaration_rule},
	/*
	 *   DECLARATION
	 */
	[FUNCTION_DEFINITION] = {"FUNCTION_DEFINITION", function_definition_rule},
	/*
	 *   DECLARATION_SPECIFIERS[CONTEXT => 'FUNCTION DEFINITION'](?)
	 * + DECLARATOR[CONTEXT => 'FUNCTION DEFINITION']
	 * + COMPOUND_STATEMENT[CONTEXT => 'FUNCTION DEFINITION STATEMENT'](?)
	 */
	[DECLARATION_SPECIFIERS] = {"DECLARATION_SPECIFIERS", declaration_specifiers_rule},
	[DECLARATOR] = {"DECLARATOR", declarator_rule},
	[COMPOUND_STATEMENT] = {"COMPOUND_STATEMENT", compound_statement_rule},
	/*
	 *   '{'
	 * + block_item_list(s?)
	 * + '_rule}'
	 */
	[DECLARATION] = {"DECLARATION", declaration_rule},
	[BLOCK_ITEM_LIST] = {"BLOCK_ITEM_LIST", block_item_list_rule},
	/*
	 *   BLOCK_ITEM(s)
	 */
	[BLOCK_ITEM] = {"BLOCK_ITEM", block_item_rule},
	/*
	 *   DECLARATION
	 * | STATEMENT[CONTEXT => "$ARG{CONTEXT}|BLOCK ITEM"]
	 * | COMMENT
	 */
	[STATEMENT] = {"STATEMENT", statement_rule},
	/*
	 *   JUMP_STATEMENT
	 * | COMPOUND_STATEMENT
	 * | ITERATION_STATEMENT
	 * | SELECTION_STATEMENT
	 * | LABELED_STATEMENT
	 * | EXPRESSION_STATEMENT
	 */
	[STATEMENT_LIST] = {"STATEMENT_LIST", statement_list_rule},
	/*
	 *   COMMENT(?)
	 * + STATEMENT[CONTEXT => UNDEF]
	 * + STATEMENT_LIST(?)
	 */
	[JUMP_STATEMENT] = {"JUMP_STATEMENT", jump_statement_rule},
	[ITERATION_STATEMENT] = {"ITERATION_STATEMENT", iteration_statement_rule},
	[SELECTION_STATEMENT] = {"SELECTION_STATEMENT", selection_statement_rule},
	[LABELED_STATEMENT] = {"LABELED_STATEMENT", labeled_statement_rule},
	[EXPRESSION_STATEMENT] = {"EXPRESSION_STATEMENT", expression_statement_rule},
	[FOR_EXPRESSION] = {"FOR_EXPRESSION", for_expression_rule},
	[FOR_INCREMENT] = {"FOR_INCREMENT", for_increment_rule},
	[FOR_INITIALIZATION] = {"FOR_INITIALIZATION", for_initialization_rule},
	[CONDITIONAL_EXPRESSION] = {"CONDITIONAL_EXPRESSION", conditional_expression_rule},
	[CONDITIONAL_TERNARY_EXPRESSION] = {"CONDITIONAL_TERNARY_EXPRESSION", conditional_ternary_expression_rule},
	[ASSIGNMENT_OPERATOR] = {"ASSIGNMENT_OPERATOR", assignment_operator_rule},
	[CONSTANT_EXPRESSION] = {"CONSTANT_EXPRESSION", constant_expression_rule},
	[LOGICAL_OR_AND_EXPRESSION] = {"LOGICAL_OR_AND_EXPRESSION", logical_or_and_expression_rule},
	[LOG_OR_AND_BIT_OR_AND_EQ] = {"LOG_OR_AND_BIT_OR_AND_EQ", log_or_and_bit_or_and_eq_rule},
	[REL_MUL_ADD_EX_OP] = {"REL_MUL_ADD_EX_OP", rel_mul_add_ex_op_rule},
	[UNARY_OPERATOR] = {"UNARY_OPERATOR", unary_operator_rule},
	[CLOSURE] = {"CLOSURE", closure_rule},
	[STATIC_ASSERT] = {"STATIC_ASSERT", static_assert_rule},
	[STATIC_ASSERT_DECLARATION] = {"STATIC_ASSERT_DECLARATION", static_assert_declaration_rule},
	[DECLARATION_LIST] = {"DECLARATION_LIST", declaration_list_rule},
	[INIT_DECLARATOR_LIST] = {"INIT_DECLARATOR_LIST", init_declarator_list_rule},
	[INIT_DECLARATOR] = {"INIT_DECLARATOR", init_declarator_rule},
	[INITIALIZER] = {"INITIALIZER", initializer_rule},
	[INITIALIZER_LIST] = {"INITIALIZER_LIST", initializer_list_rule},
	[DESIGNATION] = {"DESIGNATION", designation_rule},
	[DESIGNATOR_LIST] = {"DESIGNATOR_LIST", designator_list_rule},
	[DESIGNATOR] = {"DESIGNATOR", designator_rule},
	[UNARY_EXPRESSION] = {"UNARY_EXPRESSION", unary_expression_rule},
	[ALIGNOF] = {"ALIGNOF", alignof_rule},
	[POSTFIX_PRODUCTIONS] = {"POSTFIX_PRODUCTIONS", postfix_productions_rule},
	[POSTFIX_EXPRESSION] = {"POSTFIX_EXPRESSION", postfix_expression_rule},
	[POSTFIX_SUFFIX] = {"POSTFIX_SUFFIX", postfix_suffix_rule},
	[ARGUMENT_EXPRESSION_LIST] = {"ARGUMENT_EXPRESSION_LIST", argument_expression_list_rule},
	[NARROW_CLOSURE] = {"NARROW_CLOSURE", narrow_closure_rule},
	[PRIMARY_EXPRESSION] = {"PRIMARY_EXPRESSION", primary_expression_rule},
	[GENERIC_SELECTION] = {"GENERIC_SELECTION", generic_selection_rule},
	[GENERIC_ASSOC_LIST] = {"GENERIC_ASSOC_LIST", generic_assoc_list_rule},
	[GENERIC_ASSOCIATION] = {"GENERIC_ASSOCIATION", generic_association_rule},
	[ALIGNAS] = {"ALIGNAS", alignas_rule},
	[ALIGNMENT_SPECIFIER] = {"ALIGNMENT_SPECIFIER", alignment_specifier_rule},
	[ARRAY_QUALIFIERS] = {"ARRAY_QUALIFIERS", array_qualifiers_rule},
	[ARRAY_DECLARATOR] = {"ARRAY_DECLARATOR", array_declarator_rule},
	[IDENTIFIER_LIST] = {"IDENTIFIER_LIST", identifier_list_rule},
	[PARAMETER_TYPE_LIST] = {"PARAMETER_TYPE_LIST", parameter_type_list_rule},
	[PARAMETER_LIST] = {"PARAMETER_LIST", parameter_list_rule},
	[PARAMETER_DECLARATION] = {"PARAMETER_DECLARATION", parameter_declaration_rule},
	[ABSTRACT_DECLARATOR] = {"ABSTRACT_DECLARATOR", abstract_declarator_rule},
	[DIRECT_ABSTRACT_DECLARATOR] = {"DIRECT_ABSTRACT_DECLARATOR", direct_abstract_declarator_rule},
	[DAD] = {"DAD", dad_rule},
	[IDENTIFIER] = {"IDENTIFIER", identifier_rule},
	[POINTER] = {"POINTER", pointer_rule},
	[TYPE_QUALIFIER_LIST] = {"TYPE_QUALIFIER_LIST", type_qualifier_list_rule},
	[FUNCTION_SPECIFIER] = {"FUNCTION_SPECIFIER", function_specifier_rule},
	[STORAGE_CLASS_SPECIFIER] = {"STORAGE_CLASS_SPECIFIER", storage_class_specifier_rule},
	[TYPE_QUALIFIER] = {"TYPE_QUALIFIER", type_qualifier_rule},
	[ATOMIC_TYPE_SPECIFIER] = {"ATOMIC_TYPE_SPECIFIER", atomic_type_specifier_rule},
	[TYPE_SPECIFIER] = {"TYPE_SPECIFIER", type_specifier_rule},
	[TYPEDEF_NAME] = {"TYPEDEF_NAME", typedef_name_rule},
	[STRUCT_OR_UNION_SPECIFIER] = {"STRUCT_OR_UNION_SPECIFIER", struct_or_union_specifier_rule},
	[STRUCT_DECLARATION_LIST] = {"STRUCT_DECLARATION_LIST", struct_declaration_list_rule},
	[STRUCT_DECLARATION] = {"STRUCT_DECLARATION", struct_declaration_rule},
	[TYPE_NAME] = {"TYPE_NAME", type_name_rule},
	[SPECIFIER_QUALIFIER_LIST] = {"SPECIFIER_QUALIFIER_LIST", specifier_qualifier_list_rule},
	[STRUCT_OR_UNION] = {"STRUCT_OR_UNION", struct_or_union_rule},
	[ENUM_SPECIFIER] = {"ENUM_SPECIFIER", enum_specifier_rule},
	[ENUMERATOR_LIST] = {"ENUMERATOR_LIST", enumerator_list_rule},
	[ENUMERATOR] = {"ENUMERATOR", enumerator_rule},
	[COMMENT_C] = {"COMMENT_C", comment_c_rule},
	[COMMENT_CXX] = {"COMMENT_CXX", comment_cxx_rule},
	[CONSTANT] = {"CONSTANT", constant_rule},
	[IDENTIFIER_WORD] = {"IDENTIFIER_WORD", identifier_word_rule},
	[RESERVED] = {"RESERVED", reserved_rule},
};

/* current symbol */
static size_t sym_idx;
static struct str_list tok_list;

static inline void next_sym(void)
{
	sym_idx++;
#ifdef _DEBUG
	printf("prev/cur: (%s, %s)\n", tok_list.list[sym_idx - 1], tok_list.list[sym_idx]);
#endif
}

static inline bool accept(enum sym_type type)
{
	if (type) {
		next_sym();
		return true;
	}
	return false;
}

static inline bool expect(enum sym_type type)
{
	if (accept(type))
		return true;
	WARNX("%s: %s", "failure parsing symbol", symbols[type].name);
	return false;
}

void translation_unit_rule(void)
{
	(void)tok_list, (void)expect, (void)start_rule;
}

void start_rule(void)
{
	translation_unit_rule();
}
