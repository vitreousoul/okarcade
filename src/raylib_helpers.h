#define TARGET_SCREEN_WIDTH 1024
#define TARGET_SCREEN_HEIGHT 600

f32 Min(f32 ValueA, f32 ValueB);
f32 Max(f32 ValueA, f32 ValueB);

#define V2(x, y) ((Vector2){x, y})

Vector2 _V2(f32 X, f32 Y);
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

EM_JS(void, UpdateCanvasDimensions, (f32 TargetWidth, f32 TargetHeight), {
    var Canvas = document.getElementById("canvas");

    if (Canvas) {
        var CanvasRect = Canvas.getBoundingClientRect();
#if 0
        var CanvasRatio = CanvasRect.width / CanvasRect.height;

        var PixelCount = TargetWidth * TargetHeight;
        var PixelScale = PixelCount / (CanvasRect.width * CanvasRect.height);

        Canvas.width = Math.sqrt((PixelScale * CanvasRect.width * CanvasRect.height) * CanvasRatio);
        Canvas.height = Math.sqrt((PixelScale * CanvasRect.width * CanvasRect.height) / CanvasRatio);
#else
        Canvas.width = CanvasRect.width;
        Canvas.height = CanvasRect.height;
#endif
    }
});

EM_JS(s32, GetCanvasWidth, (void), {
    var canvas = document.getElementById("canvas");
    if (canvas) {
        var rect = canvas.getBoundingClientRect();
        return canvas.width;
    } else {
        return -1.0;
    }
});

EM_JS(s32, GetCanvasHeight, (void), {
    var canvas = document.getElementById("canvas");
    if (canvas) {
        var rect = canvas.getBoundingClientRect();
        return canvas.height;
    } else {
        return -1.0;
    }
});


void InitRaylibCanvas(void)
{
    UpdateCanvasDimensions(TARGET_SCREEN_WIDTH, TARGET_SCREEN_HEIGHT);
    f32 CanvasWidth = GetCanvasWidth();
    f32 CanvasHeight = GetCanvasHeight();

    if (CanvasWidth > 0.0f && CanvasHeight > 0.0f)
    {
        SCREEN_WIDTH = CanvasWidth;
        SCREEN_HEIGHT = CanvasHeight;
    }
}

#endif
