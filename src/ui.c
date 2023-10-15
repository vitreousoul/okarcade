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

b32 DoButton(ui *UI, button Button)
{
    Color ButtonColor = (Color){20,180,180,255};
    Color TextColor = (Color){0,0,0,255};

    DrawRectangle(Button.Position.x, Button.Position.y, Button.Size.x, Button.Size.y, ButtonColor);
    DrawText(Button.Text,
             Button.Position.x + BUTTON_PADDING, Button.Position.y + BUTTON_PADDING,
             UI->FontSize, TextColor);
    return 0;
}
