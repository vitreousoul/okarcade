f32 MinF32(f32 A, f32 B);
f32 MaxF32(f32 A, f32 B);
f32 ClampF32(f32 Value, f32 Low, f32 High);

Vector2 RotateVector2(Vector2 V, f32 Theta);
Vector2 NormalizeVector2(Vector2 Vector);


f32 MinF32(f32 A, f32 B)
{
    return (A < B) ? A : B;
}

f32 MaxF32(f32 A, f32 B)
{
    return (A > B) ? A : B;
}

f32 ClampF32(f32 Value, f32 Low, f32 High)
{
    return MinF32(MaxF32(Value, Low), High);
}

Vector2 RotateVector2(Vector2 V, f32 Theta)
{
    f32 CosineTheta = cos(Theta);
    f32 SineTheta = sin(Theta);
    f32 X = (V.x * CosineTheta) - (V.y * SineTheta);
    f32 Y = (V.x * SineTheta) + (V.y * CosineTheta);

    return (Vector2){X, Y};
}
