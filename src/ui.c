#define BUTTON_PADDING 8

typedef struct
{
    Vector2 Position;
    Vector2 Size;
    b32 Active;
    char *Text;
    s32 Id;
} button;

typedef s32 ui_id;

typedef struct
{
    ui_id Active;
    ui_id Hot;

    s32 FontSize;

    b32 MouseButtonPressed;
    b32 MouseButtonReleased;
    Vector2 MousePosition;
} ui;

b32 DoButton(ui *UI, button Button);

internal b32 PositionIsInsideButton(Vector2 Position, button Button)
{
    f32 X0 = Button.Position.x;
    f32 Y0 = Button.Position.y;
    f32 X1 = Button.Position.x + Button.Size.x;
    f32 Y1 = Button.Position.y + Button.Size.y;

    return ((Position.x >= X0) &&
            (Position.x <= X1) &&
            (Position.y >= Y0) &&
            (Position.y <= Y1));
}

b32 DoButton(ui *UI, button Button)
{
    b32 ButtonPressed = 0;
    Color UnderColor = (Color){20,20,20,225};
    Color InactiveColor = (Color){20,180,180,255};
    Color HotColor = (Color){40,120,120,255};
    Color ActiveColor = (Color){40,120,120,255};
    Color TextColor = (Color){0,0,0,255};

    b32 IsInside = PositionIsInsideButton(UI->MousePosition, Button);
    b32 IsActive = UI->Active == Button.Id;

    if (IsInside)
    {
        UI->Hot = Button.Id;
    }

    b32 IsHot = UI->Hot == Button.Id;

    if (UI->MouseButtonPressed)
    {
        if (IsHot)
        {
            if (!UI->Active)
            {
                UI->Active = Button.Id;
            }
        }
    }
    else if (UI->MouseButtonReleased)
    {
        if (IsHot && IsActive)
        {
            ButtonPressed = 1;
        }

        if (IsActive)
        {
            UI->Active = 0;
        }
    }

    Rectangle Rect = (Rectangle){Button.Position.x, Button.Position.y,
                                 Button.Size.x, Button.Size.y};
    Rectangle UnderRect = (Rectangle){Button.Position.x + 2.0f, Button.Position.y + 2.0f,
                                      Button.Size.x, Button.Size.y};

    Color ButtonColor = IsActive ? ActiveColor : InactiveColor;

    DrawRectangleLinesEx(UnderRect, 2.0f, UnderColor);
    DrawRectangle(Rect.x, Rect.y, Rect.width, Rect.height, ButtonColor);

    if (IsHot && (IsActive || !UI->Active))
    {
        DrawRectangleLinesEx(Rect, 2.0f, HotColor);
    }

    DrawText(Button.Text,
             Button.Position.x + BUTTON_PADDING, Button.Position.y + BUTTON_PADDING,
             UI->FontSize, TextColor);

    return ButtonPressed;
}
