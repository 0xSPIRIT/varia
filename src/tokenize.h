#define MAX_TOKEN_LENGTH 256

enum Token_Type {
    TOKEN_NONE,
    
    TOKEN_IDENTIFIER,
    TOKEN_LITERAL,

    TOKEN_COLON = ':',
    TOKEN_EQUAL = '=',
    TOKEN_ADD = '+',
    TOKEN_SUBTRACT = '-',
    TOKEN_END_STATEMENT = ';',
    TOKEN_OPEN_FUNCTION = '(',
    TOKEN_COMMA = ',',
    TOKEN_CLOSE_FUNCTION = ')',
    TOKEN_OPEN_SCOPE = '{',
    TOKEN_CLOSE_SCOPE = '}',
};

enum Identifier_Type {
    IDENTIFIER_NONE,
    IDENTIFIER_VARIABLE_OR_TYPE,
    IDENTIFIER_KEYWORD,      // eg: the struct in  "Vector :: struct {"
    IDENTIFIER_FUNCTION_DEF, // eg: the main in    "main :: () {"
    IDENTIFIER_FUNCTION_CALL,
    IDENTIFIER_STRUCT_DEF,   // eg: the Vector in  "Vector :: struct {"
};

struct Token {
    int line; // Line in source code file.
    
    enum Token_Type type;
    enum Identifier_Type identifier_type;
    char name[MAX_TOKEN_LENGTH];

    struct Token *prev, *next;
};

struct Tokenizer {
    char file_name[256];
    char *buffer; // The actual source file.
    
    int current_line;

    struct Token *token_start, *token_curr;
    unsigned token_count;
};
