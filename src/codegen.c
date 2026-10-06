#include "ast.h"
#include "lexer.h"
#include "codegen.h"

#include <stdio.h>
#include <stdlib.h>

static void gen_expr(Expr *e);
static void gen_addr(Expr *e);
static void gen_assign(Expr *e);

static void comparison_code(const char *instruction)
{
        printf("  cmp\trax,\trdi\n");
        printf("  %s\tal\n", instruction);
        printf("  movzx\trax,\tal\n");
}

static int next_label(void)
{
        static int count = 1;
        return count++;
}

static void gen_and(Expr *e)
{
        int n = next_label();

        gen_expr(e->binary.left);
        printf("  cmp\trax,\t0\n");
        printf("  je\t.L.false.%d\n", n);
        gen_expr(e->binary.right);
        printf("  cmp\trax,\t0\n");
        printf("  je\t.L.false.%d\n", n);
        printf("  mov\trax,\t1\n");
        printf("  jmp\t.L.end.%d\n", n);
        printf(".L.false.%d:\n", n);
        printf("  mov\trax,\t0\n");
        printf(".L.end.%d:\n", n);
}

static void gen_or(Expr *e)
{
        int n = next_label();

        gen_expr(e->binary.left);
        printf("  cmp\trax,\t0\n");
        printf("  jne\t.L.true.%d\n", n);
        gen_expr(e->binary.right);
        printf("  cmp\trax,\t0\n");
        printf("  jne\t.L.true.%d\n", n);
        printf("  mov\trax,\t0\n");
        printf("  jmp\t.L.end.%d\n", n);
        printf(".L.true.%d:\n", n);
        printf("  mov\trax,\t1\n");
        printf(".L.end.%d:\n", n);

}

static void gen_expr(Expr *e)
{
        switch (e->kind) {
        case EXPR_NUMBER:
                printf("  mov\trax,\t%lld\n", (long long)e->number);
                return;

        case EXPR_VAR:
                gen_addr(e);
                printf("  mov\trax,\r[rax]\n");
                return;

        case EXPR_UNARY:
                gen_expr(e->unary.operand);
                switch (e->unary.op) {
                        case TOK_PLUS:  return;
                        case TOK_MINUS: printf("  neg\trax\n");          return;
                        case TOK_NOT:   
                                printf("  cmp\trax,\t0\n");
                                printf("  sete al\n");
                                printf("  movzx\trax,\tal\n");
                                return;
                        default: break;
                }
                return;

        case EXPR_BINARY:
                if (e->binary.op == TOK_AND)    { gen_and(e);    return;}
                if (e->binary.op == TOK_OR)     { gen_or(e);     return;}
                if (e->binary.op == TOK_ASSIGN) { gen_assign(e); return;}

                gen_expr(e->binary.right);
                printf("  push\trax\n");
                gen_expr(e->binary.left);
                printf("  pop\trdi\n");

                switch (e->binary.op) {
                        case TOK_PLUS:  printf("  add\trax,\trdi\n");     return;
                        case TOK_MINUS: printf("  sub\trax,\trdi\n");     return;
                        case TOK_STAR:  printf("  imul\trax,\trdi\n");    return;
                        case TOK_EQ:    comparison_code("sete");   return;
                        case TOK_NE:    comparison_code("setne");  return;
                        case TOK_LT:    comparison_code("setl");   return;
                        case TOK_LE:    comparison_code("setle");  return;
                        case TOK_GT:    comparison_code("setg");   return;
                        case TOK_GE:    comparison_code("setge");  return;
                        case TOK_SLASH:
                                printf("  cqo\n");
                                printf("  idiv\trdi\n");
                                return;
                        case TOK_MODULO:
                                printf("  cqo\n");
                                printf("  idiv\trdi\n");
                                printf("  mov\trax,\trdx\n");
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
                        printf("  jmp .L.return\n");
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
        printf("  lea\trax,\t[rbp-%d]\n", e->var->offset);
}

static void gen_assign(Expr *e)
{
        gen_addr(e->binary.left);
        printf("  push\trax\n");
        gen_expr(e->binary.right);
        printf("  pop\trdi\n");
        printf("  mov\t[rdi],\trax\n");
}

void codegen(Stmt *prog, i32 stack_size)
{
        printf("  .intel_syntax noprefix\n");
        printf("  .globl main\n");
        printf("main:\n");
        printf("  push\trbp\n");
        printf("  mov\trbp,\trsp\n");
        printf("  sub\trsp,\t%d\n", (stack_size + 15) / 16 * 16);
        gen_stmt(prog);
        printf("  mov\trax,\t0\n");
        printf(".L.return:\n");
        printf("  mov\trsp,\trbp\n");
        printf("  pop\trbp\n");
        printf("  ret\n");
        printf("  .section .note.GNU-stack,\"\",@progbits\n");
}
