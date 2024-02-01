#define BUTTON_PADDING 8

typedef enum
{
    ui_element_type_UNDEFINED,
    ui_element_type_Button,
    ui_element_type_Slider,
    ui_element_type_Tablet,
    ui_element_type_Count,
} ui_element_type;

typedef enum
{
   alignment_TopLeft,
   alignment_TopCenter,
   alignment_TopRight,
   alignment_CenterRight,
   alignment_BottomRight,
   alignment_BottomCenter,
   alignment_BottomLeft,
   alignment_CenterLeft,
   alignment_CenterCenter,
} alignment;

typedef struct
{
    s32 Id;

    Vector2 Position;
    alignment Alignment;

    Vector2 Size;
    u8 *Text;
} button;

typedef struct
{
    s32 Id;

    Vector2 Position;
    Vector2 Size;
    f32 Value;
} slider;

typedef struct
{
    s32 Id;

    Vector2 Position;
    Vector2 Size;
    Vector2 Offset;

    b32 UpdateTurtlePosition; /* TODO: Replace UpdateTurtlePosition with a more generic name for "mouse realeased" */
} tablet;

typedef struct
{
    ui_element_type Type;
    union
    {
        button Button;
        slider Slider;
        tablet Tablet;
    };
} ui_element;

typedef struct
{
    s32 Active;
    s32 Hot;


    Font Font;
    s32 FontSize;

    b32 MouseButtonPressed;
    b32 MouseButtonReleased;
    Vector2 MousePosition;

    Vector2 ActivationPosition;
} ui;

button CreateButton(Vector2 Position, u8 *Text, s32 Id);

b32 DoButton(ui *UI, button *Button);
b32 DoButtonWith(ui *UI, s32 Id, u8 *Text, Vector2 Position, alignment Alignment);
b32 DoSlider(ui *UI, slider *Slider);
b32 DoTablet(ui *UI, tablet *Tablet);
b32 DoUiElement(ui *UI, ui_element *UiElement);

button CreateButton(Vector2 Position, u8 *Text, s32 Id)
{
    button Button;

    Button.Position = Position;
    Button.Text = Text;
    Button.Id = Id;

    return Button;
}

internal b32 PositionIsInsideRect(Vector2 Position, Rectangle Rect)
{
    f32 x0 = Rect.x;
    f32 y0 = Rect.y;
    f32 x1 = x0 + Rect.width;
    f32 y1 = y0 + Rect.height;

    return ((Position.x >= x0) &&
            (Position.x <= x1) &&
            (Position.y >= y0) &&
            (Position.y <= y1));
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

internal b32 PositionIsInsideTablet(Vector2 Position, tablet *Tablet)
{
    f32 X0 = Tablet->Position.x;
    f32 Y0 = Tablet->Position.y;
    f32 X1 = Tablet->Position.x + Tablet->Size.x;
    f32 Y1 = Tablet->Position.y + Tablet->Size.y;

    return ((Position.x >= X0) &&
            (Position.x <= X1) &&
            (Position.y >= Y0) &&
            (Position.y <= Y1));
}

internal Rectangle GetAlignedRectangle(Vector2 Position, Vector2 Size, alignment Alignment)
{
    Rectangle Result;
    Vector2 HalfSize = V2(Size.x / 2.0f, Size.y / 2.0f);
    Vector2 Offset = V2(0.0f, 0.0f);

    switch(Alignment)
    {
    case alignment_TopLeft: Offset = V2(0.0f, 0.0f); break;
    case alignment_TopCenter: Offset = V2(-HalfSize.x, 0.0f); break;
    case alignment_TopRight: Offset = V2(-Size.x, 0.0f); break;
    case alignment_CenterRight: Offset = V2(-Size.x, -HalfSize.y); break;
    case alignment_BottomRight: Offset = V2(-Size.x, -Size.y); break;
    case alignment_BottomCenter: Offset = V2(-HalfSize.x, -Size.y); break;
    case alignment_BottomLeft: Offset = V2(0.0f, -Size.y); break;
    case alignment_CenterLeft: Offset = V2(0.0f, -HalfSize.y); break;
    case alignment_CenterCenter:
    default: Offset = V2(-HalfSize.x, -HalfSize.y);
    }

    Result = R2(Position.x + Offset.x, Position.y + Offset.y, Size.x, Size.y);

    return Result;
}

b32 DoButton(ui *UI, button *Button)
{
    b32 ButtonPressed = 0;
    Color UnderColor = (Color){20,20,20,225};
    Color InactiveColor = (Color){20,180,180,255};
    Color HotColor = (Color){40,120,120,255};
    Color ActiveColor = (Color){40,120,120,255};
    Color TextColor = (Color){0,0,0,255};

    f32 FontSpacing = 0;
    Vector2 TextSize = MeasureTextEx(UI->Font, (char *)Button->Text, UI->FontSize, FontSpacing);

    f32 TwicePadding = 2 * BUTTON_PADDING;
    Button->Size = V2(TextSize.x + TwicePadding, UI->FontSize + TwicePadding);

    Rectangle AlignedRect = GetAlignedRectangle(Button->Position, Button->Size, Button->Alignment);

    b32 IsHot = PositionIsInsideRect(UI->MousePosition, AlignedRect);
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

    Rectangle Rect = (Rectangle){AlignedRect.x, AlignedRect.y,
                                 AlignedRect.width, AlignedRect.height};
    Rectangle UnderRect = (Rectangle){AlignedRect.x + 2.0f, AlignedRect.y + 2.0f,
                                      AlignedRect.width, AlignedRect.height};

    Color ButtonColor = IsActive ? ActiveColor : InactiveColor;

    DrawRectangleLinesEx(UnderRect, 2.0f, UnderColor);
    DrawRectangle(Rect.x, Rect.y, Rect.width, Rect.height, ButtonColor);

    if (IsHot && (IsActive || !UI->Active))
    {
        DrawRectangleLinesEx(Rect, 2.0f, HotColor);
    }

    Vector2 TextPosition = V2(AlignedRect.x + BUTTON_PADDING, AlignedRect.y + BUTTON_PADDING);

    DrawTextEx(UI->Font, (char *)Button->Text, TextPosition, UI->FontSize, FontSpacing, TextColor);

    return ButtonPressed;
}

b32 DoButtonWith(ui *UI, s32 Id, u8 *Text, Vector2 Position, alignment Alignment)
{
    button Button;

    Button.Id = Id;
    Button.Position = Position;
    Button.Alignment = Alignment;
    Button.Size = V2(0.0f, 0.0f);
    Button.Text = Text;

    b32 Pressed = DoButton(UI, &Button);

    return Pressed;
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

b32 DoTablet(ui *UI, tablet *Tablet)
{
    b32 Changed = 0;

    b32 IsHot = PositionIsInsideTablet(UI->MousePosition, Tablet);
    b32 NoActive = !UI->Active;
    b32 IsActive = UI->Active == Tablet->Id;

    Tablet->UpdateTurtlePosition = 0;

    if (IsHot)
    {
        if (UI->MouseButtonPressed && NoActive)
        {
            UI->Active = Tablet->Id;
            UI->ActivationPosition = UI->MousePosition;
        }

        if (IsActive)
        {
            Changed = 1;

            Tablet->Offset.x = UI->MousePosition.x - UI->ActivationPosition.x;
            Tablet->Offset.y = UI->MousePosition.y - UI->ActivationPosition.y;
        }
    }

    if (UI->MouseButtonReleased)
    {
        if (IsActive)
        {
            IsActive = 0;
            UI->Active = 0;
            Changed = 1;

            Tablet->UpdateTurtlePosition = 1;
            /* Tablet->Offset.x = UI->ActivationPosition.x + Tablet->Offset.x; */
            /* Tablet->Offset.y = UI->ActivationPosition.y + Tablet->Offset.y; */
        }
    }

    return Changed;
}

b32 DoUiElement(ui *UI, ui_element *UiElement)
{
    b32 Changed = 0;

    switch (UiElement->Type)
    {
    case ui_element_type_Button:
        Changed = DoButton(UI, &UiElement->Button);
        break;
    case ui_element_type_Slider:
        Changed = DoSlider(UI, &UiElement->Slider);
        break;
    case ui_element_type_Tablet:
        Changed = DoTablet(UI, &UiElement->Tablet);
        break;
    default: break;
    }

    return Changed;
}
