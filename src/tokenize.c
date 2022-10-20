void
token_new(
    struct Tokenizer *tokenizer,
    enum Token_Type type,
    char data[MAX_TOKEN_LENGTH]
) {
    struct Token *token = calloc(1, sizeof(struct Token));

    token->type = type;
    memcpy(token->name, data, strlen(data));

    if (!tokenizer->token_start) {
        tokenizer->token_start = token;
    } else {
        token->prev = tokenizer->token_curr;
        tokenizer->token_curr->next = token;
    }
    
    tokenizer->token_curr = token;

    tokenizer->token_count++;
}

bool
is_special_char(char c) {
    switch (c) {
    case TOKEN_COLON: case TOKEN_EQUAL: case TOKEN_ADD:
    case TOKEN_SUBTRACT: case TOKEN_END_STATEMENT:
    case TOKEN_OPEN_FUNCTION: case TOKEN_CLOSE_FUNCTION:
    case TOKEN_OPEN_SCOPE: case TOKEN_CLOSE_SCOPE:
    case TOKEN_COMMA:
        return true;
    }
    return false;
}

bool
is_valid_identifier_char(bool first, char c) {
    if (!first) {
        if (c >= '0' && c <= '9') {
            return true;
        }
    }

    if (c == '_' || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) {
        return true;
    }

    return false;
}

bool
is_literal_char(char c) {
    return c >= '0' && c <= '9';
}

bool
is_whitespace(char c) {
    switch (c) {
    case '\n': case '\r': case ' ': case '\t':
        return true;
    }
    return false;
}

// function :: (...) {...}
bool
is_function_def(struct Token *token) {
    Assert(token);
    if (!token->next || !token->next->next || !token->next->next->next) return false;
    if (token->type != TOKEN_IDENTIFIER) return false;
    
    if (token->next->type == TOKEN_COLON &&
        token->next->next->type == TOKEN_COLON &&
        token->next->next->next->type == TOKEN_OPEN_FUNCTION)
    {
        return true;
    }
    return false;
}

bool
is_function_call(struct Token *token) {
    Assert(token);
    if (!token->next) return false;
    if (token->type != TOKEN_IDENTIFIER) return false;

    if (token->next->type == TOKEN_OPEN_FUNCTION)
        return true;
    return false;
}

// Vector :: struct {
bool
is_struct_name(struct Token *token) {
    Assert(token);
    if (!token->next || !token->next->next || !token->next->next->next) return false;
    if (token->type != TOKEN_IDENTIFIER) return false;

    if (token->next->type == TOKEN_COLON &&
        token->next->next->type == TOKEN_COLON &&
        0==strcmp(token->next->next->next->name, "struct"))
    {
        return true;
    }
    return false;
}

struct Tokenizer
tokenize(char *source_buffer) {
    struct Tokenizer tokenizer = {0};

    tokenizer.buffer = source_buffer;

    char *s = tokenizer.buffer;

    enum Token_Type current_token_type = TOKEN_NONE;
    char current_token[MAX_TOKEN_LENGTH] = {0};
    int current_token_len = 0;

    bool string = false; // Are we in a string?

    while (*s) {
        if (string) {
            if (*s == '"') {
                string = false;

                // At this point, current_token_type = TOKEN_LITERAL
                // unless the string looks like this ""
                token_new(&tokenizer, current_token_type, current_token);
                memset(current_token, 0, MAX_TOKEN_LENGTH);
                current_token_len = 0;
            
                ++s;
                continue;
            }

            current_token[current_token_len++] = *s;
            current_token_type = TOKEN_LITERAL;

            ++s;
            continue;
        }

        if (is_whitespace(*s)) {
            // Close off the current identifier if we have one.
            if (current_token_len) {
                token_new(&tokenizer, current_token_type, current_token);
                memset(current_token, 0, MAX_TOKEN_LENGTH);
                current_token_len = 0;
            }
            current_token_type = 0;

            ++s;
            continue;
        }

        // Check if the current char is any special character.
        if (is_special_char(*s)) {
            // Close off the current identifier if we have one.
            if (current_token_len) {
                token_new(&tokenizer, current_token_type, current_token);
                memset(current_token, 0, MAX_TOKEN_LENGTH);
                current_token_len = 0;
            }

            if (*s == '"') {
                string = true;
            } else {
                char data[MAX_TOKEN_LENGTH] = {*s, 0};
                token_new(&tokenizer, *s, data);
            }
        } else if (current_token_type != TOKEN_LITERAL &&
                   is_valid_identifier_char(current_token_len == 0, *s))
        {
            Assert(current_token_len <= MAX_TOKEN_LENGTH);
            current_token[current_token_len++] = *s;
            current_token_type = TOKEN_IDENTIFIER;
        } else if (is_literal_char(*s)) {
            Assert(current_token_type == TOKEN_LITERAL || current_token_len == 0); // We can't start a literal while we're in another token!
            current_token[current_token_len++] = *s;
            current_token_type = TOKEN_LITERAL;
        }

        ++s;
    }

    // Set all identifier types.
    for (struct Token *tok = tokenizer.token_start; tok; tok = tok->next) {
        if (tok->type != TOKEN_IDENTIFIER) continue;

        tok->identifier_type = IDENTIFIER_NONE;

        if (0==strcmp(tok->name, "struct")) {
            tok->identifier_type = IDENTIFIER_KEYWORD;
        } else if (is_function_def(tok)) {
            tok->identifier_type = IDENTIFIER_FUNCTION_DEF;
        } else if (is_function_call(tok)) {
            tok->identifier_type = IDENTIFIER_FUNCTION_CALL;
        } else if (is_struct_name(tok)) {
            tok->identifier_type = IDENTIFIER_STRUCT_DEF;
        } else {
            tok->identifier_type = IDENTIFIER_VARIABLE_OR_TYPE;
        }
    }

    return tokenizer;
}

void
print_tokens(struct Tokenizer *tokenizer) {
    struct Token *start = tokenizer->token_start;
    printf("Token Count: %u\n", tokenizer->token_count);

    for (struct Token *tok = start; tok; tok = tok->next) {
        enum Token_Type type = tok->type;
        
        char name[32] = {0};
        switch (type) {
        case TOKEN_IDENTIFIER: strcpy(name, "Identifier"); break;
        case TOKEN_LITERAL: strcpy(name, "Literal"); break;
        }

        char identifier_type[256] = {0};
        if (type == TOKEN_IDENTIFIER) {
            char str[64] = {0};

            switch (tok->identifier_type) {
            case IDENTIFIER_NONE:
                strcpy(str, "None");
                break;
            case IDENTIFIER_KEYWORD:
                strcpy(str, "Keyword");
                break;
            case IDENTIFIER_FUNCTION_DEF:
                strcpy(str, "Function Def");
                break;
            case IDENTIFIER_FUNCTION_CALL:
                strcpy(str, "Function Call");
                break;
            case IDENTIFIER_STRUCT_DEF:
                strcpy(str, "Struct Def");
                break;
            case IDENTIFIER_VARIABLE_OR_TYPE:
                strcpy(str, "Variable / Type");
                break;
            }

            sprintf(identifier_type, " | Identifier Type: %s", str);
        }

        printf("Token: %s \"%s\"%s\n", name, tok->name, identifier_type);
    }
}
