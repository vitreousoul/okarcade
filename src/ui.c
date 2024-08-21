#define BUTTON_PADDING 8

#define IS_SPACE(c) ((c) == ' ' || (c) == '\t' || (c) == '\n' || (c) == '\r')

#define Is_Utf8_Header_Byte1(b) (((b) & 0x80) == 0x00)
#define Is_Utf8_Header_Byte2(b) (((b) & 0xE0) == 0xC0)
#define Is_Utf8_Header_Byte3(b) (((b) & 0xF0) == 0xE0)
#define Is_Utf8_Header_Byte4(b) (((b) & 0xF8) == 0xF0)
#define Is_Utf8_Tail_Byte(b)    (((b) & 0xC0) == 0x80)

typedef s32 ui_id;

typedef enum
{
    ui_element_type_UNDEFINED,
    ui_element_type_Button,
    ui_element_type_Slider,
    ui_element_type_Tablet,
    ui_element_type_Count,
    ui_element_type_Text,
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
    b32 Shift;
    b32 Control;
    b32 Alt;
    b32 Super;
} modifier_keys;

typedef enum
{
    accent_NULL,
    accent_Acute,
    accent_Tilde,
} accent;

typedef struct
{
    ui_id Id;

    Vector2 Position;
    alignment Alignment;

    Vector2 Size;
    u8 *Text;
} button_element;

typedef struct
{
    ui_id Id;

    Vector2 Position;
    Vector2 Size;
    f32 Value;
} slider_element;

typedef struct
{
    ui_id Id;

    Vector2 Position;
    Vector2 Size;
    Vector2 Offset;

    b32 MouseReleased;
} tablet_element;

typedef struct
{
    ui_id Id;

    u8 *Text;
    s32 TextSize;
    /* TODO: Turn booleans into flags. */
    b32 AccentMode;
    b32 KeyHasRepeated;

    Vector2 Position;
    Vector2 Size;

    s32 Index;
    f32 KeyRepeatTime;

    accent CurrentAccent;

    Color Color;
} text_element;

typedef struct
{
    ui_element_type Type;
    union
    {
        button_element Button;
        slider_element Slider;
        tablet_element Tablet;
    };
} ui_element;

typedef struct
{
    ui_id Active;
    ui_id Hot;

    Font Font;
    s32 FontSize;

    b32 MouseButtonPressed;
    b32 MouseButtonReleased;
    Vector2 MousePosition;

    modifier_keys ModifierKeys;
    b32 EnterPressed;

    Vector2 ActivationPosition;

    f32 CursorBlinkRate;
    f32 CursorBlinkTime;

    /* TODO: Dynamically allocate the line buffer. */
#define Max_Line_Count 6
#define Max_Line_Buffer_Size 256
    u8 LineBuffer[Max_Line_Count][Max_Line_Buffer_Size];
    s32 LineBufferIndex;
} ui;

ui_id GetElementId(ui_element Element);

button_element CreateButton(Vector2 Position, u8 *Text, s32 Id);

void DrawWrappedText(ui *Ui, u8 *Text, f32 MaxWidth, f32 X, f32 Y, f32 LineHeight, f32 LetterSpacing, Color FontColor, alignment Alignment);

b32 DoButton(ui *UI, button_element *Button);
b32 DoButtonWith(ui *UI, s32 Id, u8 *Text, Vector2 Position, alignment Alignment);
b32 DoSlider(ui *UI, slider_element *Slider);
b32 DoTablet(ui *UI, tablet_element *Tablet);
b32 DoUiElement(ui *UI, ui_element *UiElement);


ui_id GetElementId(ui_element Element)
{
    ui_id Result;

    switch (Element.Type)
    {
    case ui_element_type_Button:
    {
        Result = Element.Button.Id;
    } break;
    case ui_element_type_Slider:
    {
        Result = Element.Slider.Id;
    } break;
    case ui_element_type_Tablet:
    {
        Result = Element.Tablet.Id;
    } break;
    default:
    {
        Result = 0;
    } break;
    }

    return Result;
}

button_element CreateButton(Vector2 Position, u8 *Text, s32 Id)
{
    button_element Button;

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

internal b32 PositionIsInsideSliderHandle(Vector2 Position, slider_element *Slider)
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

internal b32 PositionIsInsideTablet(Vector2 Position, tablet_element *Tablet)
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

internal f32 GetCenteredTextX(ui *Ui, u8 *Text, s32 LetterSpacing)
{
    Vector2 TextSize = MeasureTextEx(Ui->Font, (char *)Text, Ui->FontSize, LetterSpacing);
    f32 X = (SCREEN_WIDTH - TextSize.x) / 2.0f;

    return X;
}

void DrawWrappedText(ui *Ui, u8 *Text, f32 MaxWidth, f32 X, f32 Y, f32 LineHeight, f32 LetterSpacing, Color FontColor, alignment Alignment)
{
    s32 I = 0;
    s32 J = 0;
    s32 Offset = 0;
    Ui->LineBufferIndex = 0;

    /* TODO: @Speed We should be able do better than a linear search */
    while (Text[I] && Ui->LineBufferIndex < Max_Line_Count)
    {
        J = I - Offset;
        Assert(J >= 0 && J < Max_Line_Buffer_Size);

        u8 *Line = Ui->LineBuffer[Ui->LineBufferIndex];

        Line[J] = Text[I];
        Line[J + 1] = 0;
        Assert(J > 0 || !IS_SPACE(Line[J]));
        Vector2 LineSize = MeasureTextEx(Ui->Font, (char *)Line, Ui->FontSize, LetterSpacing);

        if (LineSize.x > MaxWidth)
        {
            b32 SpaceFound = 0;

            /* NOTE: Seek back to the last space, and break the line there */
            while (J >= 0)
            {
                if (IS_SPACE(Line[J]))
                {
                    SpaceFound = 1;
                    Line[J] = 0;
                    Offset += J + 1;
                    I = Offset;

                    ++Ui->LineBufferIndex;

                    break;
                }
                else
                {
                    while (Is_Utf8_Tail_Byte(Line[J--]));
                }
            }

            /* NOTE: No space was found, which means we have a word that is longer than
               the maximum line width. Break the word and put a dash at the end of the line.
            */
            if (!SpaceFound)
            {
                s32 LastCharIndex = I - Offset;
                Line[LastCharIndex] = '-';

                Offset = I;
            }

            Assert(Offset == I);
        }
        else
        {
            ++I;
        }
    }

    f32 Height = LineHeight * Ui->LineBufferIndex;
    Rectangle AlignedRectangle = GetAlignedRectangle(V2(X, Y), V2(MaxWidth, Height), Alignment);

    f32 OffsetY = 0.0f;

    /* NOTE: Draw the accumulated lines */
    for (s32 LineIndex = 0; LineIndex < Ui->LineBufferIndex; ++LineIndex)
    {
        u8 *Line = Ui->LineBuffer[LineIndex];
        Vector2 LineSize = MeasureTextEx(Ui->Font, (const char *)Line, Ui->FontSize, LetterSpacing);
        /* TODO: This is a hack to make centered alignment also center text. Really we want _two_ alignments: rectangular and text. */
        /* @CopyPasta ------v */
        f32 OffsetX = 0.0f;
        if (Alignment == alignment_TopCenter ||
            Alignment == alignment_CenterCenter ||
            Alignment == alignment_BottomCenter)
        {
            OffsetX = 0.5f * (MaxWidth - LineSize.x);
        }

        DrawTextEx(Ui->Font, (char *)Line,
                   V2(OffsetX + AlignedRectangle.x, AlignedRectangle.y + OffsetY),
                   Ui->FontSize, LetterSpacing, FontColor);
        OffsetY += LineHeight;
    }

    /* NOTE: Draw any text remaining after the last line. */
    if (I > Offset)
    {
        u8 *RemainingText = Text + Offset;
        Vector2 LineSize = MeasureTextEx(Ui->Font, (const char *)RemainingText, Ui->FontSize, LetterSpacing);

        /* @CopyPasta ------^ */
        f32 OffsetX = 0.0f;
        if (Alignment == alignment_TopCenter ||
            Alignment == alignment_CenterCenter ||
            Alignment == alignment_BottomCenter)
        {
            OffsetX = 0.5f * (MaxWidth - LineSize.x);
        }

        DrawTextEx(Ui->Font, (char *)RemainingText,
                   V2(OffsetX + AlignedRectangle.x, AlignedRectangle.y + OffsetY),
                   Ui->FontSize, LetterSpacing, FontColor);
    }
}

b32 DoButton(ui *UI, button_element *Button)
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

    b32 IsMouseOver = PositionIsInsideRect(UI->MousePosition, AlignedRect);
    b32 IsHot = UI->Hot == Button->Id;
    b32 IsActive = UI->Active == Button->Id;

    if (IsMouseOver)
    {
        UI->Hot = Button->Id;
    }
    else if (UI->Hot == Button->Id)
    {
        UI->Hot = 0;
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
    else if (UI->EnterPressed)
    {
        if (IsHot)
        {
            ButtonPressed = 1;
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
    button_element Button;

    Button.Id = Id;
    Button.Position = Position;
    Button.Alignment = Alignment;
    Button.Size = V2(0.0f, 0.0f);
    Button.Text = Text;

    b32 Pressed = DoButton(UI, &Button);

    return Pressed;
}

b32 DoSlider(ui *UI, slider_element *Slider)
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

b32 DoTablet(ui *UI, tablet_element *Tablet)
{
    b32 Changed = 0;

    b32 IsHot = PositionIsInsideTablet(UI->MousePosition, Tablet);
    b32 NoActive = !UI->Active;
    b32 IsActive = UI->Active == Tablet->Id;

    Tablet->MouseReleased = 0;

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

            Tablet->MouseReleased = 1;
            /* Tablet->Offset.x = UI->ActivationPosition.x + Tablet->Offset.x; */
            /* Tablet->Offset.y = UI->ActivationPosition.y + Tablet->Offset.y; */
        }
    }

    return Changed;
}

/* TODO: Probably don't pass DeltaTime in, but have it live as a member of some arguemnt struct.
   Or handle cursor blink timing outside of DoText? */
u8 *DoText(ui *UI, text_element *TextElement, f32 DeltaTime)
{
    u8 *Result = (u8 *)"";
    char *Text = (char *)TextElement->Text;
    int LetterSpacing = 1;

    if (Text)
    {
        f32 BlinkTime = UI->CursorBlinkTime;
        f32 BlinkRate = UI->CursorBlinkRate;
        b32 ShowCursor = BlinkTime < 0.6f * BlinkRate;
        Vector2 TextSize = MeasureTextEx(UI->Font, Text, UI->FontSize, 1);

        f32 InputX = TextElement->Position.x - (TextSize.x / 2.0f);
        f32 InputY = TextElement->Position.y;

        f32 CursorX = (SCREEN_WIDTH + TextSize.x) / 2.0f;
        f32 CursorY = InputY;

        DrawTextEx(UI->Font, Text, V2(InputX, InputY), UI->FontSize, LetterSpacing, TextElement->Color);

        if (BlinkRate > 0.0f)
        {
            BlinkTime = BlinkTime + DeltaTime;
            while (BlinkTime > BlinkRate)
            {
                BlinkTime -= BlinkRate;
            }
            UI->CursorBlinkTime = BlinkTime;

            if (ShowCursor)
            {
                Color CursorColor = (Color){130,100,250,255};
                f32 Spacing = 3.0f;
                DrawRectangle(CursorX + Spacing, CursorY, 3, UI->FontSize, CursorColor);
            }
        }
    }

    return Result;
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
