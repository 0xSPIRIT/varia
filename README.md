# Varia - Programming Language

Varia is a C-style interpreted programming language.
This is a side-project, for times when I'm bored of
programming my main project (a game).

Example code is located at bin\test.v

It's in no way finished yet, it's still in very
early stages and there are lots of bugs. For example,
`return`, and `sum += b` don't work as yet. But,
the language has function calls & the call stack,
parameters & calling functions working correctly... So far

Compiler used: MSVC 2022, Editor: 4coder.

Syntax:
```c
// Function Declarations:
main :: (param1: int, param2: float, param3: string) {
    // Variable declarations are "name : type = value;"
    
    a : int = 3;
    b := 5; // Automatically figures out the type.
>   b = "string"; // This is NOT legal.
    // Function calls are the same as C.
    
    print(a); // Outputs 3 to the console.
}

sum :: (a: int, b: int) {
    sum := a;
    sum += b;
    return sum;
    // Note that "return a+b" is not legal because this language doesn't support expressions.
    // We don't use an Abstract Syntax Tree to figure out expressions.
}
  
```