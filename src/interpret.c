struct Variable *
program_find_variable(struct Function *curr_func, const char *name) {
    struct Scope *scope = curr_func->current_scope;
    while (scope) {
        for (int i = 0; i < scope->var_count; i++) {
            if (0==strcmp(name, (char*)scope->variables[i].name)) {
                return &scope->variables[i];
            }
        }
        scope = scope->up;
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

enum Type
get_type(const char *name) {
    enum Type result = 0;
    
    if (0==strcmp(name, "char") || 0==strcmp(name, "u8")) {
        result = TYPE_U8;
    } else if (0==strcmp(name, "int") || 0==strcmp(name, "i64")) {
        result = TYPE_S64;
    } else if (0==strcmp(name, "u64")) {
        result = TYPE_U64;
    } else if (0==strcmp(name, "float") || 0==strcmp(name, "f64")) {
        result = TYPE_F64;
    } else if (0==strcmp(name, "string")) {
        result = TYPE_STRING;
    }
    
    return result;
}

enum Type
get_automatic_type(struct Program *program, const char *name) {
    enum Type result = 0;
    
    if (name[0] == '"') {
        result = TYPE_STRING;
        return result;
    }
    
    bool is_signed = false;
    bool is_number = true;
    bool has_decimal = false;
    bool is_identifier = false;
    bool is_string = false;
    
    if (name[0] == '-') {
        is_signed = true;
    }
    
    for (unsigned i = 0; i < strlen(name); i++) {
        char c = name[i];
        if (c == ' ') {
            is_identifier = true;
        } else if (!is_identifier && (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
            is_string = true;
        } else if (c == '.') {
            has_decimal = true;
        } else if (!(c >= '0' && c <= '9')) {
            is_number = false;
        }
    }
    
    // TODO: Characters (U8)
    if (is_identifier) {
        struct Variable *v = program_find_variable(program->current_function, name);
        Assert(v);
        result = v->type;
    } else if (is_string) {
        result = TYPE_STRING;
    } else if (has_decimal) {
        result = TYPE_F64;
    } else if (is_signed) {
        result = TYPE_S64;
    } else {
        result = TYPE_U64;
    }
    
    return result;
}

void
function_setup_scope(struct Function *function) {
    function->top_scope = calloc(1, sizeof(struct Scope));
    function->current_scope = function->top_scope;
}

u64
type_size_notstr(enum Type type) {
    u64 result = 0;
    
    Assert(type != TYPE_STRING);
    
    switch (type) {
        case TYPE_U8: {
            result = 8;
            break;
        }
        case TYPE_U64: case TYPE_S64: {
            result = 64;
            break;
        }
        case TYPE_F64: {
            result = 64;
            break;
        }
    }
    
    result /= 8;
    
    Assert(result > 0);
    
    return result;
}

u64
type_size(struct Variable *v) {
    enum Type type = v->type;
    u64 result = 0;
    
    switch (type) {
        case TYPE_STRING: {
            Assert(((char*)v->value)[0] != '"'); // Should be snipped out by now.
            result = strlen((char*)v->value);
            break;
        }
        case TYPE_U8: {
            result = 8;
            break;
        }
        case TYPE_U64: case TYPE_S64: {
            result = 64;
            break;
        }
        case TYPE_F64: {
            result = 64;
            break;
        }
    }
    
    result /= 8;
    
    Assert(result > 0);
    
    return result;
}

void
print(struct Variable *v) {
    void *ptr = v->value;
    enum Type type = v->type;
    
    switch (type) {
        case TYPE_STRING: {
            printf("%s", (char*)ptr);
            break;
        }
        
        case TYPE_U8: {
            putchar(*(uint8_t*)ptr);
            break;
        }
        
        case TYPE_S64: {
            printf("%zd\n", *(s64*)ptr);
            break;
        }
        
        case TYPE_U64: {
            printf("%zu\n", *(u64*)ptr);
            break;
        }
        
        case TYPE_F64: {
            printf("%lf\n", *(f64*)ptr);
            break;
        }
    }
}

void *
program_alloc(struct Program *program, u64 size) {
    Assert((u64)(program->memory_caret - program->memory)+size < program->memory_size);
    
    void *result = program->memory_caret;
    program->memory_caret += size;
    return result;
}

struct Variable *
program_add_variable(struct Program *program,
                     struct Scope *scope,
                     const char *name,
                     enum Type type,
                     u64 size)
{
    struct Variable *var = &scope->variables[scope->var_count++];
    
    strcpy(var->name, name);
    var->type = type;
    var->pointer = false;
    var->value = program_alloc(program, size);
    return var;
}

void
program_setup_syscalls(struct Program *program) {
    struct Function *fun = &program->functions[program->function_count];
    function_setup_scope(fun);
    strcpy(fun->name, "print");
    fun->sys_function = SYSCALL_PRINT;
    program_add_variable(program, fun->top_scope, "_print_var", TYPE_U64, 64);
    ++program->function_count;
}

void
program_setup(struct Interpreter *interp) {
    interp->program.memory_size = Megabytes(256);
    
    LPVOID base_address = (LPVOID) 0;
    interp->program.memory = VirtualAlloc(base_address,
                                          interp->program.memory_size,
                                          MEM_COMMIT|MEM_RESERVE,
                                          PAGE_READWRITE);
    if (!interp->program.memory) {
        Error("VirtualAlloc() error! Win32 Error Code: %d\n", GetLastError());
        exit(1);
    }
    
    interp->program.memory_caret = interp->program.memory;
    
    program_setup_syscalls(&interp->program);
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
    
    struct Function *fun = &program->functions[program->function_count];
    
    fun->token = tok;
    strcpy(fun->name, tok->name);
    
    function_setup_scope(fun);
    
    // Now, we add the function parameters.
    
    tok = tok->next->next->next->next; // Pass the :: (
    
    if (tok->type != TOKEN_CLOSE_FUNCTION) {
        // Format of a parameter is "variable : type,"
        Assert(tok->type == TOKEN_IDENTIFIER);
        
        while (tok->type != TOKEN_CLOSE_FUNCTION) {
            struct Token *type_token = tok->next->next;
            enum Type type = get_type(type_token->name);
            
            // We use the top scope for the function parameters,
            // since that is used globally in the function.
            program_add_variable(program,
                                 fun->top_scope,
                                 tok->name,
                                 type,
                                 type_size_notstr(type));
            
            tok = type_token->next;
            if (tok->type == TOKEN_COMMA)
                tok = tok->next;
        }
    } else {
        // We don't have any parameters.
    }
    
    program->function_count++;
}

void
copy_variable(struct Variable *dest, struct Variable *src) {
    Assert(dest->type == src->type);
    if (dest->type == TYPE_STRING) {
        strcpy((char*)dest->value, (char*)src->value);
    } else {
        memcpy(dest->value, src->value, type_size(dest));
    }
}

void
set_variable_from_str(void *ptr, char *str, enum Type type) {
    switch (type) {
        case TYPE_STRING: {
            // Remove surrounding ""
            str++;
            str[strlen(str)-1] = 0;
            
            strcpy((char*)ptr, str);
            break;
        }
        
        case TYPE_U8: {
            u8 valueu8 = (u8) atoi(str);
            memcpy(ptr, &valueu8, type_size_notstr(type));
            break;
        }
        case TYPE_U64: {
            u64 valueu64 = (u64) atoi(str);
            memcpy(ptr, &valueu64, type_size_notstr(type));
            break;
        }
        case TYPE_S64: {
            s64 values64 = (s64) atoi(str);
            memcpy(ptr, &values64, type_size_notstr(type));
            break;
        }
    }
}

void
handle_variable(struct Interpreter *interp, struct Token **tok) {
    struct Token *tok_variable_name = *tok;
    
    struct Function *current_function = interp->program.current_function;
    
    if ((*tok)->next->type == TOKEN_COLON) {
        // @Cleanup This seems dirty.
        struct Token *tok_colon = (*tok)->next;
        struct Token *tok_type = tok_colon->next;
        struct Token *tok_equals = tok_type->next;
        struct Token *tok_literal = tok_equals->next;
        
        bool is_automatic = false;
        
        if (tok_colon->type == TOKEN_COLON && tok_colon->next->type == TOKEN_EQUAL) {
            is_automatic = true;
            tok_type = NULL;
            tok_equals = tok_colon->next;
            tok_literal = tok_equals->next;
        }
        
        // We're doing a variable declaration
        enum Type type;
        
        if (!is_automatic) {
            type = get_type(tok_type->name);
        } else {
            type = get_automatic_type(&interp->program, tok_literal->name);
        }
        
        u64 size = 0;
        if (type == TYPE_STRING && tok_equals->type == TOKEN_EQUAL) {
            size = strlen(tok_literal->name) - 2 + 1; // -2 for the surrounding "" and +1 for the null terminator.
        } else if (type == TYPE_STRING) {
            Error("Must initialize a string to something at %s(%d)\n", interp->tokenizer.file_name, (*tok)->line);
            exit(1);
        } else {
            size = type_size_notstr(type);
        }
        
        struct Variable *var = program_add_variable(&interp->program, current_function->current_scope, (*tok)->name, type, size);
        *tok = tok_equals;
        
        // We're initializing as well.
        if ((*tok)->type == TOKEN_EQUAL) {
            *tok = tok_literal;
            
            // Get the value from the literal.
            if (tok_literal->type == TOKEN_LITERAL) {
                set_variable_from_str(var->value, tok_literal->name, type);
                //printf("New variable %s = %s\n", var->name, tok_literal->name);
            } else if (tok_literal->type == TOKEN_IDENTIFIER) {
                // Copy the variable.
                struct Variable *var_set_to = program_find_variable(current_function, tok_literal->name);
                copy_variable(var, var_set_to);
                //printf("Set value for %s to the value of %s: ", var->name, tok_literal->name);
                //print(var);
            }
        }
        
        // We're at the semicolon now.
        *tok=(*tok)->next;
    } else if ((*tok)->next->type == TOKEN_EQUAL) {
        struct Token *tok_equals = tok_variable_name->next;
        struct Token *tok_literal = tok_equals->next;
        
        struct Variable *v = program_find_variable(current_function, tok_variable_name->name);
        Assert(v); // If v==NULL, that variable hasn't been declared in this scope.
        
        set_variable_from_str(v->value, tok_literal->name, v->type);
        //printf("New value for %s: %d\n", (*tok)->name, *(int*)v->value);
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
    interp.program.current_function = main_function;
    
    // Start after the {
    while (tok->type != TOKEN_OPEN_SCOPE) tok=tok->next;
    tok=tok->next;
    
    while (tok) {
        if (tok->type == TOKEN_CLOSE_SCOPE) {
            if (interp.program.call_stack_count) {
                struct Position pos = interp.program.call_stack[--interp.program.call_stack_count];
                tok = pos.tok;
                interp.program.current_function = pos.func;
            }
        } else if (tok->type == TOKEN_IDENTIFIER) {
            switch (tok->identifier_type) {
                case IDENTIFIER_VARIABLE_OR_TYPE: {
                    handle_variable(&interp, &tok);
                    break;
                }
                
                case IDENTIFIER_FUNCTION_CALL: {
                    struct Function *func = program_find_function(&interp.program, tok->name);
                    
                    tok = tok->next->next;
                    
                    int i = 0;
                    
                    // Ensure that we have the correct amount of parameters.
                    while (tok->type != TOKEN_CLOSE_FUNCTION) {
                        struct Token *param_tok = tok;
                        
                        // Note:
                        //   To get parameters, we simply index
                        //   into the variables array since the
                        //   first `parameter_count` members
                        //   of the variables in that is always
                        //   the parameters.
                        struct Variable *param =
                            &func->top_scope->variables[i];
                        
                        if (param_tok->type == TOKEN_IDENTIFIER) {
                            struct Variable *v = NULL;
                            v = program_find_variable(interp.program.current_function, param_tok->name);
                            
                            if (!v) {
                                Error("%s was not defined at %s(%d)\n", param_tok->name, interp.tokenizer.file_name, param_tok->line);
                                exit(1);
                            }
                            
                            // Special case for Print: make the variable
                            // type dynamically the same as the input.
                            if (func->sys_function == SYSCALL_PRINT) {
                                param->type = v->type;
                            }
                            
                            copy_variable(param, v);
                            
                            //print(func->parameters[i]);
                        } else if (param_tok->type == TOKEN_LITERAL) {
                            // We can just copy this data into
                            // the function parameter.
                            
                            // Special case for Print: make the variable
                            // type dynamically the same as the input.
                            if (func->sys_function == SYSCALL_PRINT) {
                                param->type = get_automatic_type(&interp.program, param_tok->name);
                            }
                            
                            set_variable_from_str(param->value, param_tok->name, param->type);
                            //printf("Set value for parameter to %s\n", param_tok->name);
                        } else {
                            Assert(0);
                        }
                        
                        tok = param_tok->next;
                        if (tok->type == TOKEN_COMMA) {
                            tok = tok->next;
                        }
                        
                        i++;
                    }
                    
                    if (func) {
                        if (func->sys_function) {
                            switch (func->sys_function) {
                                case SYSCALL_PRINT: {
                                    print(&func->top_scope->variables[0]);
                                    tok = tok->next->next;
                                    continue;
                                }
                            }
                        } else {
                            interp.program.call_stack[interp.program.call_stack_count++] = (struct Position){
                                tok->next,
                                interp.program.current_function
                            };
                            tok = func->token;
                            interp.program.current_function = func;
                        }
                        
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

