#include <stdio.h>
#include <stdlib.h>

/* 节点类型 */
typedef enum { NAME, KEYWORD, BLOCK, OPERATOR, STAMENT, OTHER } NodeType;

/* 节点结构 */
struct AstNode_t {
    int source_pos, source_len;
    NodeType type;
    struct AstNode_t *parent, *first_child, *last_child, *next;
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
        parent->last_child = child;
    }
    child->parent = parent;
}

AstNode* parse_statement(ParseState *state);

AstNode* parse_other(ParseState *state)
{
    AstNode *node = create_node(state->cur_node, OTHER, state->source_pos, 0);
    char c;
    while (c= next_char(state)) {
        if (c == '{' || c == '}') {
            state->source_pos --;
            break;
        }
        else
            node->source_len ++;

    }
    return node;
}

AstNode* parse_block(ParseState *state, char end_char)
{
    AstNode *node = create_node(state->cur_node, BLOCK, state->source_pos, 0);
    AstNode *cur_node = state->cur_node;
    state->cur_node = node;
    char c = next_char(state);
    while (c != '\0' && c != end_char) {
        state->source_pos --;
        AstNode *child = parse_statement(state);
        if (child)
            append_child(node, child);
        c = next_char(state);
    }
    node->source_len = state->source_pos - node->source_pos - 1;
    state->cur_node = cur_node;
    return node;
}

AstNode* parse_statement(ParseState *state)
{
    AstNode *node = NULL;
    char c;
    while (c= next_char(state)) {
        switch (c) {
        case '{':
            node = parse_block(state, '}');
            break;
        default:
            state->source_pos --;
            node = parse_other(state);
            break;
        }
        if (node)
            break;
    }
    return node;
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

/***********
 * 打印AST
 ***********/
void print_tree(AstNode *node, int indent)
{
    static const char indent_space[] = "                                                                      ";
    if (!node)
        return;
    if (indent > sizeof(indent_space) - 1)
        indent = sizeof(indent_space) - 1;
    const char *indent_str = indent_space + sizeof(indent_space) - indent - 1;
    printf("%s<%d> @%d,%d\n", indent_str, node->type, node->source_pos, node->source_len);
    AstNode *child = node->first_child;
    while (child) {
        print_tree(child, indent + 2);
        child = child->next;
    }
}

/**********
 * 主程序
 **********/
int main(int argc, char **argv)
{
    /*
    if (argc != 2) {
        printf("Usage: js <file>\n");
        return 0;
    }
    char *file = argv[1];
    int source_len = 0;
    char *source = read_file(file, &source_len);
    */
    char source[] = "{var i}{var j; {}}";
    int source_len = sizeof(source);
    printf("%s\n", source);
    printf("len=%d\n", source_len);

    AST ast;
    ast.root = parse(source, source_len);
    print_tree(ast.root, 0);

    return 0;
}
