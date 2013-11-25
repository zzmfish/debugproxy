#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"

/* 节点类型 */
typedef enum { NAME, KEYWORD, BLOCK, OPERATOR, STAMENT, OTHER } NodeType;

/* 节点结构 */
struct AstNode_t {
    int source_pos, source_len;
    NodeType type;
    struct AstNode_t *parent, *first_child, *last_child, *next, *prev;
};
typedef struct AstNode_t AstNode;

/* 语法树 */
typedef struct {
    AstNode *root;
} AST;

typedef struct {
    AstNode *cur_node;
    char *source;
    int source_pos;
    int source_len;
    int block_start;
} ParseState;

char next_char(ParseState *state)
{
    if (state->source_pos < state->source_len)
        return state->source[state->source_pos ++];
    return '\0';
}

char next_nonspace_char(ParseState *state)
{
    char c = next_char(state);
    while (c == ' ' || c == '\t' || c == '\r' || c == '\n')
        c = next_char(state);
    return c;
}

AstNode* create_node(AstNode *parent, NodeType type, int pos, int len)
{
    AstNode *node = (AstNode*) malloc(sizeof(AstNode));
    node->parent = parent;
    node->type = type;
    node->source_pos = pos;
    node->source_len = len;
    node->first_child = NULL;
    node->last_child = NULL;
    node->next = NULL;
    node->prev = NULL;
    //printf("create_node(parent=%p, type=%d, pos=%d, len=%d) => %p\n",
        //parent, type, pos, len, node);
    return node;
}

void append_child(AstNode *parent, AstNode *child)
{
    //printf("append_child(parent=%p, child=%p)\n", parent, child);
    if (!parent->first_child) {
        parent->first_child = child;
        parent->last_child = child;
    }
    else {
        parent->last_child->next = child;
        child->prev = parent->last_child;
        parent->last_child = child;
    }
    child->parent = parent;
}

AstNode* __parse_statement(ParseState *state, int testMode);
AstNode* parse_statement(ParseState *state) { return __parse_statement(state, 0); }
AstNode* test_statement(ParseState *state) { return __parse_statement(state, 1); }

AstNode* parse_operator(ParseState *state)
{
    AstNode *node = create_node(state->cur_node, OPERATOR, state->source_pos, 1);
    next_char(state);
    return node;
}

AstNode* parse_other(ParseState *state)
{
    //printf("parse_other BEGIN pos=%d\n", state->source_pos);
    AstNode *node = create_node(state->cur_node, OTHER, state->source_pos, 0);
    while (test_statement(state) == NULL) {
        if (state->source[state->source_pos] == '\0')
            break;
        state->source_pos ++;
        node->source_len ++;
    }
    //printf("parse_other END\n");
    return node;
}

int is_keyword(const char *str, int len)
{
    const char* keywords[] = {"function"};
    int i;
    for (i = 0; i < sizeof(keywords) / sizeof(char*); i ++) {
        if (strncmp(str, keywords[i], len) == 0)
            return 1;
    }
    return 0;
}

int is_function_keyword(AstNode *node, char *source)
{
    return node->type == KEYWORD
        && node->source_len == 8
        && strncmp(source + node->source_pos, "function", 8) == 0;
}

AstNode* parse_name(ParseState *state)
{
    char c;
    AstNode *node = create_node(state->cur_node, NAME, state->source_pos, 0);
    while (c = next_char(state)) {
        if ((c >= 'a' && c <= 'z')
            || (c >= 'A' && c <= 'Z'
            || (c >= '0' && c <= '9')
            || c == '_'))
            node->source_len ++;
        else {
            state->source_pos --;
            break;
        }
    }
    if (is_keyword(state->source + node->source_pos, node->source_len))
        node->type = KEYWORD;
    return node;
}

AstNode* parse_block(ParseState *state)
{
    char start_char = state->source[state->source_pos - 1];
    char end_char = '\0';
    if (start_char == '{')
        end_char = '}';
    else if (start_char == '(')
        end_char = ')';
    else if (start_char = '[')
        end_char = ']';
    AstNode *node = create_node(state->cur_node, BLOCK, state->source_pos - 1, 0);
    AstNode *cur_node = state->cur_node;
    state->cur_node = node;
    char c = next_nonspace_char(state);
    while (c != '\0' && c != end_char) {
        state->source_pos --;
        AstNode *child = parse_statement(state);
        if (child)
            append_child(node, child);
        c = next_nonspace_char(state);
    }
    node->source_len = state->source_pos - node->source_pos;
    state->cur_node = cur_node;
    return node;
}

AstNode* __parse_statement(ParseState *state, int testMode)
{
    //printf("__parse_statement(pos=%d, testMode=%d)\n", state->source_pos, testMode);
    AstNode *node = NULL;
    char c;
    int source_pos = state->source_pos;

    c = next_nonspace_char(state);
    if (c == '\0') {
    }
    else if (c == '{' || c == '(' || c == '[') {
        node = testMode ? (AstNode*)1 : parse_block(state);
    }
    else if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_') {
        state->source_pos --;
        node = testMode ? (AstNode*)1 : parse_name(state);
    }
    else if (c == ':' || c == '=') {
        state->source_pos --;
        node = testMode ? (AstNode*)1 : parse_operator(state);
    }
    else if (c == '}' || c == ')' || c == ']') {
        if (testMode) {
            node = (AstNode*) 1;
        }
    }
    else if (!testMode) {
        state->source_pos --;
        node = parse_other(state);
    }
    if (testMode)
        state->source_pos = source_pos;
    return node;
}

typedef struct {
    char *buffer;
    int buffer_cap;
    int buffer_len;
    int dumped_pos;
} InsertState;

int is_function_block(AstNode *node, char *source, AstNode **name)
{
    if (node->type == BLOCK && source[node->source_pos] == '{') {
        AstNode *prev1 = node->prev;
        if (!prev1) return 0;
        if (prev1->type == BLOCK && source[prev1->source_pos] == '(') {
            AstNode *prev2 = prev1->prev;
            if (!prev2) return 0;
            if (prev2->type == NAME) {
                AstNode *prev3 = prev2->prev;
                if (!prev3) return 0;
                if (is_function_keyword(prev3, source)) {
                    if (name)
                        *name = prev2;
                    return 1;
                }
            }
            else if (is_function_keyword(prev2, source)) {
                AstNode *prev3 = prev2->prev;
                if (!prev3) return 0;
                if (prev3->type == OPERATOR && (source[prev3->source_pos] == ':' || source[prev3->source_pos] == '=')) {
                    AstNode *prev4 = prev3->prev;
                    if (!prev4) return 0;
                    if (prev4->type == NAME) {
                        if (name)
                            *name = prev4;
                        return 1;
                    }
                }
            }
        }
    }
    return 0;
}

void append_source(InsertState *state, const char *source, int source_len)
{
    if (source_len == 0)
        return;
    if (state->buffer_len + source_len > state->buffer_cap) {
        if (source_len > state->buffer_cap)
            state->buffer_cap += source_len;
        else
            state->buffer_cap *= 2;
        state->buffer = (char*) realloc(state->buffer, state->buffer_cap);
    }
    memcpy(state->buffer + state->buffer_len, source, source_len);
    state->buffer_len += source_len;
}

void __insert_profile_codes(AstNode *node, char *source, InsertState *state)
{
    if (!node) return;
    AstNode *name_node = NULL;
    char name[64];
    char code[128];
    int profiling = 0;
    if (is_function_block(node, source, &name_node)) {
        //插入源码
        append_source(state, source + state->dumped_pos, node->source_pos + 1 - state->dumped_pos);
        state->dumped_pos = node->source_pos + 1;
        //函数名称
        int name_len = name_node->source_len;
        if (name_len > sizeof(name) - 1)
            name_len = sizeof(name) - 1;
        memcpy(name, source + name_node->source_pos, name_len);
        name[name_len] = '\0';
        //进入函数日志
        snprintf(code, sizeof(code), "console.log(\"%s\");if (window.dump) window.dump(\"jsconsole# %s\\n\");\n", name, name);
        append_source(state, code, strlen(code));
        profiling = 1;
    }
    AstNode *child = node->first_child;
    while (child) {
        __insert_profile_codes(child, source, state);
        child = child->next;
    }
    if (profiling) {
        //函数体
        append_source(state, source + state->dumped_pos, node->source_pos + node->source_len - 1 - state->dumped_pos);
        state->dumped_pos = node->source_pos + node->source_len - 1;
        //退出函数日志
        snprintf(code, sizeof(code), "console.log(\"<<%s\");if (window.dump) window.dump(\"jsconsole# <<%s\\n\");\n", name, name);
        append_source(state, code, strlen(code));
    }

}

void insert_profile_codes(AST *ast, char *source, int source_len,
        char **new_source, int *new_source_len)
{
    InsertState state;
    state.buffer_cap = 1024;
    state.buffer = (char*) malloc(state.buffer_cap);
    state.buffer_len = 0;
    state.dumped_pos = 0;
    __insert_profile_codes(ast->root, source, &state);
    append_source(&state, source + state.dumped_pos, source_len - state.dumped_pos);
    *new_source = state.buffer;
    *new_source_len = state.buffer_len;
}

AstNode* parse(char *source, int source_len)
{
    AstNode *root, *node;
    ParseState state;

    root = create_node(NULL, OTHER, 0, source_len);

    state.cur_node = root;
    state.source = source;
    state.source_len = source_len;
    state.source_pos = 0;
    state.block_start = -1;

    while (node = parse_statement(&state)) {
        append_child(root, node);
    }
    return root;
}

char *read_file(char *path, int *psize)
{  
    char *buffer = NULL;
    FILE *file = fopen(path, "r");
    if (file) {
        fseek(file, 0, SEEK_END);
        int size = ftell(file);
        if (size > 0) {
            fseek(file, 0, SEEK_SET);
            buffer = malloc(size);
            *psize = fread(buffer, 1, size, file);
        }
        fclose(file);
    }
    return buffer;
}

int write_file(char *path, char *buffer, int size)
{
    int result = 0;
    FILE *file = fopen(path, "w");
    if (file) {
        result = fwrite(buffer, 1, size, file);
        fclose(file);
    }
    return result;
}

void print_source(const char *source, int source_len, Bool single_line)
{
    int i;
    for (i = 0; i < source_len; i ++) {
        char c = source[i];
        if (!single_line || (c != '\r' && c != '\n'))
            printf("%c", source[i]);
    }
}

/***********
 * 显示AST
 ***********/
void print_tree(AstNode *node, char *source, int indent)
{
    int pos, len;
    AstNode *last_child;
    char desc[15];
    int desc_len;

    static const char indent_space[] = "                                                                      ";
    if (!node)
        return;


    
    /* 缩进 */
    if (indent > sizeof(indent_space) - 1)
        indent = sizeof(indent_space) - 1;
    const char *indent_str = indent_space + sizeof(indent_space) - indent - 1;

    //节点信息
    desc_len = snprintf(desc, sizeof(desc), "%d @%d,%d", node->type, node->source_pos, node->source_len);
    printf("%s%s", indent_str, desc);
    
    /* 显示前面代码 */
    pos = node->source_pos;
    len = node->source_len;
    if (node->first_child) {
        len = node->first_child->source_pos - pos;
    }
    if (len > 0) {
        print_source(indent_space, sizeof(desc) - desc_len, True);
        print_source(source + pos, len, True);
    }
    if (is_function_block(node, source, NULL))
        printf(" *");
    printf("\n");

    /* 显示孩子节点 */
    AstNode *child = node->first_child;
    while (child) {
        print_tree(child, source, indent + 2);
        child = child->next;
    }

    /* 显示后面代码 */
    last_child = node->last_child;
    if (last_child) {
        pos = last_child->source_pos + last_child->source_len;
        len = node->source_pos + node->source_len - pos;
        if (len > 0) {
            printf("%s", indent_str);
            print_source(indent_space, sizeof(desc), True);
            print_source(source + pos, len, True);
            printf("\n");
        }
    }
}

/**********
 * 主程序
 **********/
int main(int argc, char **argv)
{
    if (argc != 3) {
        printf("Usage: js <infile> <outfile>\n");
        return 0;
    }
    char *infile = argv[1];
    char *outfile = argv[2];
    int source_len = 0;
    char *source = read_file(infile, &source_len);
    #if 0
    char source[] = "{var i}{var j = 0; {} function a() { alert(1); alert([1, 2]) }}";
    int source_len = sizeof(source);
    printf("%s\n", source);
    printf("len=%d\n", source_len);
    #endif

    AST ast;
    ast.root = parse(source, source_len);
    printf("Syntax Tree:\n");
    //print_tree(ast.root, source, 0);

    char *new_source = NULL;
    int new_source_len = 0;
    insert_profile_codes(&ast, source, source_len, &new_source, &new_source_len);
    printf("new_len=%d\n", new_source_len);
    if (new_source_len)
        write_file(outfile, new_source, new_source_len);

    return 0;
}
