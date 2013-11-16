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
}

void append_child(AstNode *parent, AstNode *child)
{
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

AstNode* parse_statement(ParseState *state)
{
    AstNode *node = NULL;
    char c;
    while (c= next_char(state)) {
        switch (c) {
        case '{':
            node = create_node(state->cur_node, BLOCK, state->source_pos - 1, 0);
            break;
        case '}':
            break;
        default:
            break;
        }
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

void print_tree(AstNode *node, int indent)
{
    if (!node)
        return;
    printf("type=%d\n", node->type);
    AstNode *child = node->first_child;
    while (child) {
        print_tree(child, indent + 1);
        child = child->next;
    }
}

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
    char source[] = "{var i}";
    int source_len = sizeof(source);
    printf("%s\n", source);
    printf("len=%d\n", source_len);

    AST ast;
    ast.root = parse(source, source_len);
    print_tree(ast.root, 0);

    return 0;
}
