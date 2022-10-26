#define Kilobytes(x) ((u64)x*1024)
#define Megabytes(x) ((u64)x*1024*1024)
#define Gigabytes(x) ((u64)x*1024*1024*1024)
#define Terabytes(x) ((u64)x*1024*1024*1024*1024)

typedef int8_t  s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float f32;
typedef double f64;

#define Log(...) (printf(__VA_ARGS__), fflush(stdout))
#define Error(...) fprintf(stderr, __VA_ARGS__)
#define Assert(cond) if (!(cond)) {Error("Assertion failed at %s(%d)!\n", __FILE__, __LINE__), __debugbreak();}
#define Panic() Error("Panic at %s(%d)!\n", __FILE__, __LINE__), exit(1)
#define CompileError(interp, token, message) \
    (Error("Error: %s(%d)\n  " message "\n", \
           (interp)->tokenizer.file_name,    \
           (token)->line),                   \
           exit(1))
#define CompileError1(interp, token, message, param1) \
    (Error("Error: %s(%d)\n  " message "\n", \
           (interp)->tokenizer.file_name,    \
           (token)->line,                    \
           param1),                          \
           exit(1))

char *
read_entire_file(const char *file) {
    HANDLE hFile = CreateFile(
        file,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    DWORD count = GetFileSize(hFile, NULL);
    DWORD num_read = 0;

    char *out = calloc(1, count+1);
    memset(out, '^', count+1); // For debugging. If we see any ^'s, we know something has gone wrong.

    bool ok = ReadFile(hFile, out, count, &num_read, NULL);
    if (!ok) {
        Panic();
    }

    out[count] = 0;
            
    return out;
}
