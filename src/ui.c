#define BUTTON_PADDING 8

typedef struct
{
    s32 Id;

    Vector2 Position;
    u8 *Text;
} button;

typedef struct
{
    s32 Id;

    Vector2 Position;
    Vector2 Size;
    f32 Value;
} slider;

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
b32 DoButton(ui *UI, button *Button, s32 FontSize);
b32 DoSlider(ui *UI, slider *Slider);

button CreateButton(Vector2 Position, u8 *Text, ui_id Id)
{
    button Button;

    Button.Position = Position;
    Button.Text = Text;
    Button.Id = Id;

    return Button;
}

internal b32 PositionIsInsideButton(Vector2 Position, button *Button, Vector2 ButtonSize)
{
    f32 X0 = Button->Position.x;
    f32 Y0 = Button->Position.y;
    f32 X1 = Button->Position.x + ButtonSize.x;
    f32 Y1 = Button->Position.y + ButtonSize.y;

    return ((Position.x >= X0) &&
            (Position.x <= X1) &&
            (Position.y >= Y0) &&
            (Position.y <= Y1));
}

internal b32 PositionIsInsideSliderHandle(Vector2 Position, slider *Slider)
{
    f32 X0 = Slider->Position.x;
    f32 Y0 = Slider->Position.y;
    f32 X1 = Slider->Position.x + Slider->Size.x;
    f32 Y1 = Slider->Position.y + Slider->Size.y;

    return ((Position.x >= X0) &&
            (Position.x <= X1) &&
            (Position.y >= Y0) &&
            (Position.y <= Y1));
}

b32 DoButton(ui *UI, button *Button, s32 FontSize)
{
    b32 ButtonPressed = 0;
    Color UnderColor = (Color){20,20,20,225};
    Color InactiveColor = (Color){20,180,180,255};
    Color HotColor = (Color){40,120,120,255};
    Color ActiveColor = (Color){40,120,120,255};
    Color TextColor = (Color){0,0,0,255};

    s32 TextWidth = MeasureText((char *)Button->Text, FontSize);
    f32 TwicePadding = 2 * BUTTON_PADDING;
    Vector2 ButtonSize = CreateVector2(TextWidth + TwicePadding, FontSize + TwicePadding);

    b32 IsHot = PositionIsInsideButton(UI->MousePosition, Button, ButtonSize);
    b32 IsActive = UI->Active == Button->Id;

    if (IsHot)
    {
        UI->Hot = Button->Id;
    }

    if (UI->MouseButtonPressed)
    {
        if (IsHot)
        {
            if (!UI->Active)
            {
                UI->Active = Button->Id;
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

    Rectangle Rect = (Rectangle){Button->Position.x, Button->Position.y,
                                 ButtonSize.x, ButtonSize.y};
    Rectangle UnderRect = (Rectangle){Button->Position.x + 2.0f, Button->Position.y + 2.0f,
                                      ButtonSize.x, ButtonSize.y};

    Color ButtonColor = IsActive ? ActiveColor : InactiveColor;

    DrawRectangleLinesEx(UnderRect, 2.0f, UnderColor);
    DrawRectangle(Rect.x, Rect.y, Rect.width, Rect.height, ButtonColor);

    if (IsHot && (IsActive || !UI->Active))
    {
        DrawRectangleLinesEx(Rect, 2.0f, HotColor);
    }

    DrawText((char *)Button->Text,
             Button->Position.x + BUTTON_PADDING, Button->Position.y + BUTTON_PADDING,
             UI->FontSize, TextColor);

    return ButtonPressed;
}

b32 DoSlider(ui *UI, slider *Slider)
{
    b32 Changed = 0;

    Vector2 Position = Slider->Position;
    Vector2 Size = Slider->Size;
    f32 CenterY = Position.y + (Size.y / 2.0f);

    Color SliderColor = (Color){50,50,50,255};
    Color HandleColor = (Color){230,210,150,255};

    f32 TrackWidthPercent = 0.1f;
    f32 TrackHeight = Size.y * TrackWidthPercent;
    f32 TrackOffset = TrackHeight / 2.0f;

    f32 HandleWidth = 16.0f;

    b32 IsHot = PositionIsInsideSliderHandle(UI->MousePosition, Slider);
    b32 IsActive = UI->Active == Slider->Id;

    if (UI->MouseButtonPressed)
    {
        if (IsHot)
        {
            if (!UI->Active)
            {
                UI->Active = Slider->Id;
            }
        }
    }
    else if (UI->MouseButtonReleased)
    {
        if (IsActive)
        {
            UI->Active = 0;
        }
    }

    if (IsActive)
    {
        Slider->Value = (UI->MousePosition.x - Slider->Position.x) / Slider->Size.x;
        Slider->Value = ClampF32(Slider->Value, 0.0f, 1.0f);
        Changed = 1;
    }

    f32 HandleX = (Slider->Value * Slider->Size.x) + Slider->Position.x - (HandleWidth / 2.0f);

    DrawRectangle(Position.x, CenterY - TrackOffset, Size.x, TrackHeight, SliderColor);
    DrawRectangle(HandleX, Position.y, HandleWidth, Size.y, HandleColor);

    return Changed;
}
