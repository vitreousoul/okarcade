#define BUTTON_PADDING 8

#define IS_SPACE(c) ((c) == ' ' || (c) == '\t' || (c) == '\n' || (c) == '\r')
#define IS_UPPER_CASE(c) ((c) >= 65 && (c) <= 90)

#define Is_Utf8_Header_Byte1(b) (((b) & 0x80) == 0x00)
#define Is_Utf8_Header_Byte2(b) (((b) & 0xE0) == 0xC0)
#define Is_Utf8_Header_Byte3(b) (((b) & 0xF0) == 0xE0)
#define Is_Utf8_Header_Byte4(b) (((b) & 0xF8) == 0xF0)
#define Is_Utf8_Tail_Byte(b)    (((b) & 0xC0) == 0x80)


#define Total_Number_Of_Keys_Log2 9
#define Total_Number_Of_Keys (1 << Total_Number_Of_Keys_Log2)

#define Key_State_Chunk_Size_Log2 5
#define Key_State_Chunk_Size (1 << Key_State_Chunk_Size_Log2)
#define Key_State_Chunk_Size_Mask (Key_State_Chunk_Size - 1)

#define Key_State_Chunk_Count_Log2 5
#define Key_State_Chunk_Count_Mask ((1 << Key_State_Chunk_Count_Log2) - 1)

#define Bits_Per_Key_State_Log2 1
#define Bits_Per_Key_State (1 << Bits_Per_Key_State_Log2)
#define Bits_Per_Key_State_Mask ((1 << Bits_Per_Key_State) - 1)

#define Bits_Per_Key_State_Chunk (sizeof(key_state_chunk) * 8)
#define Key_State_Chunk_Count (Total_Number_Of_Keys / (Bits_Per_Key_State_Chunk / Bits_Per_Key_State))

#define Get_Flag(  flags, flag) (((flags) &  (flag)) != 0)
#define Set_Flag(  flags, flag)  ((flags) = (flags) | flag)
#define Unset_Flag(flags, flag)  ((flags) = (flags) & ~(flag))

#define Assign_Flag(flags, flag, value)\
    ((value) ? Set_Flag(flags, flag) : Unset_Flag(flags, flag))

#define Toggle_Flag(flags, flag)\
    (Get_Flag((flags), (flag)) ? Unset_Flag((flags), (flag)) : Set_Flag((flags), (flag)))

global_variable b32 PrintableKeys[Total_Number_Of_Keys] = {
    [KEY_SPACE] = 1,
    [KEY_APOSTROPHE] = 1,
    [KEY_COMMA] = 1,
    [KEY_MINUS] = 1,
    [KEY_PERIOD] = 1,
    [KEY_SLASH] = 1,
    [KEY_ZERO] = 1,
    [KEY_ONE] = 1,
    [KEY_TWO] = 1,
    [KEY_THREE] = 1,
    [KEY_FOUR] = 1,
    [KEY_FIVE] = 1,
    [KEY_SIX] = 1,
    [KEY_SEVEN] = 1,
    [KEY_EIGHT] = 1,
    [KEY_NINE] = 1,
    [KEY_SEMICOLON] = 1,
    [KEY_EQUAL] = 1,
    [KEY_A] = 1,
    [KEY_B] = 1,
    [KEY_C] = 1,
    [KEY_D] = 1,
    [KEY_E] = 1,
    [KEY_F] = 1,
    [KEY_G] = 1,
    [KEY_H] = 1,
    [KEY_I] = 1,
    [KEY_J] = 1,
    [KEY_K] = 1,
    [KEY_L] = 1,
    [KEY_M] = 1,
    [KEY_N] = 1,
    [KEY_O] = 1,
    [KEY_P] = 1,
    [KEY_Q] = 1,
    [KEY_R] = 1,
    [KEY_S] = 1,
    [KEY_T] = 1,
    [KEY_U] = 1,
    [KEY_V] = 1,
    [KEY_W] = 1,
    [KEY_X] = 1,
    [KEY_Y] = 1,
    [KEY_Z] = 1,
    [KEY_LEFT_BRACKET] = 1,
    [KEY_BACKSLASH] = 1,
    [KEY_RIGHT_BRACKET] = 1,
    [KEY_GRAVE] = 1,
};

typedef enum
{
    key_Up,
    key_Pressed,
    key_Down,
    key_Released,
    key_state_Count,
    key_state_Undefined,
} key_state;

global_variable key_state KeyStateMap[key_state_Count][2] = {
    [key_Up]       =  {key_Up      ,  key_Pressed},
    [key_Pressed]  =  {key_Released,  key_Down},
    [key_Down]     =  {key_Released,  key_Down},
    [key_Released] =  {key_Up      ,  key_Pressed},
};


typedef s32 ui_id;
typedef u32 key_code;

typedef struct
{
    s16 Index;
    s16 Shift;
} key_location;

typedef enum
{
    ui_element_type_UNDEFINED,

    ui_element_type_Button,
    ui_element_type_Slider,
    ui_element_type_Tablet,
    ui_element_type_Text,

    ui_element_type__Count,
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

typedef enum
{
    modifier_key_Shift    = 0x1,
    modifier_key_Control  = 0x2,
    modifier_key_Alt      = 0x4,
    modifier_key_Super    = 0x8,
} modifier_key_type;

typedef u32 modifier_flags;

typedef enum
{
    accent_NULL,
    accent_Acute,
    accent_Tilde,
} accent;

typedef enum
{
    ui_element_flag_Clickable  = (1<<0),
    ui_element_flag_SlidableX  = (1<<1),
    ui_element_flag_SlidableY  = (1<<2),
    ui_element_flag_AccentMode = (1<<3),
    ui_element_flag_HasText    = (1<<4), /* TODO: Maybe we need two distinct flags for single-line and multi-line text. Or maybe single-line text is multi-line text with infinite width? */
    ui_element_flag_Rectangle  = (1<<5),
    ui_element_flag_Outline    = (1<<6),
    ui_element_flag_Shadow     = (1<<7),

    ui_element_flag_SlidableXY = ui_element_flag_SlidableX | ui_element_flag_SlidableY,
} ui_element_flag;

typedef struct
{
    ui_element_type Type;
    ui_id Id;
    Vector2 Position;
    alignment Alignment;
    Vector2 Size;
    u8 *Text; /* TODO: Use a string type. */
    s32 TextSize;
    f32 Value;
    Vector2 Offset;
    b32 AccentMode; /* TODO: Turn this into a flag. */
    s32 Index;
    accent CurrentAccent;
    Color Color;
    ui_element_flag Flags;
} ui_element;

typedef u32 key_state_chunk;

typedef struct
{
    key_code Key;
    u32 Repeat;
} key_event;

typedef struct
{
    ui_id Active;
    ui_id Hot;

    Font Font;
    s32 FontSize;

    b32 MouseButtonPressed;
    b32 MouseButtonReleased;
    Vector2 MousePosition;

    #define Key_Press_Queue_Size 32
    key_event KeyEventQueue[Key_Press_Queue_Size];
    s32 QueueIndex;
    modifier_flags ModifierFlags;
    f32 KeyRepeatTime;
    b32 KeyHasRepeated;
    key_code LastKeyPressed;
    b32 InputOccured;
    u32 KeyStateIndex;
    key_state_chunk KeyStates[2][Key_State_Chunk_Count];

    Vector2 ActivationPosition;

    f32 CursorBlinkRate;
    f32 CursorBlinkTime;
    f32 DeltaTime;

    /* TODO: Dynamically allocate the line buffer. */
#define Max_Line_Count 6
#define Max_Line_Buffer_Size 256
    u8 LineBuffer[Max_Line_Count][Max_Line_Buffer_Size];
    s32 LineBufferIndex;
} ui;

ui_id GetElementId(ui_element Element)
{
    ui_id Result;

    switch (Element.Type)
    {
    case ui_element_type_Button:
    case ui_element_type_Slider:
    case ui_element_type_Tablet:
    {
        Result = Element.Id;
    } break;
    default:
    {
        Result = 0;
    } break;
    }

    return Result;
}

ui_element CreateButton(s32 Id, u8 *Text, Vector2 Position, alignment Alignment)
{
    ui_element Element = {};

    Element.Position = Position;
    Element.Text = Text;
    Element.Id = Id;
    Element.Alignment = Alignment;

    return Element;
}

ui_element CreateText(Vector2 Position, s32 Id, u8 *Text, s32 TextSize)
{
    ui_element Element = {};

    Element.Id = Id;
    Element.Text = Text;
    Element.TextSize = TextSize;
    Element.Position = Position;
    Element.Color = (Color){255, 255, 255, 255};

    return Element;
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

/* TODO: GetCenteredTextX relies on the Screen_Width, which doesn't feel good, we should remove this function and have calling code offset the element's x to the center and use a center-x alignment. */
internal f32 GetCenteredTextX(ui *Ui, u8 *Text, s32 LetterSpacing)
{
    Vector2 TextSize = MeasureTextEx(Ui->Font, (char *)Text, Ui->FontSize, LetterSpacing);
    f32 X = (Screen_Width - TextSize.x) / 2.0f; /* TODO: Don't rely on Screen_Width being defined! Find some other way to get the screen-width to this function. */

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

b32 DoUiElement(ui *Ui, ui_element *UiElement)
{
    b32 Changed = 0;

    f32 LetterSpacing = 1.7f; /* TODO: Don't hardcode this. */

    if (Get_Flag(UiElement->Flags, ui_element_flag_HasText))
    {
        UiElement->Size = MeasureTextEx(Ui->Font, (char *)UiElement->Text, Ui->FontSize, LetterSpacing);
    }

    if (UiElement->Type == ui_element_type_Button)
    {
        UiElement->Size = AddV2S(UiElement->Size, 2*BUTTON_PADDING);
    }

    Rectangle AlignedRect = GetAlignedRectangle(UiElement->Position, UiElement->Size, UiElement->Alignment);

    b32 IsMouseOver = PositionIsInsideRect(Ui->MousePosition, AlignedRect);
    b32 NoActive = !Ui->Active;
    b32 IsActive = Ui->Active == UiElement->Id;
    if ((NoActive || IsActive) && IsMouseOver)
    {
        Ui->Hot = UiElement->Id;
    }
    b32 IsHot = Ui->Hot == UiElement->Id;

    if (Get_Flag(UiElement->Flags, ui_element_flag_Clickable))
    {
        if (Ui->MouseButtonPressed && IsHot & !Ui->Active)
        {
            Ui->Active = UiElement->Id;
            Ui->ActivationPosition = Ui->MousePosition;
        }
        else if (Ui->MouseButtonReleased)
        {
            if (IsActive)
            {
                IsActive = 0;
                Ui->Active = 0;

                if (IsHot) /* TODO: We currently trigger changes on mouse release, but it would be better to default setting Changed to 1 on mouse down. */
                {
                    Changed = 1;
                }
            }
        }
    }

    if (Get_Flag(UiElement->Flags, ui_element_flag_SlidableX))
    {
        if (IsActive)
        {
            UiElement->Offset.x = Ui->MousePosition.x - Ui->ActivationPosition.x;
            Changed = 1;
        }
    }

    if (Get_Flag(UiElement->Flags, ui_element_flag_SlidableY))
    {
        if (IsActive)
        {
            UiElement->Offset.y = Ui->MousePosition.y - Ui->ActivationPosition.y;
            Changed = 1;
        }
    }

    switch (UiElement->Type)
    {
    case ui_element_type_Button:
    {
        Color UnderColor = (Color){20,20,20,225};
        Color InactiveColor = (Color){20,180,180,255};
        Color HotColor = (Color){40,120,120,255};
        Color ActiveColor = (Color){40,120,120,255};
        Color TextColor = (Color){0,0,0,255};


        Rectangle UnderRect = (Rectangle){AlignedRect.x + 2.0f, AlignedRect.y + 2.0f,
                                          AlignedRect.width, AlignedRect.height};

        Color ButtonColor = IsActive ? ActiveColor : InactiveColor;

        DrawRectangleLinesEx(UnderRect, 2.0f, UnderColor);
        DrawRectangle(AlignedRect.x, AlignedRect.y, AlignedRect.width, AlignedRect.height, ButtonColor);

        if (IsHot && (IsActive || !Ui->Active))
        {
            DrawRectangleLinesEx(AlignedRect, 2.0f, HotColor);
        }

        Vector2 TextPosition = V2(AlignedRect.x + BUTTON_PADDING, AlignedRect.y + BUTTON_PADDING);

        DrawTextEx(Ui->Font, (char *)UiElement->Text, TextPosition, Ui->FontSize, LetterSpacing, TextColor);
    } break;
    case ui_element_type_Slider:
    {
        f32 CenterY = UiElement->Position.y + (UiElement->Size.y / 2.0f);

        Color SliderColor = (Color){50,50,50,255};
        Color HandleColor = (Color){230,210,150,255};

        f32 TrackWidthPercent = 0.1f;
        f32 TrackHeight = UiElement->Size.y * TrackWidthPercent;
        f32 TrackOffset = TrackHeight / 2.0f;

        f32 HandleWidth = 16.0f;

        if (IsActive)
        {
            UiElement->Value = (Ui->MousePosition.x - UiElement->Position.x) / UiElement->Size.x;
            UiElement->Value = ClampF32(UiElement->Value, 0.0f, 1.0f);
            Changed = 1;
        }

        f32 HandleX = (UiElement->Value * UiElement->Size.x) + UiElement->Position.x - (HandleWidth / 2.0f);

        DrawRectangle(UiElement->Position.x, CenterY - TrackOffset, UiElement->Size.x, TrackHeight, SliderColor);
        DrawRectangle(HandleX, UiElement->Position.y, HandleWidth, UiElement->Size.y, HandleColor);
    } break;
    case ui_element_type_Tablet:
    {
        /* NOTE: The functionality for tablet disappeared into slidable code above... */
    } break;
    case ui_element_type_Text:
    {
        if (UiElement->Text)
        {
            f32 BlinkTime = Ui->CursorBlinkTime;
            f32 BlinkRate = Ui->CursorBlinkRate;
            b32 ShowCursor = BlinkTime < 0.6f * BlinkRate;
            Rectangle AlignedRectangle = GetAlignedRectangle(UiElement->Position, UiElement->Size, UiElement->Alignment);

            f32 InputX = AlignedRectangle.x;
            f32 InputY = UiElement->Position.y;

            f32 CursorX = AlignedRectangle.x + UiElement->Size.x;
            f32 CursorY = InputY;

            DrawTextEx(Ui->Font, (char *)UiElement->Text, V2(InputX, InputY), Ui->FontSize, LetterSpacing, UiElement->Color);

            if (BlinkRate > 0.0f)
            {
                BlinkTime = BlinkTime + Ui->DeltaTime;
                while (BlinkTime > BlinkRate)
                {
                    BlinkTime -= BlinkRate;
                }
                Ui->CursorBlinkTime = BlinkTime;

                if (ShowCursor)
                {
                    Color CursorColor = (Color){130,100,250,255};
                    f32 Spacing = 3.0f;
                    DrawRectangle(CursorX + Spacing, CursorY, 3, Ui->FontSize, CursorColor);
                }
            }
        }
    } break;
    default: Assert(!"Unhandled ui element type."); break;
    }

    return Changed;
}

b32 DoButton(ui *Ui, ui_element *Button)
{
    Button->Type = ui_element_type_Button;
    Set_Flag(Button->Flags, ui_element_flag_HasText | ui_element_flag_Clickable);
    return DoUiElement(Ui, Button);
}

b32 DoButtonWith(ui *Ui, s32 Id, u8 *Text, Vector2 Position, alignment Alignment)
{
    ui_element Button;

    Button.Id = Id;
    Button.Position = Position;
    Button.Alignment = Alignment;
    Button.Size = V2(0.0f, 0.0f);
    Button.Text = Text;
    Set_Flag(Button.Flags, ui_element_flag_HasText | ui_element_flag_Clickable);

    b32 Pressed = DoButton(Ui, &Button);

    return Pressed;
}

b32 DoSlider(ui *Ui, ui_element *Slider)
{
    Slider->Type = ui_element_type_Slider;
    Set_Flag(Slider->Flags, ui_element_flag_Clickable | ui_element_flag_SlidableX);
    return DoUiElement(Ui, Slider);
}

b32 DoTablet(ui *Ui, ui_element *Tablet)
{
    Tablet->Type = ui_element_type_Tablet;
    Set_Flag(Tablet->Flags, ui_element_flag_Clickable);
    return DoUiElement(Ui, Tablet);
}

void DoTextElement(ui *Ui, ui_element *TextElement, alignment Alignment)
{
    TextElement->Type = ui_element_type_Text;
    TextElement->Alignment = Alignment;
    Set_Flag(TextElement->Flags, ui_element_flag_HasText);
    DoUiElement(Ui, TextElement);
}

internal inline key_location GetKeyLocation(u32 KeyNumber)
{
     key_location Location;

     Location.Index = Key_State_Chunk_Count_Mask & (KeyNumber >> (Key_State_Chunk_Size_Log2 - Bits_Per_Key_State_Log2));
     Location.Shift = Key_State_Chunk_Size_Mask & (KeyNumber << Bits_Per_Key_State_Log2);

    if (Location.Index >= (s32)Key_State_Chunk_Count)
    {
        Location.Index = -1;
        Location.Shift = -1;
    }

    return Location;
}

internal inline b32 KeyLocationIsValid(key_location Location)
{
    return Location.Index >= 0 && Location.Shift >= 0;
}

internal void ClearAccentKey(ui_element *TextElement)
{
    switch (TextElement->CurrentAccent)
    {
    case accent_Acute:
    case accent_Tilde:
    {
        if (TextElement->Text)
        {
            TextElement->Text[TextElement->Index] = 0;
            if (TextElement->Index + 1 < TextElement->TextSize)
            {
                TextElement->Text[TextElement->Index+1] = 0;
            }
            TextElement->AccentMode = 0;
            TextElement->CurrentAccent = 0;
        }
    } break;
    default:
    {
        Assert(!"Not Implemented.");
    }
    }
}

void HandleKey(ui *Ui, ui_element *TextElement, key_code Key)
{
    if (PrintableKeys[Key])
    {
        if (TextElement->Index + 1 < TextElement->TextSize)
        {
            if (TextElement->AccentMode)
            {
                unsigned char SecondByte = 0;

                if (TextElement->Index + 2 < TextElement->TextSize)
                {
                    if (TextElement->CurrentAccent == accent_Acute)
                    {
                        b32 DoNotUpdate = 0;

                        switch (Key)
                        {
                        case KEY_A: SecondByte = 0x81; break;
                        case KEY_E: SecondByte = 0x89; break;
                        case KEY_I: SecondByte = 0x8d; break;
                        case KEY_O: SecondByte = 0x93; break;
                        case KEY_U: SecondByte = 0x9a; break;
                        default: DoNotUpdate = 1; break;
                        }

                        if (DoNotUpdate)
                        {
                            ClearAccentKey(TextElement);
                        }
                        else
                        {
                            if (!Get_Flag(Ui->ModifierFlags, modifier_key_Shift))
                            {
                                SecondByte += 32;
                            }

                            TextElement->Text[TextElement->Index] = 0xC3;
                            TextElement->Text[TextElement->Index+1] = SecondByte;
                            TextElement->Index += 2;
                        }
                    }
                    else if (TextElement->CurrentAccent == accent_Tilde && Key == KEY_N)
                    {
                        char SecondByte = 0x91;

                        if (!Get_Flag(Ui->ModifierFlags, modifier_key_Shift))
                        {
                            SecondByte += 32;
                        }

                        TextElement->Text[TextElement->Index] = 0xC3;
                        TextElement->Text[TextElement->Index+1] = SecondByte;
                        TextElement->Index += 2;
                    }
                    else
                    {
                        ClearAccentKey(TextElement);
                    }
                }

                TextElement->AccentMode = 0;
                TextElement->CurrentAccent = 0;
            }
            else if (Get_Flag(Ui->ModifierFlags, modifier_key_Alt))
            {
                switch (Key)
                {
                case KEY_E:
                {
                    if (TextElement->Index + 2 < TextElement->TextSize)
                    {
                        /* NOTE: Acute accent '´' */
                        TextElement->Text[TextElement->Index] = 0xC2;
                        TextElement->Text[TextElement->Index+1] = 0xB4;
                        TextElement->AccentMode = 1;
                        TextElement->CurrentAccent = accent_Acute;
                    }
                } break;
                case KEY_N:
                {
                    if (TextElement->Index + 2 < TextElement->TextSize)
                    {
                        /* NOTE: Tilde '˜' */
                        /* TextElement->Text[TextElement->Index] = '~'; */
                        TextElement->Text[TextElement->Index] = 0xCB;
                        TextElement->Text[TextElement->Index+1] = 0x9C;
                        TextElement->AccentMode = 1;
                        TextElement->CurrentAccent = accent_Tilde;
                    }
                } break;
                case KEY_SLASH:
                {
                    if (Get_Flag(Ui->ModifierFlags, modifier_key_Shift) &&
                        TextElement->Index + 2 < TextElement->TextSize)
                    {
                        /* NOTE: Inverted question mark '¿' */
                        TextElement->Text[TextElement->Index] = 0xC2;
                        TextElement->Text[TextElement->Index+1] = 0xBF;
                        TextElement->Index = TextElement->Index + 2;
                    }
                } break;
                }
            }
            else if (Get_Flag(Ui->ModifierFlags, modifier_key_Shift) && Key == KEY_SLASH)
            {
                TextElement->Text[TextElement->Index] = '?';
                TextElement->Index = TextElement->Index + 1;
            }
            else
            {
                if (!Get_Flag(Ui->ModifierFlags, modifier_key_Shift) && IS_UPPER_CASE(Key))
                {
                    Key += 32;
                }

                TextElement->Text[TextElement->Index] = Key;
                TextElement->Index = TextElement->Index + 1;
            }
        }
    }
    else
    {
        switch (Key)
        {
        case KEY_BACKSPACE:
        {
            if (TextElement->AccentMode)
            {
                ClearAccentKey(TextElement);
            }
            else
            {
                int TailBytesSkipped = 0;

                for (;;)
                {
                    if (TextElement->Index > 0)
                    {
                        TextElement->Index = TextElement->Index - 1;
                    }

                    if (TextElement->Index < 0)
                    {
                        break;
                    }

                    char Char = TextElement->Text[TextElement->Index];

                    if (Is_Utf8_Tail_Byte(Char))
                    {
                        TextElement->Text[TextElement->Index] = 0;
                        TailBytesSkipped += 1;
                    }
                    else if (TailBytesSkipped == 1 && Is_Utf8_Header_Byte2(Char))
                    {
                        if (TextElement->Index < 0) break;
                        TextElement->Text[TextElement->Index] = 0;
                        break;
                    }
                    else if (TailBytesSkipped == 2 && Is_Utf8_Header_Byte3(Char))
                    {
                        if (TextElement->Index < 0) break;
                        TextElement->Text[TextElement->Index] = 0;
                        break;
                    }
                    else if (TailBytesSkipped == 3 && Is_Utf8_Header_Byte4(Char))
                    {
                        if (TextElement->Index < 0) break;
                        TextElement->Text[TextElement->Index] = 0;
                        break;
                    }
                    else
                    {
                        /* NOTE: Delete an ASCII character. */
                        TextElement->Text[TextElement->Index] = 0;
                        break;
                    }
                }
            }
        } break;
        }
    }
}

void UpdateUserInputForUi(ui *Ui)
{
    u32 Key;
    Ui->QueueIndex = 0;
    Ui->Hot = 0;
    b32 KeysWerePressed = 0;

    {
        b32 ShiftDown = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
        b32 ControlDown = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
        b32 AltDown = IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT);
        b32 SuperDown = IsKeyDown(KEY_LEFT_SUPER) || IsKeyDown(KEY_RIGHT_SUPER);

        Assign_Flag(Ui->ModifierFlags, modifier_key_Shift, ShiftDown);
        Assign_Flag(Ui->ModifierFlags, modifier_key_Control, ControlDown);
        Assign_Flag(Ui->ModifierFlags, modifier_key_Alt, AltDown);
        Assign_Flag(Ui->ModifierFlags, modifier_key_Super, SuperDown);
    }

    Ui->MousePosition = GetMousePosition();
    Ui->MouseButtonPressed = IsMouseButtonPressed(0);
    Ui->MouseButtonReleased = IsMouseButtonReleased(0);

    for (u32 I = 0; I < Key_Press_Queue_Size; ++I)
    {
        Ui->KeyEventQueue[I] = (key_event){};
    }

    for (;;)
    {
        Key = GetKeyPressed();
        b32 QueueHasRoom = Ui->QueueIndex < Key_Press_Queue_Size;

        if (!(Key && QueueHasRoom))
        {
            break;
        }

        Ui->KeyEventQueue[Ui->QueueIndex].Key = Key;
        Ui->QueueIndex += 1;
        KeysWerePressed = 1;
        Ui->KeyRepeatTime = 0.0f;
        Ui->KeyHasRepeated = 0;
        Ui->LastKeyPressed = Key;

        Ui->InputOccured = 1;
    }

    if (!KeysWerePressed && IsKeyDown(Ui->LastKeyPressed))
    {
        Ui->KeyRepeatTime += Ui->DeltaTime;

        b32 InitialKeyRepeat = !Ui->KeyHasRepeated && Ui->KeyRepeatTime > 0.3f;
        /* NOTE: If the key repeat time is short enough, we may need to handle multiple repeats in a single frame. */
        b32 TheOtherKeyRepeats = Ui->KeyHasRepeated && Ui->KeyRepeatTime > 0.04f;

        if (InitialKeyRepeat || TheOtherKeyRepeats)
        {
            Ui->KeyRepeatTime = 0.0f;
            Ui->KeyHasRepeated = 1;
            Ui->InputOccured = 1;

            if (Ui->QueueIndex < Key_Press_Queue_Size)
            {
                Ui->KeyEventQueue[Ui->QueueIndex].Key = Ui->LastKeyPressed;
                Ui->QueueIndex += 1;
            }
        }
    }

    for (u32 Key = 0; Key < Total_Number_Of_Keys; ++Key)
    {
        /* NOTE: Update state for each key */
        u32 NextKeyStateIndex = !Ui->KeyStateIndex;
        b32 IsDown = IsKeyDown(Key);
        key_location Location = GetKeyLocation(Key);

        if (KeyLocationIsValid(Location))
        {
            u32 KeyStateChunk = Ui->KeyStates[Ui->KeyStateIndex][Location.Index];
            key_state CurrentState = (KeyStateChunk >> Location.Shift) & Bits_Per_Key_State_Mask;
            key_state NextState = KeyStateMap[CurrentState][IsDown]; /* TODO: What is NextState used for? */

            Assert(NextState < key_state_Count);

            if (CurrentState != key_Up || NextState != key_Up)
            {
                /* TODO: do we really want to set InputOccured to 1 here??? */
                Ui->InputOccured = 1;
            }

            u32 UnsetMask = ~(Bits_Per_Key_State_Mask << Location.Shift);
            u32 NewChunk = (KeyStateChunk & UnsetMask) | (NextState << Location.Shift);

            Ui->KeyStates[NextKeyStateIndex][Location.Index] = NewChunk;
        }
        else
        {
            /* NOTE: Error */
        }
    }
}
