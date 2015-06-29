#include "nolli.h"
#include "ast.h"
#include "type.h"
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

typedef LLVMValueRef (*bin_op_instr) (LLVMBuilderRef, LLVMValueRef, LLVMValueRef, const char*);

static bin_op_instr jit_bin_int_expr(struct jit* jit, struct nl_ast* node)
{
    bin_op_instr instr;
    switch (node->binexpr.op) {
        case TOK_ADD:
            instr = LLVMBuildAdd;
            break;
        case TOK_SUB:
            instr = LLVMBuildSub;
            break;
        case TOK_MUL:
            instr = LLVMBuildMul;
            break;
        case TOK_DIV:
            instr = LLVMBuildSDiv;
            break;
        default:
            JIT_ERROR(jit, node, "unsupported real binary operation");
            instr = NULL;
    }
    return instr;
}

static bin_op_instr jit_bin_real_expr(struct jit* jit, struct nl_ast* node)
{
    bin_op_instr instr;
    switch (node->binexpr.op) {
        case TOK_ADD:
            instr = LLVMBuildFAdd;
            break;
        case TOK_SUB:
            instr = LLVMBuildFSub;
            break;
        case TOK_MUL:
            instr = LLVMBuildFMul;
            break;
        case TOK_DIV:
            instr = LLVMBuildFDiv;
            break;
        default:
            JIT_ERROR(jit, node, "unsupported real binary operation");
            instr = NULL;
    }
    return instr;
}

static LLVMValueRef jit_bin_expr(struct jit* jit, struct nl_ast* node)
{
    assert(node->tag == NL_AST_BINEXPR);

    LLVMValueRef lhs = jit_expr(jit, node->binexpr.lhs);
    LLVMValueRef rhs = jit_expr(jit, node->binexpr.rhs);

    struct nl_type* lhs_type = node->binexpr.lhs->type;

    bin_op_instr instr;
    switch (lhs_type->tag) {
        case NL_TYPE_INT:
            instr = jit_bin_int_expr(jit, node);
            break;
        case NL_TYPE_REAL:
            instr = jit_bin_real_expr(jit, node);
            break;
        default:
            JIT_ERROR(jit, node, "unsupported binary operand type");
            return NULL;
    }

    return instr(jit->builder, lhs, rhs, "tmp");
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
        expr = LLVMConstInt(LLVMInt1Type(), 1, false);
        break;
    case NL_AST_INT_LIT:
        expr = LLVMConstInt(LLVMInt64Type(), node->l, true);
        break;
    case NL_AST_REAL_LIT:
        expr = LLVMConstReal(LLVMDoubleType(), node->d);
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

static void jit_ifelse(struct jit* jit, struct nl_ast* node)
{
    assert(node->tag == NL_AST_IFELSE);

    LLVMValueRef cond = jit_expr(jit, node->ifelse.cond);

    LLVMBasicBlockRef insert_block = LLVMGetInsertBlock(jit->builder);
    LLVMValueRef function = LLVMGetBasicBlockParent(insert_block);

    LLVMBasicBlockRef then_block = LLVMAppendBasicBlock(function, "then");
    LLVMBasicBlockRef else_block = LLVMAppendBasicBlock(function, "else");
    LLVMBasicBlockRef end_block = LLVMAppendBasicBlock(function, "ifcont");

    LLVMBuildCondBr(jit->builder, cond, then_block, else_block);

    LLVMPositionBuilderAtEnd(jit->builder, then_block);
    jit_node(jit, node->ifelse.if_body);
    LLVMBuildBr(jit->builder, end_block);

    /* update then_block to current insert block, which may have changed */
    then_block = LLVMGetInsertBlock(jit->builder);

    LLVMPositionBuilderAtEnd(jit->builder, else_block);
    jit_node(jit, node->ifelse.else_body);
    LLVMBuildBr(jit->builder, end_block);

    /* update else_block to current insert block, which may have changed */
    else_block = LLVMGetInsertBlock(jit->builder);

    LLVMPositionBuilderAtEnd(jit->builder, end_block);

    LLVMValueRef phi = LLVMBuildPhi(jit->builder, LLVMInt1Type(), "iftmp");

    LLVMValueRef fake1 = LLVMConstInt(LLVMInt1Type(), 1, false);
    LLVMValueRef fake2 = LLVMConstInt(LLVMInt1Type(), 1, false);
    LLVMValueRef values[] = {fake1, fake2};
    LLVMBasicBlockRef blocks[] = {then_block, else_block};
    LLVMAddIncoming(phi, values, blocks, 2);
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

    LLVMTypeRef func_type = LLVMFunctionType(ret_type, NULL, 0, 0);

    LLVMValueRef func = LLVMAddFunction(jit->mod, func_name, func_type);

    LLVMBasicBlockRef entry = LLVMAppendBasicBlock(func, "entry");
    LLVMPositionBuilderAtEnd(jit->builder, entry);

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

        jit_fake /* jit_decl */,
        jit_fake /* jit_init */,
        jit_fake /* jit_bind */,
        jit_fake /* jit_assign */,
        jit_ifelse,
        jit_fake /* jit_while */,
        jit_fake /* jit_for */,
        jit_call_stmt ,
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

    struct jit jit = {.ctx=ctx, .mod=mod, .builder=builder};

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
