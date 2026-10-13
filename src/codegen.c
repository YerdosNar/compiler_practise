#include "include/ast.h"
#include "include/lexer.h"
#include "include/codegen.h"

#include <stdio.h>
#include <stdlib.h>

static void gen_expr(Expr *e);
static void gen_addr(Expr *e);
static void gen_assign(Expr *e);
static u32  break_label = 0;
static Function *current_fn;

static void comparison_code(const char *instruction)
{
        printf("  cmp    rax,  rdi\n");
        printf("  %s  al\n", instruction);
        printf("  movzx  rax,  al\n");
}

static u32 next_label(void)
{
        static u32 count = 1;
        return count++;
}

static void gen_and(Expr *e)
{
        u32 n = next_label();

        gen_expr(e->binary.left);
        printf("  cmp    rax,  0\n");
        printf("  je     .L.false.%u\n", n);
        gen_expr(e->binary.right);
        printf("  cmp    rax,  0\n");
        printf("  je     .L.false.%u\n", n);
        printf("  mov    rax,  1\n");
        printf("  jmp    .L.end.%u\n", n);
        printf(".L.false.%u:\n", n);
        printf("  mov    rax,  0\n");
        printf(".L.end.%u:\n", n);
}

static void gen_or(Expr *e)
{
        u32 n = next_label();

        gen_expr(e->binary.left);
        printf("  cmp    rax,  0\n");
        printf("  jne    .L.true.%u\n", n);
        gen_expr(e->binary.right);
        printf("  cmp    rax,  0\n");
        printf("  jne    .L.true.%u\n", n);
        printf("  mov    rax,  0\n");
        printf("  jmp    .L.end.%u\n", n);
        printf(".L.true.%u:\n", n);
        printf("  mov    rax,  1\n");
        printf(".L.end.%u:\n", n);

}

static void gen_expr(Expr *e)
{
        switch (e->kind) {
        case EXPR_CALL:
                printf("  call   %.*s\n", (int)e->call.length, e->call.name);
                return;

        case EXPR_NUMBER:
                printf("  mov    rax,  %lld\n", (long long)e->number);
                return;

        case EXPR_VAR:
                gen_addr(e);
                printf("  mov    rax,  [rax]\n");
                return;

        case EXPR_UNARY:
                gen_expr(e->unary.operand);
                switch (e->unary.op) {
                        case TOK_PLUS:  return;
                        case TOK_MINUS: printf("  neg    rax\n");          return;
                        case TOK_NOT:   
                                printf("  cmp    rax,  0\n");
                                printf("  sete   al\n");
                                printf("  movzx  rax,  al\n");
                                return;
                        default: break;
                }
                return;

        case EXPR_BINARY:
                if (e->binary.op == TOK_AND)    { gen_and(e);    return;}
                if (e->binary.op == TOK_OR)     { gen_or(e);     return;}
                if (e->binary.op == TOK_ASSIGN) { gen_assign(e); return;}

                gen_expr(e->binary.right);
                printf("  push   rax\n");
                gen_expr(e->binary.left);
                printf("  pop    rdi\n");

                switch (e->binary.op) {
                        case TOK_PLUS:  printf("  add    rax,  rdi\n");     return;
                        case TOK_MINUS: printf("  sub    rax,  rdi\n");     return;
                        case TOK_STAR:  printf("  imul   rax,  rdi\n");    return;
                        case TOK_EQ:    comparison_code("sete ");   return;
                        case TOK_NE:    comparison_code("setne");  return;
                        case TOK_LT:    comparison_code("setl ");   return;
                        case TOK_LE:    comparison_code("setle");  return;
                        case TOK_GT:    comparison_code("setg ");   return;
                        case TOK_GE:    comparison_code("setge");  return;
                        case TOK_SLASH:
                                printf("  cqo\n");
                                printf("  idiv   rdi\n");
                                return;
                        case TOK_MODULO:
                                printf("  cqo\n");
                                printf("  idiv   rdi\n");
                                printf("  mov    rax,  rdx\n");
                                return;
                        default:
                                fprintf(stderr, 
                                        "ERROR: codegen: unsupported operator: %s\n",
                                        tok_kind_name(e->binary.op));
                                exit(EXIT_FAILURE);
                }
        }
}

static void gen_stmt(Stmt *s)
{
        switch (s->kind) {
                case STMT_EXPR:
                        gen_expr(s->expr);
                        return;

                case STMT_RETURN:
                        gen_expr(s->expr);
                        printf("  jmp    .L.return.%.*s\n", 
                               (int)current_fn->length,
                               current_fn->name);
                        return;
                        
                case STMT_IF: {
                        u32 n = next_label();
                        gen_expr(s->if_stmt.cond);
                        printf("  cmp    rax,  0\n");
                        printf("  je     .L.else.%u\n", n);
                        gen_stmt(s->if_stmt.then);
                        printf("  jmp    .L.end.%u\n", n);
                        printf(".L.else.%u:\n", n);
                        if (s->if_stmt.els) gen_stmt(s->if_stmt.els);
                        printf(".L.end.%u:\n", n);
                        return;
                }

                case STMT_WHILE: {
                        u32 n = next_label();
                        u32 saved = break_label;
                        break_label = n;
                        printf(".L.begin.%u:\n", n);
                        gen_expr(s->while_stmt.cond);
                        printf("  cmp    rax,  0\n");
                        printf("  je     .L.end.%u\n", n);
                        gen_stmt(s->while_stmt.body);
                        break_label = saved;
                        printf("  jmp    .L.begin.%u\n", n);
                        printf(".L.end.%u:\n", n);
                        return;
                }

                case STMT_FOR: {
                        u32 n = next_label();
                        u32 saved = break_label;
                        break_label = n;
                        if (s->for_stmt.init) 
                                gen_expr(s->for_stmt.init);
                        printf(".L.begin.%u:\n", n);
                        if (s->for_stmt.cond) {
                                gen_expr(s->for_stmt.cond);
                                printf("  cmp    rax,  0\n");
                                printf("  je     .L.end.%u\n", n);
                        }
                        gen_stmt(s->for_stmt.body);
                        break_label = saved;
                        if (s->for_stmt.inc)
                                gen_expr(s->for_stmt.inc);
                        printf("  jmp    .L.begin.%u\n", n);
                        printf(".L.end.%u:\n", n);
                        return;
                }

                case STMT_BREAK:
                        if (!break_label) {
                                fprintf(stderr, "ERROR: 'break' outside of a loop\n");
                                exit(EXIT_FAILURE);
                        }
                        printf("  jmp    .L.end.%u\n", break_label);
                        return;

                case STMT_BLOCK:
                        for (Stmt *cur = s->body; cur; cur = cur->next)
                                gen_stmt(cur);
                        return;
        }
}

static void gen_addr(Expr *e)
{
        if (e->kind != EXPR_VAR) {
                fprintf(stderr, "ERROR: left side of '=' is not a variable\n");
                exit(EXIT_FAILURE);
        }
        printf("  lea    rax,  [rbp-%u]\n", e->var->offset);
}

static void gen_assign(Expr *e)
{
        gen_addr(e->binary.left);
        printf("  push   rax\n");
        gen_expr(e->binary.right);
        printf("  pop    rdi\n");
        printf("  mov    [rdi],  rax\n");
}

void codegen(Function *prog)
{
        printf("  .intel_syntax noprefix\n");

        for (Function *fn = prog; fn; fn = fn->next) {
                current_fn = fn;
                int len = (int)fn->length;

                printf("  .globl %.*s\n", len, fn->name);
                printf("%.*s:\n", len, fn->name);
                printf("  push   rbp\n");
                printf("  mov    rbp,  rsp\n");
                printf("  sub    rsp,  %u\n", 
                       (fn->stack_size + 15) / 16 * 16);
                gen_stmt(fn->body);
                printf("  mov    rax,  0\n");
                printf(".L.return.%.*s:\n", len, fn->name);
                printf("  mov    rsp,  rbp\n");
                printf("  pop    rbp\n");
                printf("  ret\n");
        }
        printf("  .section .note.GNU-stack,\"\",@progbits\n");
}
