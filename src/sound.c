#pragma pack(push,1)
typedef struct
{
    /* Contains the letters "RIFF" in ASCII form */
    /* (0x52494646 big-endian form). */
    u8 ChunkId[4];

    /* 36 + SubChunk2Size, or more precisely: */
    /*           4 + (8 + SubChunk1Size) + (8 + SubChunk2Size) */
    /*           This is the size of the rest of the chunk */
    /*           following this number.  This is the size of the */
    /*           entire file in bytes minus 8 bytes for the */
    /*           two fields not included in this count: */
    /*           ChunkId and ChunkSize. */
    u32 ChunkSize;

    /* Contains the letters "WAVE" */
    /* (0x57415645 big-endian form). */
    u8 Format[4];

    /* The "WAVE" format consists of two subchunks: "fmt " and "data": */
    /* The "fmt " subchunk describes the sound data's format: */

    /* Contains the letters "fmt " */
    /* (0x666d7420 big-endian form). */
    u8 FmtChunkId[4];

    /* 16 for PCM.  This is the size of the */
    /* rest of the Subchunk which follows this number. */
    u32 FmtChunkSize;

    /* PCM = 1 (i.e. Linear quantization) */
    /* Values other than 1 indicate some  */
    /* form of compression. */
    u16 AudioFormat;

    /* Mono = 1, Stereo = 2, etc. */
    u16 NumChannels;

    /* 8000, 44100, etc. */
    u32 SampleRate;

    /* == SampleRate * NumChannels * BitsPerSample/8 */
    u32 ByteRate;

    /* == NumChannels * BitsPerSample/8 */
    /* The number of bytes for one sample including */
    /* all channels. I wonder what happens when */
    /* this number isn't an integer? */
    u16 BlockAlign;

    /* 8 bits = 8, 16 bits = 16, etc. */
    u16 BitsPerSample;

    /* if PCM, then doesn't exist */
    /* X   ExtraParams      space for extra parameters */
    u16 ExtraParamSize;

    /* The "data" subchunk contains the size of the data and the actual sound: */

    /* Contains the letters "data" */
    /* (0x64617461 big-endian form). */
    u8 DataChunkId[4];

    /* == NumSamples * NumChannels * BitsPerSample/8 */
    /* This is the number of bytes in the data. */
    /* You can also think of this as the size */
    /* of the read of the subchunk following this  */
    /* number. */
    u32 DataChunkSize;

    /* 44        *   Data             The actual sound data. */
    u8 *Data;
} wav_header;
#pragma pack(pop)

Wave LoadSoundData(u8 *Data, s32 Size)
{
    Wave Wave = LoadWaveFromMemory(".wav", Data, Size);

    return Wave;
}

void OkPlaySound(Sound Sound)
{
    if (IsSoundReady(Sound))
    {
        PlaySound(Sound);
    }
}
