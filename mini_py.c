#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>

// 简易变量环境：最多26个变量（a-z）
int vars[26] = {0};

// 错误处理
jmp_buf error_buf;

// ---------- 词法分析 ----------
typedef enum {
    TOK_NUM, TOK_ID, TOK_PRINT, TOK_ASSIGN,
    TOK_PLUS, TOK_MINUS, TOK_MUL, TOK_DIV,
    TOK_LPAREN, TOK_RPAREN, TOK_EOF, TOK_ERROR
} TokenType;

typedef struct {
    TokenType type;
    int num_val;      // 如果是数字
    char id_name[32]; // 如果是标识符
} Token;

char *src;            // 源码指针
Token cur_token;      // 当前token

// 获取下一个token
void next_token() {
    // 跳过空白
    while (*src == ' ' || *src == '\t' || *src == '\n') src++;

    if (*src == '\0') {
        cur_token.type = TOK_EOF;
        return;
    }

    // 数字
    if (isdigit(*src)) {
        cur_token.type = TOK_NUM;
        cur_token.num_val = 0;
        while (isdigit(*src)) {
            cur_token.num_val = cur_token.num_val * 10 + (*src - '0');
            src++;
        }
        return;
    }

    // 标识符或关键字
    if (isalpha(*src)) {
        char *start = src;
        while (isalnum(*src)) src++;
        int len = src - start;
        if (len == 5 && strncmp(start, "print", 5) == 0) {
            cur_token.type = TOK_PRINT;
        } else {
            cur_token.type = TOK_ID;
            strncpy(cur_token.id_name, start, len);
            cur_token.id_name[len] = '\0';
        }
        return;
    }

    // 单个字符运算符
    cur_token.type = TOK_ERROR;
    switch (*src) {
        case '=': cur_token.type = TOK_ASSIGN; break;
        case '+': cur_token.type = TOK_PLUS; break;
        case '-': cur_token.type = TOK_MINUS; break;
        case '*': cur_token.type = TOK_MUL; break;
        case '/': cur_token.type = TOK_DIV; break;
        case '(': cur_token.type = TOK_LPAREN; break;
        case ')': cur_token.type = TOK_RPAREN; break;
        default: longjmp(error_buf, 1); // 未知字符
    }
    src++;
}

// ---------- 语法分析 & 求值 ----------
// 前向声明
int parse_expr();

// 解析基本单元：数字、变量、括号表达式
int parse_primary() {
    if (cur_token.type == TOK_NUM) {
        int val = cur_token.num_val;
        next_token();
        return val;
    }
    if (cur_token.type == TOK_ID) {
        int idx = cur_token.id_name[0] - 'a'; // 只用单个字母变量
        if (idx < 0 || idx > 25) longjmp(error_buf, 1);
        int val = vars[idx];
        next_token();
        return val;
    }
    if (cur_token.type == TOK_LPAREN) {
        next_token();
        int val = parse_expr();
        if (cur_token.type != TOK_RPAREN) longjmp(error_buf, 1);
        next_token();
        return val;
    }
    longjmp(error_buf, 1);
}

// 解析乘除（优先级高）
int parse_term() {
    int left = parse_primary();
    while (cur_token.type == TOK_MUL || cur_token.type == TOK_DIV) {
        TokenType op = cur_token.type;
        next_token();
        int right = parse_primary();
        if (op == TOK_MUL) left = left * right;
        else left = left / right; // 整数除法
    }
    return left;
}

// 解析加减（优先级低）
int parse_expr() {
    int left = parse_term();
    while (cur_token.type == TOK_PLUS || cur_token.type == TOK_MINUS) {
        TokenType op = cur_token.type;
        next_token();
        int right = parse_term();
        if (op == TOK_PLUS) left = left + right;
        else left = left - right;
    }
    return left;
}

// 解析语句：print <expr> 或 <id> = <expr>
void parse_statement() {
    if (cur_token.type == TOK_PRINT) {
        next_token();
        int val = parse_expr();
        printf("%d\n", val);
    } else if (cur_token.type == TOK_ID) {
        char var_name = cur_token.id_name[0];
        if (var_name < 'a' || var_name > 'z') longjmp(error_buf, 1);
        next_token();
        if (cur_token.type != TOK_ASSIGN) longjmp(error_buf, 1);
        next_token();
        int val = parse_expr();
        vars[var_name - 'a'] = val;
    } else {
        longjmp(error_buf, 1);
    }
}

// ---------- REPL ----------
int main() {
    char line[256];
    printf("PyLight MVP (输入 exit 退出)\n");
    while (1) {
        printf(">>> ");
        if (!fgets(line, sizeof(line), stdin)) break;
        if (strcmp(line, "exit\n") == 0) break;

        src = line;
        next_token(); // 初始化第一个token

        if (setjmp(error_buf)) {
            printf("语法错误！\n");
            continue;
        }

        parse_statement();

        // 确保语句结束后是EOF或换行
        if (cur_token.type != TOK_EOF && cur_token.type != TOK_ERROR) {
            printf("多余字符！\n");
        }
    }
    return 0;
}