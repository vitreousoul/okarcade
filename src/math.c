typedef struct
{
    s32 x;
    s32 y;
} v2s32;

f32 MinF32(f32 A, f32 B);
f32 MaxF32(f32 A, f32 B);
f32 ClampF32(f32 Value, f32 Low, f32 High);
f32 AbsF32(f32 Value);

s32 MinS32(s32 A, s32 B);
s32 MaxS32(s32 A, s32 B);

Vector2 RotateV2(Vector2 V, f32 Theta);
Vector2 AddV2(Vector2 A, Vector2 B);
Vector2 SubtractV2(Vector2 A, Vector2 B);
Vector2 MultiplyV2(Vector2 A, Vector2 B);
Vector2 DivideV2(Vector2 A, Vector2 B);
Vector2 DivideV2S(Vector2 A, f32 S);

f32 DotV2(Vector2 A, Vector2 B);
Vector2 ProjectV2(Vector2 Base, Vector2 PointA, Vector2 PointB);

Vector2 SwapV2(Vector2 V);
Vector2 AbsV2(Vector2 V);

Vector2 AddV2S(Vector2 A, f32 S);
Vector2 MultiplyV2S(Vector2 A, f32 S);

Rectangle MultiplyR2S(Rectangle Rect, f32 S);

f32 LengthSquaredV2(Vector2 Vector);
f32 LengthV2(Vector2 Vector);
f32 DistanceSquaredV2(Vector2 A, Vector2 B);
f32 DistanceV2(Vector2 A, Vector2 B);

Vector2 NormalizeV2(Vector2 Vector);

v2s32 Addv2s32(v2s32 A, v2s32 B);
v2s32 Subtractv2s32(v2s32 A, v2s32 B);
b32 AreEqualv2s32(v2s32 A, v2s32 B);

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

f32 AbsF32(f32 Value)
{
    f32 Result = Value;

    if (Result < 0.0f)
    {
        Result = -1.0f * Result;
    }

    return Result;
}

f32 LengthSquaredV2(Vector2 Vector)
{
    f32 Result = Vector.x * Vector.x + Vector.y * Vector.y;

    return Result;
}


f32 LengthV2(Vector2 Vector)
{
    f32 Result = sqrt(Vector.x * Vector.x + Vector.y * Vector.y);

    return Result;
}

f32 DistanceSquaredV2(Vector2 A, Vector2 B)
{
    return LengthSquaredV2(SubtractV2(A, B));
}

f32 DistanceV2(Vector2 A, Vector2 B)
{
    f32 Result = sqrt(LengthSquaredV2(SubtractV2(A, B)));

    return Result;
}

Vector2 RotateV2(Vector2 V, f32 Theta)
{
    f32 CosineTheta = cos(Theta);
    f32 SineTheta = sin(Theta);
    f32 X = (V.x * CosineTheta) - (V.y * SineTheta);
    f32 Y = (V.x * SineTheta) + (V.y * CosineTheta);

    return (Vector2){X, Y};
}

Vector2 AddV2(Vector2 A, Vector2 B)
{
    Vector2 Result = (Vector2){ A.x + B.x, A.y + B.y };
    return Result;
}

Vector2 SubtractV2(Vector2 A, Vector2 B)
{
    Vector2 Result = (Vector2){ A.x - B.x, A.y - B.y };
    return Result;
}

Vector2 MultiplyV2(Vector2 A, Vector2 B)
{
    Vector2 Result = (Vector2){ A.x * B.x, A.y * B.y };
    return Result;
}

Vector2 DivideV2(Vector2 A, Vector2 B)
{
    Vector2 Result = (Vector2){ A.x / B.x, A.y / B.y };
    return Result;
}

Vector2 DivideV2S(Vector2 A, f32 S)
{
    Vector2 Result = (Vector2){ A.x / S, A.y / S };
    return Result;
}

f32 DotV2(Vector2 A, Vector2 B)
{
    f32 Result = A.x * B.x + A.y * B.y;
    return Result;
}

Vector2 ProjectV2(Vector2 Base, Vector2 PointA, Vector2 PointB)
{
    Vector2 A = SubtractV2(PointA, Base);
    Vector2 B = SubtractV2(PointB, Base);

    Vector2 NormalB = NormalizeV2(B);

    Vector2 Projection = MultiplyV2S(NormalB, DotV2(A, NormalB));
    Projection = AddV2(Projection, Base);

    return Projection;
}

Vector2 SwapV2(Vector2 V)
{
    Vector2 Result = (Vector2){V.y, V.x};
    return Result;
}

Vector2 AbsV2(Vector2 V)
{
    Vector2 Result = (Vector2){AbsF32(V.x), AbsF32(V.y)};
    return Result;
}

Vector2 MultiplyV2S(Vector2 A, f32 S)
{
    Vector2 Result = (Vector2){A.x * S, A.y * S};
    return Result;
}

Rectangle MultiplyR2S(Rectangle Rect, f32 S)
{
    Rectangle Result = (Rectangle){S * Rect.x,
                                   S * Rect.y,
                                   S * Rect.width,
                                   S * Rect.height};

    return Result;
}

Vector2 AddV2S(Vector2 A, f32 S)
{
    Vector2 Result = (Vector2){A.x + S, A.y + S};
    return Result;
}

s32 MinS32(s32 A, s32 B)
{
    return (A < B) ? A : B;
}

s32 MaxS32(s32 A, s32 B)
{
    return (A > B) ? A : B;
}

Vector2 NormalizeV2(Vector2 Vector)
{
    f32 Length = LengthV2(Vector);
    Vector2 Result;

    if (Length == 0.0f)
    {
        Result = (Vector2){0.0f, 0.0f};
    }
    else
    {
        Result = DivideV2S(Vector, Length);
    }

    return Result;
}

v2s32 Addv2s32(v2s32 A, v2s32 B)
{
    v2s32 Result = (v2s32){A.x + B.x, A.y + B.y};
    return Result;
}

v2s32 Subtractv2s32(v2s32 A, v2s32 B)
{
    v2s32 Result = (v2s32){A.x - B.x, A.y - B.y};
    return Result;
}

b32 AreEqualv2s32(v2s32 A, v2s32 B)
{
    b32 Result = (A.x == B.x) && (A.y == B.y);
    return Result;
}
