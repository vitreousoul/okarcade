#define internal static
#define global_variable static
#define debug_variable static

typedef uint8_t    u8;
typedef uint16_t   u16;
typedef uint32_t   u32;
typedef uint64_t   u64;

typedef int8_t     s8;
typedef int16_t    s16;
typedef int32_t    s32;
typedef int64_t    s64;

typedef uint8_t    b8;
typedef uint32_t   b32;

typedef float      f32;

typedef size_t     size;


#define Kilobytes(n) (1024 * (n))
#define Megabytes(n) (1024 * Kilobytes(n))
#define Gigabytes(n) (1024 * Megabytes(n))
