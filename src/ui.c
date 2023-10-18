#define BUTTON_PADDING 8

typedef struct
{
    Vector2 Position;
    b32 Active;
    u8 *Text;
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

button CreateButton(Vector2 Position, u8 *Text, ui_id Id);
b32 DoButton(ui *UI, button Button, s32 FontSize);

button CreateButton(Vector2 Position, u8 *Text, ui_id Id)
{
    button Button;

    Button.Position = Position;
    Button.Text = Text;
    Button.Id = Id;

    return Button;
}

internal b32 PositionIsInsideButton(Vector2 Position, button Button, Vector2 ButtonSize)
{
    f32 X0 = Button.Position.x;
    f32 Y0 = Button.Position.y;
    f32 X1 = Button.Position.x + ButtonSize.x;
    f32 Y1 = Button.Position.y + ButtonSize.y;

    return ((Position.x >= X0) &&
            (Position.x <= X1) &&
            (Position.y >= Y0) &&
            (Position.y <= Y1));
}

b32 DoButton(ui *UI, button Button, s32 FontSize)
{
    b32 ButtonPressed = 0;
    Color UnderColor = (Color){20,20,20,225};
    Color InactiveColor = (Color){20,180,180,255};
    Color HotColor = (Color){40,120,120,255};
    Color ActiveColor = (Color){40,120,120,255};
    Color TextColor = (Color){0,0,0,255};

    s32 TextWidth = MeasureText((char *)Button.Text, FontSize);
    f32 TwicePadding = 2 * BUTTON_PADDING;
    Vector2 ButtonSize = CreateVector2(TextWidth + TwicePadding, FontSize + TwicePadding);

    b32 IsInside = PositionIsInsideButton(UI->MousePosition, Button, ButtonSize);
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
                                 ButtonSize.x, ButtonSize.y};
    Rectangle UnderRect = (Rectangle){Button.Position.x + 2.0f, Button.Position.y + 2.0f,
                                      ButtonSize.x, ButtonSize.y};

    Color ButtonColor = IsActive ? ActiveColor : InactiveColor;

    DrawRectangleLinesEx(UnderRect, 2.0f, UnderColor);
    DrawRectangle(Rect.x, Rect.y, Rect.width, Rect.height, ButtonColor);

    if (IsHot && (IsActive || !UI->Active))
    {
        DrawRectangleLinesEx(Rect, 2.0f, HotColor);
    }

    DrawText((char *)Button.Text,
             Button.Position.x + BUTTON_PADDING, Button.Position.y + BUTTON_PADDING,
             UI->FontSize, TextColor);

    return ButtonPressed;
}
