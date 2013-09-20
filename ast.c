#include "nolli.h"

static astnode_t* create_node(ast_type_t type)
{
    astnode_t* a = alloc(sizeof(*a));
    a->type = type;
    return a;
}


astnode_t* make_bool_lit(token_t t)
{
    astnode_t* bn = create_node(AST_BOOL_LIT);
    return bn;
}

astnode_t* make_char_lit(token_t t)
{
    astnode_t* cn = create_node(AST_CHAR_LIT);
    return cn;
}

astnode_t* make_int_num(token_t t)
{
    astnode_t* ln = create_node(AST_INT_NUM);
    return ln;
}

astnode_t* make_real_num(token_t t)
{
    astnode_t* dn = create_node(AST_REAL_NUM);
    return dn;
}

astnode_t* make_str_lit(token_t t)
{
    astnode_t* sn = create_node(AST_STR_LIT);
    return sn;
}

astnode_t* make_ident(token_t name)
{
    astnode_t* node = create_node(AST_IDENT);
    return node;
}

astnode_t* make_typedef(type_t* t, astnode_t* id)
{
    astnode_t* node = create_node(AST_TYPEDEF);
    return node;
}

astnode_t* make_decl(type_t* t, astnode_t* id)
{
    astnode_t* dn = create_node(AST_DECL);
    return dn;
}

astnode_t* make_unexpr(expr_op_t op, astnode_t* expr)
{
    astnode_t* node = create_node(AST_UNEXPR);
    return node;
}

astnode_t* make_binexpr(expr_op_t op, astnode_t* exA, astnode_t* exB)
{
    astnode_t* node = create_node(AST_BINEXPR);
    return node;
}

astnode_t* make_list(astnode_t* list, astnode_t* item)
{
    astnode_t* node = create_node(AST_LIST);
    return node;
}

astnode_t* make_map(astnode_t* items, astnode_t* kv)
{
    astnode_t* node = create_node(AST_MAP);
    return node;
}

astnode_t* make_mapkv(astnode_t* key, astnode_t* val)
{
    astnode_t* node = create_node(AST_MAPKV);
    return node;
}

astnode_t* make_contaccess(astnode_t* cont, astnode_t* idx)
{
    astnode_t* node = create_node(AST_CONTACCESS);
    return node;
}

astnode_t* make_assignment(astnode_t* id, assign_op_t op, astnode_t* expr)
{
    astnode_t* node = create_node(AST_ASSIGN);
    return node;
}

astnode_t* make_contassign(astnode_t* cont, astnode_t* idx,
        assign_op_t op, astnode_t* item)
{
    astnode_t* node = create_node(AST_CONTASSIGN);
    return node;
}

astnode_t* make_ifelse(astnode_t* cond, astnode_t* t, astnode_t* f)
{
    astnode_t* node = create_node(AST_IFELSE);
    return node;
}

astnode_t* make_while(astnode_t* cond, astnode_t* s)
{
    astnode_t* node = create_node(AST_WHILE);
    return node;
}

astnode_t* make_until(astnode_t* cond, astnode_t* s)
{
    astnode_t* node = create_node(AST_UNTIL);
    return node;
}

astnode_t* make_for(astnode_t* id, astnode_t* iter, astnode_t* s)
{
    astnode_t* node = create_node(AST_FOR);
    return node;
}

astnode_t* make_call(astnode_t* func, astnode_t* args)
{
    astnode_t* node = create_node(AST_CALL);
    return node;
}

astnode_t* make_statements(astnode_t* list, astnode_t* statement)
{
    astnode_t* node = create_node(AST_STATEMENTS);
    return node;
}

astnode_t* make_module(astnode_t* id, astnode_t* statements)
{
    astnode_t* node = create_node(AST_MODULE);
    return node;
}
