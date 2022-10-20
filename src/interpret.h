#define MAX_VARIABLES 1024
#define MAX_FUNCTIONS 1024

enum Type {
    TYPE_U8,
    TYPE_S8,
    TYPE_U16,
    TYPE_S16,
    TYPE_U32,
    TYPE_S32,
    TYPE_U64,
    TYPE_S64,
    TYPE_F32,
    TYPE_F64,
    TYPE_STRING
};

struct Function {
    char name[64];
    struct Token *token; // The identifier of the function name
};

struct Variable {
    char name[64];
    enum Type type;
    bool pointer; // Are we a pointer?
    void *value; // Pointer into program.memory
};

struct Program {
    u8 *memory;
    u8 *memory_caret;
    u64 memory_size;

    struct Function functions[MAX_FUNCTIONS];
    int function_count;
    struct Variable variables[MAX_VARIABLES]; // All global for now.
    int variable_count;

    struct Token *call_stack[MAX_FUNCTIONS];
    int call_stack_count;
};

struct Interpreter {
    struct Tokenizer tokenizer;
    struct Program program;
};
