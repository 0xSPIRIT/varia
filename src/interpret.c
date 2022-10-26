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
    } else if (0==strcmp(name, "float") || 0==strcmp(name, "f64")) {
        result = TYPE_F64;
    } else if (0==strcmp(name, "string")) {
        result = TYPE_STRING;
    }
    
    return result;
}

// For example,
// converting the \n to an actual newline instead of backslash and n.
void
parse_string(char *output_string, char *input_string) {
    char *s = input_string;
    u64 length = strlen(s);
    u64 output_len = 0;
    
    for (u64 i = 0; i < length; i++) {
        if (s[i] == '\\') {
            i++;
            switch (s[i]) {
                case 'n': {
                    output_string[output_len++] = '\n';
                    break;
                }
                case 'r': {
                    output_string[output_len++] = '\r';
                    break;
                }
                case 't': {
                    output_string[output_len++] = '\t';
                    break;
                }
                case '\\': {
                    output_string[output_len++] = '\\';
                    break;
                }
            }
        } else {
            output_string[output_len++] = s[i];
        }
    }
}

enum Type
get_automatic_type_literal(const char *name) {
    if (*name == '"') {
        return TYPE_STRING;
    }
    
    for (unsigned i = 0; i < strlen(name); i++) {
        char c = name[i];
        if (c == '.') {
            return TYPE_F64;
        }
    }
    
    return TYPE_S64;
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
    
    if (name[0] == '-') {
        is_signed = true;
    }
    
    for (unsigned i = 0; i < strlen(name); i++) {
        char c = name[i];
        if (c == ' ' || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
            is_identifier = true;
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
    }  else if (has_decimal) {
        result = TYPE_F64;
    } else {
        result = TYPE_S64;
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
    
    Assert(type != TYPE_NONE);
    Assert(type != TYPE_STRING);
    
    switch (type) {
        case TYPE_U8: {
            result = 8;
            break;
        }
        case TYPE_S64: {
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
        case TYPE_S64: {
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
            Log("%s", (char*)ptr);
            break;
        }
        
        case TYPE_U8: {
            putchar(*(uint8_t*)ptr);
            break;
        }
        
        case TYPE_S64: {
            Log("%zd\n", *(s64*)ptr);
            break;
        }
        
        case TYPE_F64: {
            Log("%lf\n", *(f64*)ptr);
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
                     bool is_pointer,
                     u64 size)
{
    struct Variable *var = &scope->variables[scope->var_count++];
    strcpy(var->name, name);
    var->is_pointer = is_pointer;
    var->type = type;
    if (is_pointer) {
        var->value = program_alloc(program, sizeof(u64));
        // All pointers are u64.
    } else {
        var->value = program_alloc(program, size);
    }
    return var;
}

void
program_setup_syscalls(struct Program *program) {
    {
        struct Function *fun = &program->functions[program->function_count];
        function_setup_scope(fun);
        strcpy(fun->name, "print");
        fun->sys_function = SYSCALL_PRINT;
        program_add_variable(program,
                             fun->top_scope,
                             "_print_var",
                             TYPE_S64,
                             false,
                             64);
        fun->parameter_count = 1;
        
        ++program->function_count;
    }
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
    // TODO: Free all scopes, and everything else we may have allocated.
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
                                 false,
                                 type_size_notstr(type));
            
            tok = type_token->next;
            if (tok->type == TOKEN_COMMA)
                tok = tok->next;
            
            fun->parameter_count++;
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
get_variable_from_str(void *ptr, char *str, enum Type type) {
    Assert(ptr);
    Assert(str);
    
    switch (type) {
        case TYPE_STRING: {
            // Remove surrounding ""
            str++;
            str[strlen(str)-1] = 0;
            
            parse_string((char*)ptr, str);
            break;
        }
        
        case TYPE_U8: {
            u8 valueu8 = (u8) atoi(str);
            memcpy(ptr, &valueu8, type_size_notstr(type));
            break;
        }
        case TYPE_F64: {
            f64 valuesf64 = (f64) atof(str);
            memcpy(ptr, &valuesf64, type_size_notstr(type));
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
get_variable_from_literal_or_identifier(struct Interpreter *interp, void *ptr, struct Token *token) {
    if (token->type == TOKEN_LITERAL) {
        enum Type type = get_automatic_type(&interp->program, token->name);
        
        get_variable_from_str(ptr, token->name, type);
        //Log("New out_variable %s = %s\n", out_var->name, a->name);
    } else if (token->type == TOKEN_IDENTIFIER) {
        // Copy the out_variable.
        struct Variable *var_set_to = program_find_variable(interp->program.current_function, token->name);
        
        Assert(var_set_to);
        
        if (var_set_to->type == TYPE_STRING) {
            strcpy((char*)ptr, (char*)var_set_to->value);
        } else {
            memcpy(ptr, var_set_to->value, type_size(var_set_to));
        }
    }
}

void
make_sure_tokens_are_same_type(struct Interpreter *interp, struct Token *a, struct Token *b) {
    enum Type a_type = get_automatic_type(&interp->program, a->name);
    enum Type b_type = get_automatic_type(&interp->program, b->name);
    
    if (a_type != b_type) {
        CompileError(interp, a, "Expression must have the same type for both operands.");
    }
}

// expr - Pointer to the start of the expression
// token_count - How many tokens do the expression take?
void
evaluate_expression(struct Interpreter *interp,
                    enum Type output_type,
                    struct Token *expr,
                    int token_count,
                    struct Variable *output_var)
{
    // tok_1 operation tok_2 <- This is the only valid expression here.
    Assert(token_count == 3);
    
    Assert(output_var);
    
    if (output_type == 0) {
        Assert(output_var->type == 0); // Must not be set yet.
        Assert(output_var->value == NULL); // Must not be allocated as yet.
    }
    
    struct Token *a = expr;
    struct Token *operation = a->next;
    struct Token *b = operation->next;
    
    make_sure_tokens_are_same_type(interp, a, b);
    
    if (output_type == 0) {
        output_type = get_automatic_type(&interp->program, a->name);
    } else if (output_type != get_automatic_type(&interp->program, a->name)) {
        CompileError(interp, a, "Type of variable is not equal to the expression return type");
    }
    
    output_var->type = output_type;
    
    if (!output_var->value) {
        u64 size = type_size_notstr(output_var->type);
        output_var->value = program_alloc(&interp->program, size);
    }
    
    s64 a_s64 = 0, b_s64 = 0;
    f64 a_f64 = 0, b_f64 = 0;
    
    if (output_type == TYPE_S64) {
        get_variable_from_literal_or_identifier(interp, &a_s64, a);
        get_variable_from_literal_or_identifier(interp, &b_s64, b);
    } else if (output_type == TYPE_F64) {
        get_variable_from_literal_or_identifier(interp, &a_f64, a);
        get_variable_from_literal_or_identifier(interp, &b_f64, b);
    }
    
    switch (operation->type) {
        case TOKEN_ADD: {
            if (output_type == TYPE_S64) {
                s64 result = a_s64 + b_s64;
                *(s64*)output_var->value = result;
            } else if (output_type == TYPE_F64) {
                f64 result = a_f64 + b_f64;
                *(f64*)output_var->value = result;
            }
            break;
        }
        case TOKEN_SUBTRACT: {
            if (output_type == TYPE_S64) {
                s64 result = a_s64 - b_s64;
                *(s64*)output_var->value = result;
            } else if (output_type == TYPE_F64) {
                f64 result = a_f64 - b_f64;
                *(f64*)output_var->value = result;
            }
            break;
        }
        case TOKEN_DIVIDE: {
            if (output_type == TYPE_S64) {
                s64 result = a_s64 / b_s64;
                *(s64*)output_var->value = result;
            } else if (output_type == TYPE_F64) {
                f64 result = a_f64 / b_f64;
                *(f64*)output_var->value = result;
            }
            break;
        }
        case TOKEN_MULTIPLY: {
            if (output_type == TYPE_S64) {
                s64 result = a_s64 * b_s64;
                *(s64*)output_var->value = result;
            } else if (output_type == TYPE_F64) {
                f64 result = a_f64 * b_f64;
                *(f64*)output_var->value = result;
            }
            break;
        }
    }
}

void
handle_variable(struct Interpreter *interp, struct Token **tok) {
    struct Token *tok_variable_name = *tok;
    
    struct Function *current_function = interp->program.current_function;
    
    if ((*tok)->next->type == TOKEN_COLON) {
        bool is_pointer = (*tok)->next->next->type == TOKEN_POINTER;
        
        struct Token *tok_colon, *tok_ptr, *tok_type, *tok_equals, *tok_literal;
        
        tok_colon = (*tok)->next;
        if (is_pointer) {
            tok_ptr = tok_colon->next;
            tok_type = tok_ptr->next;
        } else {
            tok_type = tok_colon->next;
        }
        
        tok_equals = tok_type->next;
        tok_literal = tok_equals->next;
        
        bool is_automatic = false;
        
        if (is_automatic && tok_equals->next->type == TOKEN_ADDRESS) {
            CompileError(interp, *tok, "Must declare specifically the type of a pointer.");
        }
        
        if (tok_colon->type == TOKEN_COLON && tok_colon->next->type == TOKEN_EQUAL) {
            is_automatic = true;
            tok_type = NULL;
            tok_equals = tok_colon->next;
            tok_literal = tok_equals->next;
        }
        
        // We're doing a variable declaration
        enum Type type = 0;
        
        // Is the part after the equals sign an expression?
        bool is_expression = tok_literal->next->type != TOKEN_END_STATEMENT;
        
        if (!is_automatic) {
            type = get_type(tok_type->name);
        } else {
            // We can't figure out the type if it's an expression.
            if (!is_expression) {
                type = get_automatic_type(&interp->program, tok_literal->name);
            }
        }
        
        u64 size = 0;
        if (type == TYPE_NONE) {
            // ...
        } else if (type == TYPE_STRING && tok_equals->type == TOKEN_EQUAL) {
            size = strlen(tok_literal->name) - 2 + 1; // -2 for the surrounding "" and +1 for the null terminator.
        } else if (type == TYPE_STRING) {
            CompileError(interp, *tok, "Must initialize a string to something.");
        } else {
            size = type_size_notstr(type);
        }
        
        *tok = tok_equals;
        
        // We're initializing as well.
        if ((*tok)->type == TOKEN_EQUAL) {
            if (is_expression) {
                // program_add_variable won't work for us here because
                // we don't know the type yet.
                struct Scope *scope = current_function->current_scope;
                struct Variable *var = &scope->variables[scope->var_count++];
                var->is_pointer = is_pointer;
                strcpy(var->name, tok_variable_name->name);
                evaluate_expression(interp, type, tok_literal, 3, var);
            } else {
                struct Variable *var = program_add_variable(&interp->program,
                                                            current_function->current_scope,
                                                            tok_variable_name->name,
                                                            type,
                                                            is_pointer,
                                                            size);
                                
                // Get the value from the literal.
                if (tok_literal->type == TOKEN_LITERAL) {
                    get_variable_from_str(var->value, tok_literal->name, type);
                    //Log("New variable %s = %s\n", var->name, tok_literal->name);
                } else if (tok_literal->type == TOKEN_IDENTIFIER) {
                    // Copy the variable.
                    struct Variable *var_set_to = program_find_variable(current_function, tok_literal->name);
                    copy_variable(var, var_set_to);
                }
            }
        }
        
        // We're at the semicolon now.
        *tok=(*tok)->next;
    } else if ((*tok)->next->type == TOKEN_EQUAL) {
        struct Token *tok_equals = tok_variable_name->next;
        struct Token *tok_literal = tok_equals->next;
        
        struct Variable *v = program_find_variable(current_function,
                                                   tok_variable_name->name);
        if (!v) {
            CompileError1(interp, tok_variable_name,
                          "%s is not defined", tok_variable_name->name);
        }
        Assert(v); // Make sure it's declared.
        
        bool is_expression = tok_literal->next->type != TOKEN_END_STATEMENT;
        
        if (is_expression) {
            evaluate_expression(interp, v->type, tok_literal, 3, v);
        } else {
            get_variable_from_str(v->value, tok_literal->name, v->type);
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
                    
                    struct Token *function_start_token = tok;
                    
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
                                CompileError1(&interp, param_tok, "%s was not defined", param_tok->name);
                            }
                            
                            // Special case for Print: make the variable
                            // type dynamically the same as the input.
                            if (func->sys_function == SYSCALL_PRINT) {
                                // Print doesn't take more than one parameter.
                                if (i > 0) {
                                    CompileError(&interp, param_tok, "print() only takes one parameter");
                                }
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
                            
                            get_variable_from_str(param->value, param_tok->name, param->type);
                            //Log("Set value for parameter to %s\n", param_tok->name);
                        } else {
                            Assert(0);
                        }
                        
                        tok = param_tok->next;
                        if (tok->type == TOKEN_COMMA) {
                            tok = tok->next;
                        }
                        
                        i++;
                    }
                    
                    if (i != func->parameter_count) {
                        // char outstr[128] = {0};
                        // sprintf(outstr, "Function %s takes %d arguments, not %d!\n", func->name, func->parameter_count, i);
                        CompileError1(&interp, function_start_token, "Function %s does not take that amount of arguments!", func->name);
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

