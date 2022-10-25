#define MAX_VARIABLES 1024
#define MAX_FUNCTIONS 1024
#define MAX_FUNCTION_PAREMETERS 8

enum Type {
    TYPE_U8,
    TYPE_U64,
    TYPE_S64,
    TYPE_F64,
    TYPE_STRING
};

enum SysCall {
    SYSCALL_NONE,
    SYSCALL_PRINT
};

struct Variable {
    char name[64];
    enum Type type;
    bool pointer; // Are we a pointer?
    void *value; // Pointer into program.memory
};

struct Scope {
    struct Variable variables[MAX_VARIABLES];
    int var_count;
    struct Scope *down, *up;
};

struct Function {
    char name[64];
    enum SysCall sys_function;
    int parameter_count;
    
    struct Scope *top_scope, *current_scope;
    
    struct Token *token; // The identifier of the function name
};

struct Position {
    struct Token *tok;
    struct Function *func; // The function tok is in.
};

struct Program {
    u8 *memory;
    u8 *memory_caret;
    u64 memory_size;
    
    struct Function functions[MAX_FUNCTIONS];
    struct Function *current_function;
    int function_count;
    
    struct Position call_stack[MAX_FUNCTIONS];
    int call_stack_count;
};

struct Interpreter {
    struct Tokenizer tokenizer;
    struct Program program;
};
