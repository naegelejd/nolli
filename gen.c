#include "nolli.h"
#include "ast.h"
#include "type.h"
#include "symtable.h"
#include "debug.h"

/* FIXME: need lexer.h to look up tokens */
#include "lexer.h"

#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Target.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/BitWriter.h>

#include <assert.h>

#define JIT_DEBUGF(J, fmt, ...) \
    NL_DEBUGF((J)->ctx, fmt, __VA_ARGS__)
#define JIT_DEBUG(P, S) JIT_DEBUGF(P, "%s", S)

#define JIT_ERRORF(J, n, fmt, ...) \
    NL_ERRORF((J)->ctx, NL_ERR_JIT, fmt " near line %d", __VA_ARGS__, (n)->lineno)
#define JIT_ERROR(J, n, ...) JIT_ERRORF(J, n, "%s", __VA_ARGS__)

struct jit {
    struct nl_context* ctx;
    LLVMModuleRef mod;
    LLVMBuilderRef builder;
    struct nl_symtable* named_values;
};

static void jit_node(struct jit*, struct nl_ast*);
static LLVMValueRef jit_expr(struct jit* jit, struct nl_ast* node);


/* static void jit_unit(struct jit* jit, struct nl_ast* node) */
/* { */
/*     assert(node->tag == NL_AST_UNIT); */
/*     JIT_DEBUG(jit, "JITing unit"); */

/*     if (node->unit.packages == NULL) { */
/*         JIT_ERROR(jit, node, "unit's packages are NULL"); */
/*     } */
/*     jit_node(jit, node->unit.packages); */
/* } */

static LLVMValueRef jit_ident(struct jit* jit, struct nl_ast* node)
{
    assert(node->tag == NL_AST_IDENT);

    LLVMValueRef val = nl_symtable_search(jit->named_values, node->s);
    if (NULL == val) {
        JIT_ERRORF(jit, node, "no such variable: %s", node->s);
        return NULL; /* TODO: exit JIT */
    }
    return LLVMBuildLoad(jit->builder, val, node->s);
}

static LLVMValueRef jit_bin_int_expr(struct jit* jit, struct nl_ast* node,
        LLVMValueRef lhs, LLVMValueRef rhs)
{
    LLVMValueRef result;
    switch (node->binexpr.op) {
        case TOK_ADD:
            result = LLVMBuildAdd(jit->builder, lhs, rhs, "tmp.add");
            break;
        case TOK_SUB:
            result = LLVMBuildSub(jit->builder, lhs, rhs, "tmp.sub");
            break;
        case TOK_MUL:
            result = LLVMBuildMul(jit->builder, lhs, rhs, "tmp.mul");
            break;
        case TOK_DIV:
            result = LLVMBuildSDiv(jit->builder, lhs, rhs, "tmp.div");
            break;
        case TOK_LT:
            result = LLVMBuildICmp(jit->builder, LLVMIntSLT, lhs, rhs, "tmp.lt");
            break;
        case TOK_LTE:
            result = LLVMBuildICmp(jit->builder, LLVMIntSLE, lhs, rhs, "tmp.le");
            break;
        case TOK_GT:
            result = LLVMBuildICmp(jit->builder, LLVMIntSGT, lhs, rhs, "tmp.gt");
            break;
        case TOK_GTE:
            result = LLVMBuildICmp(jit->builder, LLVMIntSGE, lhs, rhs, "tmp.ge");
            break;
        case TOK_EQ:
            result = LLVMBuildICmp(jit->builder, LLVMIntEQ, lhs, rhs, "tmp.eq");
            break;
        case TOK_NEQ:
            result = LLVMBuildICmp(jit->builder, LLVMIntNE, lhs, rhs, "tmp.ne");
            break;
        default:
            JIT_ERROR(jit, node, "unsupported int binary operation");
            result = NULL;
    }
    return result;
}

static LLVMValueRef jit_bin_real_expr(struct jit* jit, struct nl_ast* node,
        LLVMValueRef lhs, LLVMValueRef rhs)
{
    LLVMValueRef result;
    switch (node->binexpr.op) {
        case TOK_ADD:
            result = LLVMBuildFAdd(jit->builder, lhs, rhs, "tmp.fadd");
            break;
        case TOK_SUB:
            result = LLVMBuildFSub(jit->builder, lhs, rhs, "tmp.fsub");
            break;
        case TOK_MUL:
            result = LLVMBuildFMul(jit->builder, lhs, rhs, "tmp.fmul");
            break;
        case TOK_DIV:
            result = LLVMBuildFDiv(jit->builder, lhs, rhs, "tmp.fdiv");
            break;
        default:
            JIT_ERROR(jit, node, "unsupported real binary operation");
            result = NULL;
    }
    return result;
}

static LLVMValueRef jit_bin_expr(struct jit* jit, struct nl_ast* node)
{
    assert(node->tag == NL_AST_BINEXPR);

    LLVMValueRef lhs = jit_expr(jit, node->binexpr.lhs);
    LLVMValueRef rhs = jit_expr(jit, node->binexpr.rhs);

    struct nl_type* lhs_type = node->binexpr.lhs->type;

    LLVMValueRef result;
    switch (lhs_type->tag) {
        case NL_TYPE_INT:
            result = jit_bin_int_expr(jit, node, lhs, rhs);
            break;
        case NL_TYPE_REAL:
            result = jit_bin_real_expr(jit, node, lhs, rhs);
            break;
        default:
            JIT_ERROR(jit, node, "unsupported binary operand type");
            return NULL;
    }

    return result;
}

static LLVMValueRef jit_call(struct jit* jit, struct nl_ast* node)
{
    assert(node->tag == NL_AST_CALL);

    const char* name = node->call.func->s;
    LLVMValueRef callee = LLVMGetNamedFunction(jit->mod, name);
    if (!callee) {
        JIT_ERROR(jit, node, "unknown function reference");
        return NULL;    // TODO: exit JIT
    }

    struct nl_ast_list* args_list = &node->call.args->list;

    LLVMValueRef *args = nl_alloc(jit->ctx, sizeof(*args) * args_list->count);
    struct nl_ast *arg = args_list->head;
    int i = 0;
    while (arg != NULL) {
        args[i++] = jit_expr(jit, arg);
        arg = arg->next;
    }

    return LLVMBuildCall(jit->builder, callee, args, args_list->count, "tmp");
}

static LLVMValueRef jit_expr(struct jit* jit, struct nl_ast* node)
{
    LLVMValueRef expr;
    switch (node->tag) {
    case NL_AST_BOOL_LIT:
        if (node->b) {
            expr = LLVMConstInt(LLVMInt1Type(), 1, false);
        } else {
            expr = LLVMConstInt(LLVMInt1Type(), 0, false);
        }
        break;
    case NL_AST_INT_LIT:
        expr = LLVMConstInt(LLVMInt64Type(), node->l, true);
        break;
    case NL_AST_REAL_LIT:
        expr = LLVMConstReal(LLVMDoubleType(), node->d);
        break;
    case NL_AST_IDENT:
        expr = jit_ident(jit, node);
        break;
    case NL_AST_BINEXPR:
        expr = jit_bin_expr(jit, node);
        break;
    case NL_AST_CALL:
        expr = jit_call(jit, node);
        break;
    default:
        JIT_ERROR(jit, node, "expression not yet supported");
        return NULL;
    }

    return expr;
}

static void jit_decl(struct jit* jit, struct nl_ast* node)
{
    assert(node->tag == NL_AST_DECL);
    /*
     * node->decl.type
     * node->decl.rhs
     * node->decl.tp // var/const
     */

    struct nl_ast* rhs = node->decl.rhs;
    if (rhs->tag == NL_AST_LIST_DECLS) {
        // TODO: decl list
    } else {
        assert(rhs->tag == NL_AST_IDENT);
        // TODO: initializers

        const char *varname = rhs->s;
        struct nl_ast* decl_type = node->decl.type;

        LLVMTypeRef type;
        LLVMValueRef value;
        switch (decl_type->type->tag) {
        case NL_TYPE_BOOL:
            type = LLVMInt1Type();
            value = LLVMConstInt(type, 0, false);
            break;
        case NL_TYPE_INT:
            type = LLVMInt64Type();
            value = LLVMConstInt(type, 0, false);
            break;
        case NL_TYPE_REAL:
            type = LLVMDoubleType();
            value = LLVMConstReal(type, 0.0);
            break;
        default:
            JIT_ERROR(jit, node, "decl type not yet supported");
            return;     /* TODO: exit JIT */
        }

        LLVMValueRef alloca = LLVMBuildAlloca(jit->builder, type, varname);
        LLVMBuildStore(jit->builder, value, alloca);

        /* save this variable binding */
        nl_symtable_add(jit->named_values, (nl_string_t)varname, alloca);
    }
}

static void jit_bind(struct jit* jit, struct nl_ast* node)
{
    assert(node->tag == NL_AST_BIND);

    LLVMValueRef bind_value = jit_expr(jit, node->bind.expr);

    const char *varname = node->bind.ident->s;
    struct nl_type* expr_type = node->bind.expr->type;

    LLVMTypeRef type;
    switch (expr_type->tag) {
    case NL_TYPE_BOOL:
        type = LLVMInt1Type();
        break;
    case NL_TYPE_INT:
        type = LLVMInt64Type();
        break;
    case NL_TYPE_REAL:
        type = LLVMDoubleType();
        break;
    default:
        JIT_ERROR(jit, node, "bind type not yet supported");
        return;     /* TODO: exit JIT */
    }

    LLVMValueRef alloca = LLVMBuildAlloca(jit->builder, type, varname);
    LLVMBuildStore(jit->builder, bind_value, alloca);

    /* save this variable binding */
    nl_symtable_add(jit->named_values, (nl_string_t)varname, alloca);
}

static void jit_assign(struct jit* jit, struct nl_ast* node)
{
    assert(node->tag == NL_AST_ASSIGN);

    struct nl_ast* lhs = node->assignment.lhs;
    assert(lhs->tag == NL_AST_IDENT); /* only variable assignments for now */

    LLVMValueRef rhs = jit_expr(jit, node->assignment.expr);

    if (node->assignment.op != TOK_ASS) {
        LLVMValueRef lhs_value = jit_ident(jit, lhs);
        switch (node->assignment.op) {
        case TOK_ASS:
            break;
        case TOK_IADD:
            rhs = LLVMBuildAdd(jit->builder, lhs_value, rhs, "tmp.add");
            break;
        case TOK_ISUB:
            rhs = LLVMBuildSub(jit->builder, lhs_value, rhs, "tmp.sub");
            break;
        case TOK_IMUL:
            rhs = LLVMBuildMul(jit->builder, lhs_value, rhs, "tmp.mul");
            break;
        case TOK_IDIV:
            rhs = LLVMBuildSDiv(jit->builder, lhs_value, rhs, "tmp.div");
            break;
        default:
            JIT_ERROR(jit, node, "unsupported assignment operator");
            return;     /* TODO: exit JIT */
        }
    }

    LLVMValueRef alloca = nl_symtable_search(jit->named_values, lhs->s);
    if (NULL == alloca) {
        JIT_ERRORF(jit, node, "no such variable: %s", lhs->s);
        return; /* TODO: exit JIT */
    }

    LLVMBuildStore(jit->builder, rhs, alloca);
}

static void jit_ifelse(struct jit* jit, struct nl_ast* node)
{
    assert(node->tag == NL_AST_IFELSE);

    struct nl_ast* if_body = node->ifelse.if_body;
    struct nl_ast* else_body = node->ifelse.else_body;

    LLVMValueRef cond = jit_expr(jit, node->ifelse.cond);

    LLVMBasicBlockRef insert_block = LLVMGetInsertBlock(jit->builder);
    LLVMValueRef function = LLVMGetBasicBlockParent(insert_block);

    LLVMBasicBlockRef then_block = LLVMAppendBasicBlock(function, "if.then");
    LLVMBasicBlockRef else_block;
    if (else_body != NULL) {
        else_block = LLVMAppendBasicBlock(function, "if.else");
    }
    LLVMBasicBlockRef end_block = LLVMAppendBasicBlock(function, "if.end");

    if (else_body != NULL) {
        LLVMBuildCondBr(jit->builder, cond, then_block, else_block);
    } else {
        LLVMBuildCondBr(jit->builder, cond, then_block, end_block);
    }

    LLVMPositionBuilderAtEnd(jit->builder, then_block);
    jit_node(jit, if_body);
    if (!LLVMGetBasicBlockTerminator(then_block)) {
        /* only emit branch if block doesn't already have a terminator (e.g. return) */
        LLVMBuildBr(jit->builder, end_block);
    }

    LLVMMoveBasicBlockAfter(then_block, LLVMGetLastBasicBlock(function));

    /* update then_block to current insert block, which may have changed */
    /* then_block = LLVMGetInsertBlock(jit->builder); */

    if (else_body != NULL) {
        LLVMPositionBuilderAtEnd(jit->builder, else_block);
        jit_node(jit, node->ifelse.else_body);
        if (!LLVMGetBasicBlockTerminator(else_block)) {
            /* only emit branch if block doesn't already have a terminator */
            LLVMBuildBr(jit->builder, end_block);
        }

        LLVMMoveBasicBlockAfter(else_block, LLVMGetLastBasicBlock(function));
        /* update else_block to current insert block, which may have changed */
        /* else_block = LLVMGetInsertBlock(jit->builder); */
    }

    LLVMMoveBasicBlockAfter(end_block, LLVMGetLastBasicBlock(function));
    LLVMPositionBuilderAtEnd(jit->builder, end_block);

    /* the following is for if-else *expressions* */
    /* LLVMValueRef phi = LLVMBuildPhi(jit->builder, LLVMInt1Type(), "iftmp"); */
    /* LLVMValueRef fake1 = LLVMConstInt(LLVMInt1Type(), 1, false); */
    /* LLVMValueRef fake2 = LLVMConstInt(LLVMInt1Type(), 1, false); */
    /* LLVMValueRef values[] = {fake1, fake2}; */
    /* LLVMBasicBlockRef blocks[] = {then_block, else_block}; */
    /* LLVMAddIncoming(phi, values, blocks, 2); */
}

static void jit_while(struct jit* jit, struct nl_ast* node)
{
    assert(node->tag == NL_AST_WHILE);

    LLVMBasicBlockRef insert_block = LLVMGetInsertBlock(jit->builder);
    LLVMValueRef function = LLVMGetBasicBlockParent(insert_block);

    LLVMBasicBlockRef loop_block = LLVMAppendBasicBlock(function, "while.loop");
    LLVMBasicBlockRef body_block = LLVMAppendBasicBlock(function, "while.body");
    LLVMBasicBlockRef end_block = LLVMAppendBasicBlock(function, "while.end");

    /* insert an explicit fallthrough from current block to loop block */
    LLVMBuildBr(jit->builder, loop_block);
    LLVMPositionBuilderAtEnd(jit->builder, loop_block);

    LLVMValueRef cond = jit_expr(jit, node->while_loop.cond);
    LLVMBuildCondBr(jit->builder, cond, body_block, end_block);

    LLVMPositionBuilderAtEnd(jit->builder, body_block);

    jit_node(jit, node->while_loop.body);
    if (!LLVMGetBasicBlockTerminator(loop_block)) {
        LLVMBuildBr(jit->builder, loop_block);
    }

    LLVMBuildBr(jit->builder, loop_block);

    LLVMMoveBasicBlockAfter(end_block, LLVMGetLastBasicBlock(function));
    LLVMPositionBuilderAtEnd(jit->builder, end_block);
}

static void jit_call_stmt(struct jit* jit, struct nl_ast* node)
{
    assert(node->tag == NL_AST_CALL_STMT);

    jit_call(jit, node);
}

static void jit_return(struct jit* jit, struct nl_ast* node)
{
    assert(node->tag == NL_AST_RETURN);
    JIT_DEBUG(jit, "JITing return statement");

    JIT_DEBUGF(jit, "ret expr: %s", nl_ast_name(node->ret.expr));
    LLVMValueRef ret = jit_expr(jit, node->ret.expr);
    LLVMBuildRet(jit->builder, ret);
}

static void jit_package(struct jit* jit, struct nl_ast* node)
{
    assert(node->tag == NL_AST_PACKAGE);
    JIT_DEBUGF(jit, "JITing package %s", node->package.name->s);

    /* jit_node(node->package.name); */
    jit_node(jit, node->package.globals);
}

static void jit_function(struct jit* jit, struct nl_ast* node)
{
    assert(node->tag == NL_AST_FUNCTION);

    const char* func_name = node->function.name->s;
    JIT_DEBUGF(jit, "JITing function %s", func_name);

    // TODO: param types
    struct nl_ast* function_type = node->function.type;
    assert(function_type->tag == NL_AST_FUNC_TYPE);

    struct nl_type* return_type = function_type->func_type.ret_type->type;
    LLVMTypeRef ret_type;
    switch (return_type->tag) {
    case NL_TYPE_BOOL:
        ret_type = LLVMInt1Type();
        break;
    case NL_TYPE_INT:
        ret_type = LLVMInt64Type();
        break;
    case NL_TYPE_REAL:
        ret_type = LLVMDoubleType();
        break;
    default:
        JIT_ERROR(jit, function_type, "return type not yet supported");
        return;
    }

    unsigned int param_count = function_type->func_type.params->list.count;
    LLVMTypeRef* param_types = nl_alloc(jit->ctx, sizeof(*param_types) * param_count);

    struct nl_ast* param = function_type->func_type.params->list.head;
    unsigned int idx = 0;
    while (param) {
        assert(param->tag == NL_AST_DECL);
        struct nl_type* ptp = param->decl.type->type;
        LLVMTypeRef param_type;
        switch (ptp->tag) {
            case NL_TYPE_BOOL:
                param_type = LLVMInt1Type();
                break;
            case NL_TYPE_INT:
                param_type = LLVMInt64Type();
                break;
            case NL_TYPE_REAL:
                param_type = LLVMDoubleType();
                break;
            default:
                JIT_ERROR(jit, function_type, "argument type not yet supported");
                /* printf("jit argument %s with type %s: %d\n", param->decl.rhs->s, */
                /*         param->decl.type->s, param->decl.type->type->tag); */
                return;
        }
        param_types[idx++] = param_type;
        param = param->next;
    }

    // TODO: variable argument functions
    LLVMTypeRef func_type = LLVMFunctionType(ret_type, param_types, param_count, false);

    LLVMValueRef func = LLVMAddFunction(jit->mod, func_name, func_type);

    LLVMBasicBlockRef entry = LLVMAppendBasicBlock(func, "entry");
    LLVMPositionBuilderAtEnd(jit->builder, entry);

    /* create argument allocas */
    /* struct nl_ast* */ param = function_type->func_type.params->list.head;
    /* unsigned int */ idx = 0;
    while (param) {
        assert(param->tag == NL_AST_DECL);
        struct nl_type* ptp = param->decl.type->type;
        LLVMTypeRef param_type;
        switch (ptp->tag) {
            case NL_TYPE_BOOL:
                param_type = LLVMInt1Type();
                break;
            case NL_TYPE_INT:
                param_type = LLVMInt64Type();
                break;
            case NL_TYPE_REAL:
                param_type = LLVMDoubleType();
                break;
            default:
                JIT_ERROR(jit, function_type, "argument type not yet supported");
                /* printf("jit argument %s with type %s: %d\n", param->decl.rhs->s, */
                /*         param->decl.type->s, param->decl.type->type->tag); */
                return;
        }
        param_types[idx] = param_type;
        // TODO: handle initialized arguments
        assert(param->decl.rhs->tag == NL_AST_IDENT);
        const char *param_name = param->decl.rhs->s;
        /* printf("naming parameter %s\n", param_name); */

        LLVMValueRef arg = LLVMGetParam(func, idx);
        LLVMSetValueName(arg, param_name);

        LLVMValueRef alloca = LLVMBuildAlloca(jit->builder, param_type, param_name);
        LLVMBuildStore(jit->builder, arg, alloca);

        /* add argument to symbol table */
        nl_symtable_add(jit->named_values, (nl_string_t)param_name, alloca);

        idx++;
        param = param->next;
    }

    jit_node(jit, node->function.body);
}

static void jit_list(struct jit* jit, struct nl_ast* node)
{
    struct nl_ast *elem = node->list.head;
    while (elem) {
        jit_node(jit, elem);
        elem = elem->next;
    }
}

/* TODO: eliminate this function */
static void jit_fake(struct jit* jit, struct nl_ast* node)
{
    JIT_ERRORF(jit, node, "JIT not yet supported for %s", nl_ast_name(node));
}


typedef void (*jiter) (struct jit*, struct nl_ast*);

static void jit_node(struct jit* jit, struct nl_ast* node)
{
    assert(node);

    static jiter jiters[] = {
        NULL,   /* sentinel */

        jit_fake /* jit_bool_lit */,
        jit_fake /* jit_char_lit */,
        jit_fake /* jit_int_lit */,
        jit_fake /* jit_real_lit */,
        jit_fake /* jit_str_lit */,
        jit_fake /* jit_list */,     /* list_lit */
        jit_fake /* jit_list */,     /* map_lit */
        jit_fake /* jit_class_lit */,
        jit_fake /* jit_ident */,
        jit_fake /* jit_unexpr */,
        jit_fake /* jit_binexpr */,
        jit_fake /* jit_call */,
        jit_fake /* jit_keyval */,
        jit_fake /* jit_lookup */,
        jit_fake /* jit_selector */,
        jit_fake /* jit_package_ref */,
        jit_function,

        jit_fake /* jit_tmpl_type */,
        jit_fake /* jit_qual_type */,
        jit_fake /* jit_func_type */,

        jit_decl,
        jit_fake /* jit_init */,
        jit_bind,
        jit_assign,
        jit_ifelse,
        jit_while,
        jit_fake /* jit_for */,
        jit_call_stmt,
        jit_return,
        jit_fake /* jit_break */,
        jit_fake /* jit_continue */,

        jit_fake /* jit_alias */,
        jit_fake /* jit_using */,

        jit_fake /* jit_class */,
        jit_fake /* jit_interface */,
        jit_package,
        NULL, /* units have been deconstructed into packages */

        NULL,   /* sentinel separator */

        jit_fake /* jit_list */,
        jit_fake /* jit_list */,
        jit_fake /* jit_list */,
        jit_fake /* jit_list */,
        jit_fake /* jit_list */,
        jit_fake /* jit_list */,
        jit_fake /* jit_list */,
        jit_fake /* jit_list */,
        jit_fake /* jit_list */,
        jit_list,
        jit_fake /* jit_list */,
        jit_list,
        jit_list,
        NULL, /* units have been deconstructed into packages */

        NULL /* sentinel */
    };

    assert(sizeof(jiters) / sizeof(*jiters) == NL_AST_LAST + 1);

    jiter j = jiters[node->tag];

    /* assert(j); */
    if (j == NULL) {
        JIT_ERROR(jit, node, "undefined JIT function");
        return;
    }
    j(jit, node);
}

int nl_jit(struct nl_context *ctx, struct nl_ast* packages, int* return_code)
{
    assert(ctx);
    assert(ctx->ast_list);


    LLVMInitializeNativeTarget();
    LLVMInitializeNativeAsmPrinter();
    LLVMInitializeNativeAsmParser();
    LLVMLinkInMCJIT();

    LLVMModuleRef mod = LLVMModuleCreateWithName("nolli");

    char *error = NULL;
    struct LLVMMCJITCompilerOptions options;
    LLVMInitializeMCJITCompilerOptions(&options, sizeof(options));
    LLVMExecutionEngineRef engine;
    if (LLVMCreateMCJITCompilerForModule(&engine, mod, &options, sizeof(options), &error) != 0) {
        NL_ERROR(ctx, NL_ERR_JIT, "failed to create execution engine\n");
        return NL_ERR_JIT;
    }
    if (error) {
        NL_ERRORF(ctx, NL_ERR_JIT, "error: %s\n", error);
        LLVMDisposeMessage(error);
        return NL_ERR_JIT;
    }

    LLVMBuilderRef builder = LLVMCreateBuilder();

    struct nl_symtable* named_values = nl_symtable_create(NULL);
    struct jit jit = {
        .ctx=ctx,
        .mod=mod,
        .builder=builder,
        .named_values=named_values,
    };

    jit_node(&jit, packages);

    /* error = NULL; */
    /* if (!LLVMVerifyModule(mod, LLVMReturnStatusAction, &error)) { */
    /*     NL_ERRORF(ctx, NL_ERR_JIT, "LLVM module failed verification: %s", error); */
    /*     LLVMDisposeMessage(error); */
    /*     return NL_ERR_JIT; */
    /* } */

    LLVMDumpModule(mod);

    uint64_t addr = LLVMGetFunctionAddress(engine, "main");
    int32_t (*fp)() = (int32_t (*)())addr;

    *return_code = fp();

    JIT_DEBUGF(&jit, "main evaluated to: %d", *return_code);

    return NL_NO_ERR;
}
