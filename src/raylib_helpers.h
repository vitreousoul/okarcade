Vector2 V2(f32 X, f32 Y);
Rectangle R2(f32 X, f32 Y, f32 Width, f32 Height);


inline Vector2 V2(f32 X, f32 Y)
{
    return (Vector2){X,Y};
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
