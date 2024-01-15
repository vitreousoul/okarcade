#define internal static
#define global_variable static
#define debug_variable static

typedef uint8_t u8;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int32_t s32;

typedef uint32_t b32;

typedef float f32;

typedef size_t size;

#define Kilobytes(n) ((u64)1024 * (u64)(n))
#define Megabytes(n) ((u64)1024 * Kilobytes(n))
#define Gigabytes(n) ((u64)1024 * Megabytes(n))
