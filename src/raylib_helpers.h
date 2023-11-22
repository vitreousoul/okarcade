Vector2 CreateVector2(f32 X, f32 Y);


Vector2 CreateVector2(f32 X, f32 Y)
{
    return (Vector2){X,Y};
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
