f32 Min(f32 ValueA, f32 ValueB);
f32 Max(f32 ValueA, f32 ValueB);

Vector2 V2(f32 X, f32 Y);
Vector2 ClampV2(Vector2 VectorA, f32 Min, f32 Max);


Rectangle R2(f32 X, f32 Y, f32 Width, f32 Height);

inline f32 Min(f32 ValueA, f32 ValueB)
{
    f32 Result = ValueA < ValueB ? ValueA : ValueB;
    return Result;
}

inline f32 Max(f32 ValueA, f32 ValueB)
{
    f32 Result = ValueA > ValueB ? ValueA : ValueB;
    return Result;
}

inline Vector2 V2(f32 X, f32 Y)
{
    return (Vector2){X,Y};
}

inline Vector2 ClampV2(Vector2 V, f32 Min, f32 Max)
{
    Vector2 Result;

    Result.x = ClampF32(V.x, Min, Max);
    Result.y = ClampF32(V.y, Min, Max);

    return Result;
}


inline Rectangle R2(f32 X, f32 Y, f32 Width, f32 Height)
{
    return (Rectangle){X, Y, Width, Height};
}

/*
  Web-Specific Helpers
*/
#if defined(PLATFORM_WEB)
void InitRaylibCanvas(void);


void InitRaylibCanvas(void)
{
    UpdateCanvasDimensions();

    s32 CanvasWidth = GetCanvasWidth();
    s32 CanvasHeight = GetCanvasHeight();
    if (CanvasWidth > 0.0f && CanvasHeight > 0.0f)
    {
        SCREEN_WIDTH = CanvasWidth;
        SCREEN_HEIGHT = CanvasHeight;
    }
}
#endif
