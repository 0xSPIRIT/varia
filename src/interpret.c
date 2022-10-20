enum Type
get_type(const char *name) {
    enum Type result = 0;

    if (0==strcmp(name, "char") || 0==strcmp(name, "u8")) {
        result = TYPE_U8;
    } else if (0==strcmp(name, "s8")) {
        result = TYPE_S8;
    } else if (0==strcmp(name, "int") || 0==strcmp(name, "s16")) {
        result = TYPE_S16;
    } else if (0==strcmp(name, "float") || 0==strcmp(name, "f32")) {
        result = TYPE_F32;
    } else if (0==strcmp(name, "double") || 0==strcmp(name, "f64")) {
        result = TYPE_F64;
    } else if (0==strcmp(name, "string")) {
        result = TYPE_STRING;
    }

    return result;
}

unsigned
type_size(enum Type type) {
    unsigned result = 0;
    
    switch (type) {
    case TYPE_U8: case TYPE_S8:
        result = 8;
        break;
    case TYPE_U16: case TYPE_S16:
        result = 16;
        break;
    case TYPE_U32: case TYPE_S32:
        result = 32;
        break;
    case TYPE_U64: case TYPE_S64:
        result = 64;
        break;
    case TYPE_F32: 
        result = 32;
        break;
    case TYPE_F64:
        result = 64;
        break;
    }

    result /= 8;

    Assert(result > 0);
    return result;
}

void
program_setup(struct Interpreter *interp) {
    interp->program.memory_size = Megabytes(256);

    // Base address for the memory block so debugging
    // will be consistent (same memory addresses).
    LPVOID base_address = (LPVOID) Terabytes(2);
    interp->program.memory = VirtualAlloc(
        base_address,
        interp->program.memory_size,
        MEM_COMMIT|MEM_RESERVE,
        PAGE_READWRITE
    );
    interp->program.memory_caret = interp->program.memory;
}

void
program_free(struct Interpreter *interp) {
    VirtualFree(interp->program.memory, 0, MEM_RELEASE);
}

void
program_add_function(struct Program *program, struct Token *tok) {
    Assert(tok->type == TOKEN_IDENTIFIER);
    Assert(tok->identifier_type == IDENTIFIER_FUNCTION_DEF);
    Assert(program->function_count < MAX_FUNCTIONS);

    program->functions[program->function_count].token = tok;
    strcpy(program->functions[program->function_count].name, tok->name);

    program->function_count++;
}

void *
program_alloc(struct Program *program, u64 size) {
    Assert((u64)(program->memory_caret - program->memory)+size < program->memory_size);

    void *result = program->memory_caret;
    program->memory_caret += size;
    return result;
}

struct Variable *
program_add_variable(struct Program *program, const char *name, enum Type type, u64 size) {
    struct Variable *var = &program->variables[program->variable_count++];
    strcpy(var->name, name);
    var->type = type;
    var->pointer = false;
    var->value = program_alloc(program, size);
    return var;
}

struct Variable *
program_find_variable(struct Program *program, const char *name) {
    for (int i = 0; i < program->variable_count; i++) {
        if (0==strcmp(name, (char*)program->variables[i].name)) {
            return &program->variables[i];
        }
    }

    return NULL;
}

struct Function *
program_find_function(struct Program *program, const char *name) {
    for (int i = 0; i < program->function_count; i++) {
        if (0==strcmp(name, (char*)program->functions[i].name)) {
            return &program->functions[i];
        }
    }

    return NULL;
}

void
set_variable_from_str(void *ptr, char *str, enum Type type) {
    switch (type) {
    case TYPE_STRING:
        strcpy((char*)ptr, str);
        puts((char*)ptr);
        break;
                                
    case TYPE_U8:;
        u8 valueu8 = (u8) atoi(str);
        memcpy(ptr, &valueu8, type_size(type));
        break;
    case TYPE_U16:;
        u16 valueu16 = (u16) atoi(str);
        memcpy(ptr, &valueu16, type_size(type));
        break;
    case TYPE_U32:;
        u32 valueu32 = (u32) atoi(str);
        memcpy(ptr, &valueu32, type_size(type));
        break;
    case TYPE_U64:;
        u64 valueu64 = (u64) atoi(str);
        memcpy(ptr, &valueu64, type_size(type));
        break;

    case TYPE_S8:;
        s8 values8 = (s8) atoi(str);
        memcpy(ptr, &values8, type_size(type));
        break;
    case TYPE_S16:;
        s16 values16 = (s16) atoi(str);
        memcpy(ptr, &values16, type_size(type));
        break;
    case TYPE_S32:;
        s32 values32 = (s32) atoi(str);
        memcpy(ptr, &values32, type_size(type));
        break;
    case TYPE_S64:;
        s64 values64 = (s64) atoi(str);
        memcpy(ptr, &values64, type_size(type));
        break;
    }
}

void
handle_variable(struct Interpreter *interp, struct Token **tok) {
    struct Token *tok_variable_name = *tok;

    if ((*tok)->next->type == TOKEN_COLON) {
        // @Cleanup This seems dirty.
        struct Token *tok_colon = (*tok)->next;
        struct Token *tok_type = tok_colon->next;
        struct Token *tok_equals = tok_type->next;
        struct Token *tok_literal = tok_equals->next;

        // We're doing a variable declaration
        enum Type type = get_type(tok_type->name);

        u64 size = 0;
        if (type == TYPE_STRING && tok_equals->type == TOKEN_EQUAL) {
            tok_literal = tok_literal->next; // Miss out on the opening "
            size = strlen(tok_literal->name);
            printf("New variable %s = %s.\n", var->name, (*tok)->name);
        } else {
            size = type_size(type);
        }

        struct Variable *var = program_add_variable(&interp->program, (*tok)->name, type, size);
        *tok = tok_equals;

        // We're initializing as well.
        if ((*tok)->type == TOKEN_EQUAL) {
            *tok = tok_literal;

            // Get the value from the literal.
            if ((*tok)->type == TOKEN_LITERAL) {
                set_variable_from_str(var->value, (*tok)->name, type);
                printf("New variable %s = %s.\n", var->name, (*tok)->name);
            }
        }

        // We're at the semicolon now.
        *tok=(*tok)->next;
    } else if ((*tok)->next->type == TOKEN_EQUAL) {
        struct Token *tok_equals = tok_variable_name->next;
        struct Token *tok_literal = tok_equals->next;

        struct Variable *v = program_find_variable(&interp->program, tok_variable_name->name);
        if (v) {
            set_variable_from_str(v->value, tok_literal->name, v->type);
            printf("New value for %s: %d\n", (*tok)->name, *(int*)v->value);
        }
    }
}

// Runs the actual program.
void
interpret(struct Tokenizer tokenizer) {
    struct Interpreter interp = {0};

    interp.tokenizer = tokenizer;

    program_setup(&interp);

    struct Function *main_function = NULL;

    // Firstly, tag all functions.
    for (struct Token *tok = tokenizer.token_start; tok; tok = tok->next) {
        if (tok->identifier_type == IDENTIFIER_FUNCTION_DEF) {
            program_add_function(&interp.program, tok);
            if (0==strcmp(tok->name, "main")) {
                main_function = &interp.program.functions[interp.program.function_count-1];
            }
        }
    }

    if (!main_function) {
        Error("Main function was not defined!\n");
        program_free(&interp);
        exit(1);
    }

    struct Token *tok = main_function->token;
    
    // Start after the {
    while (tok->type != TOKEN_OPEN_SCOPE) tok=tok->next;
    tok=tok->next;

    while (tok) {
        if (tok->type == TOKEN_CLOSE_SCOPE) {
            if (interp.program.call_stack_count) {
                tok = interp.program.call_stack[--interp.program.call_stack_count];
            }
        } else if (tok->type == TOKEN_IDENTIFIER) {
            switch (tok->identifier_type) {
            case IDENTIFIER_VARIABLE_OR_TYPE:
                handle_variable(&interp, &tok);
                break;
            case IDENTIFIER_FUNCTION_CALL: {
                struct Function *func = program_find_function(&interp.program, tok->name);
                if (func) {
                    interp.program.call_stack[interp.program.call_stack_count++] = tok->next;
                    tok = func->token;

                    // Start after the {
                    while (tok->type != TOKEN_OPEN_SCOPE) tok = tok->next;
                }
                break;
            }
            }
        }
        tok = tok->next;
    }

    program_free(&interp);
}

