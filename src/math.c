Vector2 RotateVector2(Vector2 V, f32 Theta);

Vector2 RotateVector2(Vector2 V, f32 Theta)
{
    f32 CosineTheta = cos(Theta);
    f32 SineTheta = sin(Theta);
    f32 X = (V.x * CosineTheta) - (V.y * SineTheta);
    f32 Y = (V.x * SineTheta) + (V.y * CosineTheta);

    return (Vector2){X, Y};
}
