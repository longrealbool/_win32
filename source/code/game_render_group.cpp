
inline v4 
SRGB255ToLinear1(v4 C) {
  
  v4 Result;
  
  real32 Inv255 = 1.0f/255.0f;
  
  Result.r = Square(Inv255*C.r);
  Result.g = Square(Inv255*C.g);
  Result.b = Square(Inv255*C.b);
  Result.a = Inv255*C.a;
  
  return Result;
};


inline v4 
Linear1ToSRGB255(v4 C) {
  
  v4 Result;
  
  Result.r = 255.0f*SquareRoot(C.r);
  Result.g = 255.0f*SquareRoot(C.g);
  Result.b = 255.0f*SquareRoot(C.b);
  Result.a = 255.0f*C.a;
  
  return Result;
}

inline v4
UnscaleAndBiasNormal(v4 Normal) {
  
  v4 Result;
  real32 Inv255 = 1.0f/255.0f;
  Result.x = -1.0f + 2.0f*Inv255*Normal.x;
  Result.y = -1.0f + 2.0f*Inv255*Normal.y;
  Result.z = -1.0f + 2.0f*Inv255*Normal.z;
  
  Result.w = Inv255*Normal.w;
  
  return Result;
}


internal render_group *
AllocateRenderGroup(memory_arena *Arena, uint32 MaxPushBufferSize) {
  
  render_group *Result = PushStruct(Arena, render_group);
  Result->PushBufferBase = (uint8 *)PushSize(Arena, MaxPushBufferSize);
  Result->PushBufferSize = 0;
  Result->MaxPushBufferSize = MaxPushBufferSize;
  
  Result->GlobalAlpha = 1.0f;
  
  Result->Transform.OffsetP = V3(0, 0, 0);
  Result->Transform.Scale = 1.0f;
  
  return Result;
}


inline void
Perspective(render_group *RenderGroup, v2 DimInPixels, real32 MtP,
            real32 FocalLength, real32 CameraDistanceAboveGround) {

  real32 PtM = 1.0f / MtP;  
  RenderGroup->MonitorHalfDimInMeters = 0.5f*PtM*DimInPixels;
  
  RenderGroup->Transform.MtP = MtP;
  RenderGroup->Transform.FocalLength = FocalLength;
  RenderGroup->Transform.CameraDistanceAboveGround = CameraDistanceAboveGround;
  RenderGroup->Transform.NearClipPlane = 0.2f;
  RenderGroup->Transform.ScreenCenter = 0.5f*DimInPixels;
}


inline void
Orthographic(render_group *RenderGroup, v2 DimInPixels, real32 MtP ) {
  
  real32 PtM = 1.0f / MtP;
  RenderGroup->MonitorHalfDimInMeters = 0.5f*DimInPixels*PtM ;
  
  RenderGroup->Transform.MtP = MtP;
  RenderGroup->Transform.FocalLength = 1.0f;
  RenderGroup->Transform.CameraDistanceAboveGround = 1.0f;
  RenderGroup->Transform.NearClipPlane = 0.2f;
  RenderGroup->Transform.ScreenCenter = 0.5f*DimInPixels;
}


struct entity_basis_p_result {
  
  v2 P;
  real32 Scale;
  bool32 Valid;
};

inline entity_basis_p_result
GetRenderEntityBasisP(render_transform *Transform, v3 PushOffset) {

  entity_basis_p_result Result = {};
  
  v3 P = PushOffset + Transform->OffsetP;
  
  v2 ScreenCenter = Transform->ScreenCenter;
  real32 FocalLength = Transform->FocalLength;
  real32 CameraDistanceAboveGround = Transform->CameraDistanceAboveGround;// + 26.0f;
  real32 NearClipPlane = Transform->NearClipPlane;
  real32 MtP = Transform->MtP;
  
  real32 DistanceToPz = (CameraDistanceAboveGround - P.z);
  v3 RawXY = ToV3(P.xy, 1.0f);
  
  if(DistanceToPz > NearClipPlane) {
    
    v3 ProjectedXY = (1.0f/DistanceToPz) * RawXY*FocalLength;
    
    Result.P = ScreenCenter + MtP*ProjectedXY.xy;
    Result.Scale = MtP*ProjectedXY.z;
    Result.Valid = true;
  }
  
  return Result;
}


#define PushRenderElement(Group, Type) (Type *)PushRenderElement_(Group, sizeof(Type), RenderGroupEntryType_##Type)

internal void*
PushRenderElement_(render_group *Group, uint32 Size, render_group_entry_type Type) {
  
  void *Result = 0;
  
  Size += sizeof(render_group_entry_header);
  
  if((Group->PushBufferSize + Size) < Group->MaxPushBufferSize) {
    
    render_group_entry_header *Header = (render_group_entry_header *)(Group->PushBufferBase + Group->PushBufferSize);
    Header->Type = Type;
    Result = (Header + 1);
    Group->PushBufferSize += Size;
  }
  else {
    
    InvalidCodePath;
  }
  
  return Result;
}


internal void 
PushBitmap(render_group *Group, loaded_bitmap *Bitmap, v3 Offset, real32 Height,
           v4 Color = V4(1,1,1,1)) {
  
  v2 Size = V2(Height * Bitmap->WidthOverHeight, Height);
  v2 Align = Hadamard(Bitmap->AlignPercentage, Size);
  v3 PushOffset = Offset - ToV3(Align, 0);
  
  entity_basis_p_result Basis = GetRenderEntityBasisP(&Group->Transform, PushOffset);
  
  if(Basis.Valid) {
    
    render_entry_bitmap *Entry = PushRenderElement(Group, render_entry_bitmap);
    if(Entry) {
      
      // NOTE(Egor): this is actual width and height in meters
      
      Entry->Size = Size*Basis.Scale;
      Entry->P = Basis.P;
      Entry->Bitmap = Bitmap;
      Entry->Color = Group->GlobalAlpha*Color;
    }
  }
}

internal void 
PushRect(render_group *Group, v3 Offset, v2 Dim, v4 Color) {

  v3 PushOffset = Offset - ToV3(0.5f*Dim, 0);
  entity_basis_p_result Basis = GetRenderEntityBasisP(&Group->Transform, PushOffset);
  
  if(Basis.Valid) {
    
    render_entry_rectangle *Entry = PushRenderElement(Group, render_entry_rectangle);
    if(Entry) {
      
      Entry->Flag = Group->Transform.DebugFlag;
      Entry->Dim = Basis.Scale*Dim;
      Entry->Color = Color;
      Entry->P = Basis.P;
      
    }
  }
}

internal void 
PushRectOutline(render_group *Group, v3 Offset, v2 Dim, v4 Color) {
  
  real32 Thickness = 0.10f;
  
  PushRect(Group, Offset - V3(0, Dim.y, 0)*0.5f, V2(Dim.x, Thickness), Color);
  PushRect(Group, Offset + V3(0, Dim.y, 0)*0.5f, V2(Dim.x, Thickness), Color);
  
  PushRect(Group, Offset - V3(Dim.x, 0, 0)*0.5f, V2(Thickness, Dim.y), Color);
  PushRect(Group, Offset + V3(Dim.x, 0, 0)*0.5f, V2(Thickness, Dim.y), Color);
}



internal void 
DrawBitmap(loaded_bitmap *Buffer,
           loaded_bitmap *Bitmap,
           real32 RealX, real32 RealY, real32 CAlpha = 1.0f) {
  
  int32 MinX = RoundReal32ToInt32(RealX);
  int32 MinY = RoundReal32ToInt32(RealY);
  int32 MaxX = MinX + Bitmap->Width;
  int32 MaxY = MinY + Bitmap->Height;
  
  
  
  int32 SourceOffsetX = 0;
  // clipping value to actual size of the backbuffer
  if(MinX < 0) {
    SourceOffsetX = -MinX;
    MinX = 0;
  }
  int32 SourceOffsetY = 0;
  if(MinY < 0) { 
    SourceOffsetY = -MinY;
    MinY = 0;
  }
  // we will write up to, but not including to final row and column
  if(MaxX > Buffer->Width) MaxX = Buffer->Width;
  if(MaxY > Buffer->Height) MaxY = Buffer->Height;
  
  // NOTE(Egor): just example
  // uint8 *EndOfBuffer = (uint8 *)Buffer->Memory + Buffer->Pitch*Buffer->Height;
  
  
  
  uint8 *Test = (uint8 *)Bitmap->Memory;
  uint8 *SourceRow = (uint8 *)Bitmap->Memory + SourceOffsetY*Bitmap->Pitch
    + BITMAP_BYTES_PER_PIXEL*SourceOffsetX;
  
  // go to line to draw
  uint8 *DestRow = ((uint8 *)Buffer->Memory + MinY*Buffer->Pitch + MinX*BITMAP_BYTES_PER_PIXEL);
  
  for(int Y = MinY; Y < MaxY; ++Y)
  {
    uint32 *Dest = (uint32 *)DestRow;
    uint32 *Source = (uint32 *)SourceRow;
    for(int X = MinX; X < MaxX; ++X)
    {
      
      
      v4 Texel = V4((real32)((*Source >> 16) & 0xFF),
                    (real32)((*Source >> 8) & 0xFF),
                    (real32)((*Source >> 0) & 0xFF),
                    (real32)((*Source >> 24) & 0xFF));
      
      Texel = SRGB255ToLinear1(Texel);
      Texel *= CAlpha;
      
      v4 D = V4((real32)((*Dest >> 16) & 0xFF),
                (real32)((*Dest >> 8) & 0xFF),
                (real32)((*Dest >> 0) & 0xFF),
                (real32)((*Dest >> 24) & 0xFF));
      
      D = SRGB255ToLinear1(D);
      
      real32 RAsComplement = (1 - Texel.a);
      
      // NOTE(Egor): alpha channel for compisited bitmaps in premultiplied alpha mode
      // for case when we create intermediate buffer with two or more bitmaps blend with 
      // each other
      
      v4 Blended = V4(RAsComplement*D.r + Texel.r,
                      RAsComplement*D.g + Texel.g,
                      RAsComplement*D.b + Texel.b,
                      (Texel.a + D.a - Texel.a*D.a));
      
      Blended = Linear1ToSRGB255(Blended);
      
      *Dest = (((uint32)(Blended.a + 0.5f) << 24) |
               ((uint32)(Blended.r + 0.5f) << 16) |
               ((uint32)(Blended.g + 0.5f) << 8)  |
               ((uint32)(Blended.b + 0.5f) << 0));
      
      Dest++;
      Source++;
    }
    DestRow += Buffer->Pitch;
    SourceRow += Bitmap->Pitch;
    
  }
}

internal void
DrawRectangle(loaded_bitmap *Buffer, v2 vMin, v2 vMax, v4 Color, 
              rectangle2i ClipRect, bool32 Even)
{
  
  rectangle2i FillRect; // = InvertedMaxRectangle();
  
  FillRect.XMin = RoundReal32ToInt32(vMin.x);
  FillRect.YMin = RoundReal32ToInt32(vMin.y);
  FillRect.XMax = RoundReal32ToInt32(vMax.x);
  FillRect.YMax = RoundReal32ToInt32(vMax.y);
  
  FillRect = Intersect(ClipRect, FillRect);
  
  if(!Even == (FillRect.YMin & 1)) {
    
    FillRect.YMin += 1;
  }
  
  uint32 PixColor = ((RoundReal32ToUInt32(Color.a * 255.0f) << 24) |
                     (RoundReal32ToUInt32(Color.r * 255.0f) << 16) |
                     (RoundReal32ToUInt32(Color.g * 255.0f) << 8)  |
                     (RoundReal32ToUInt32(Color.b * 255.0f) << 0));
  
  
  uint8 *EndOfBuffer = (uint8 *)Buffer->Memory + Buffer->Pitch*Buffer->Height;
  
  // go to line to draw
  uint8 *Row = ((uint8 *)Buffer->Memory + FillRect.YMin*Buffer->Pitch + FillRect.XMin*BITMAP_BYTES_PER_PIXEL);
  // TODO: color
  for(int Y = FillRect.YMin; Y < FillRect.YMax; Y += 2)
  {
    uint32 *Pixel = (uint32 *)Row;
    for(int X = FillRect.XMin; X < FillRect.XMax; ++X)
    {
      *Pixel++ = PixColor;
    }
    Row += (Buffer->Pitch*2);
  }
}




inline v4
Unpack4x8(uint32 Packed) {
  
  v4 Result = V4((real32)((Packed >> 16) & 0xFF),
                 (real32)((Packed >> 8) & 0xFF),
                 (real32)((Packed >> 0) & 0xFF),
                 (real32)((Packed >> 24) & 0xFF));
  
  return Result;
}


inline bilinear_sample
BilinearSample(loaded_bitmap *Texture, int32 X, int32 Y) {
  
  bilinear_sample Result;
  
  uint8 *TexelPtr = ((uint8 *)Texture->Memory) + Y*Texture->Pitch + X*BITMAP_BYTES_PER_PIXEL;
  
  Result.A = *(uint32 *)(TexelPtr);
  Result.B = *(uint32 *)(TexelPtr + sizeof(uint32));
  Result.C = *(uint32 *)(TexelPtr + Texture->Pitch);
  Result.D = *(uint32 *)(TexelPtr + Texture->Pitch + sizeof(uint32));
  
  return Result;
}

inline v4
SRGBBilinearBlend(bilinear_sample Sample, real32 fX, real32 fY) {
  
  v4 TexelA  = Unpack4x8(Sample.A);
  v4 TexelB  = Unpack4x8(Sample.B);
  v4 TexelC  = Unpack4x8(Sample.C);
  v4 TexelD  = Unpack4x8(Sample.D);
  // NOTE(Egor): from SRGB to 'linear' brightness space
  TexelA = SRGB255ToLinear1(TexelA);
  TexelB = SRGB255ToLinear1(TexelB);
  TexelC = SRGB255ToLinear1(TexelC);
  TexelD = SRGB255ToLinear1(TexelD);
  
  v4 Result = Lerp(Lerp(TexelA, fX, TexelB),
                   fY,
                   Lerp(TexelC, fX, TexelD));
  
  return Result;
}


inline v3
SampleEnvironmentMap(v2 ScreenSpaceUV, v3 SampleDirection, real32 Roughness, 
                     environment_map *Map, real32 DistanceFromMapInZ) {
  
  uint32 LODIndex = (uint32)(Roughness*(real32)(ArrayCount(Map->LOD)-1) + 0.5f);
  Assert(LODIndex < ArrayCount(Map->LOD));
  loaded_bitmap *LOD = &Map->LOD[LODIndex];
  
  // NOTE(Egor): Compute the distance to the map
  // (thing that have image on it we want to reflect)
  // UV is a scaling factor from relative coordinates to real world distance
  real32 UVsPerMeter = 0.01f;
  real32 c = (UVsPerMeter*DistanceFromMapInZ) / SampleDirection.y;
  
  // NOTE(Egor): find the place we take color from
  // we use Z coordinate instead of Y because Y is our top-to-bottom coordinate
  // and Z is our out of the screen coordinate
  v2 Offset = c * V2(SampleDirection.x, SampleDirection.z);
  v2 UV = Offset + ScreenSpaceUV;
  
  UV.x = Clamp01(UV.x);
  UV.y = Clamp01(UV.y);
  
  real32 tX = (UV.x * (real32)(LOD->Width - 2));
  real32 tY = (UV.y * (real32)(LOD->Height - 2));
  int32 X = (int32)tX;
  int32 Y = (int32)tY;
  // NOTE(Egor): take the fractional part of tX and tY
  real32 fX = tX - (real32)X;
  real32 fY = tY - (real32)Y;
  Assert(X >= 0 && X <= (LOD->Width));
  Assert(Y >= 0 && Y <= (LOD->Height));
  
#if 0  
  uint8 *TexelPtr = ((uint8 *)LOD->Memory) + Y*LOD->Pitch + X*BITMAP_BYTES_PER_PIXEL;
  *(uint32 *)TexelPtr = 0xFFFFFFFF;
#endif
  
  bilinear_sample Sample = BilinearSample(LOD, X, Y);
  v3 Result = SRGBBilinearBlend(Sample, fX, fY).rgb;
  
  return Result;
}


#if 1 // NOTE(Egor): IACA SWITCH
#include "iacaMarks.h"
#else

#define IACA_VC64_START
#define IACA_VC64_END

#endif


internal void
DrawRectangleSlowly(loaded_bitmap *Buffer, render_v2_basis Basis, v4 Color,
                    loaded_bitmap *Texture,
                    rectangle2i ClipRect,
                    real32 PtM, bool32 Even) {
  
  BEGIN_TIMED_BLOCK(DrawRectangleSlowly);
  
  //NOTE(Egor): premultiply color
  Color.rgb *= Color.a;
  
#if 1
  
  // NOTE(Egor): specific order for algorithm
  rectangle2i FillRect = InvertedHalfMaxRectangle();
#else
  
  rectangle2i FillRect = HalfMaxRectangle();
#endif
  
  // NOTE(Egor): take points of (possibly) rotated coordinate axis to determine
  // area that includes shape
  v2 Min = Basis.Origin;
  v2 Max = Basis.Origin + Basis.XAxis + Basis.YAxis;
  v2 P[4] = {Min, Min + Basis.XAxis, Min + Basis.YAxis, Max};
  
  for(uint32 Index = 0; Index < ArrayCount(P); ++Index) {
    
    v2 *TestP = P + Index;
    
    int32 CeilX = CeilReal32ToInt32(TestP->x) + 1;
    int32 FloorX = FloorReal32ToInt32(TestP->x);
    int32 CeilY = CeilReal32ToInt32(TestP->y) + 1;
    int32 FloorY = FloorReal32ToInt32(TestP->y);
    
    if(FillRect.XMin > FloorX) FillRect.XMin = FloorX;
    if(FillRect.YMin > FloorY) FillRect.YMin = FloorY;
    if(FillRect.XMax < CeilX) FillRect.XMax = CeilX;
    if(FillRect.YMax < CeilY) FillRect.YMax = CeilY;
  }
  
  FillRect = Intersect(ClipRect, FillRect);
  
  if(HasArea(FillRect)) {
    
    // NOTE(Egor): interleaved alternating scanning lines
    if(!Even == (FillRect.YMin & 1)) {
      
      FillRect.YMin += 1;
    }
    
    __m128i StartClipMask = _mm_set1_epi8(-1);
    __m128i EndClipMask = _mm_set1_epi8(-1);
    
    __m128i StartClipMasks[] = {
      _mm_slli_si128(StartClipMask, 0*4),
      _mm_slli_si128(StartClipMask, 1*4),
      _mm_slli_si128(StartClipMask, 2*4),
      _mm_slli_si128(StartClipMask, 3*4),
    };
    
    __m128i EndClipMasks[] = {
      _mm_srli_si128(EndClipMask, 0*4),
      _mm_srli_si128(EndClipMask, 3*4),
      _mm_srli_si128(EndClipMask, 2*4),
      _mm_srli_si128(EndClipMask, 1*4),
    };
    
    if(FillRect.XMin & 3) {

      StartClipMask = StartClipMasks[FillRect.XMin & 3];
      FillRect.XMin = FillRect.XMin & ~3;
    }
    
    if(FillRect.XMax & 3) {
      
      EndClipMask = EndClipMasks[FillRect.XMax & 3];
      FillRect.XMax = (FillRect.XMax & ~3) + 4;
    }
    
#if 0
    int32 FillWidth = FillRect.XMax - FillRect.XMin;
    int32 FillWidthAlign = FillWidth & 0x3;
    int32 Adjust = (~FillWidthAlign + 1) & 0x3;
    FillWidth += Adjust;
    FillRect.XMin = FillRect.XMax - FillWidth;
    
    // TODO(Egor): do not use madskills
    __m128i Dummy = _mm_setr_epi32(0,1,2,3);
    __m128i StartupClipMask = _mm_sub_epi32(Dummy, _mm_set1_epi32(Adjust));
    StartupClipMask = _mm_cmplt_epi32(StartupClipMask, _mm_set1_epi32(0));
    StartupClipMask = _mm_andnot_si128(StartupClipMask, _mm_set1_epi8(-1));
#endif
    
    real32 InvXAxisLengthSq = 1.0f/LengthSq(Basis.XAxis);
    real32 InvYAxisLengthSq = 1.0f/LengthSq(Basis.YAxis);
    
    v2 nXAxis = InvXAxisLengthSq * Basis.XAxis;
    v2 nYAxis = InvYAxisLengthSq * Basis.YAxis;
    
    __m128 nXAxisX_4x = _mm_set1_ps(nXAxis.x);
    __m128 nXAxisY_4x = _mm_set1_ps(nXAxis.y);
    
    __m128 nYAxisX_4x = _mm_set1_ps(nYAxis.x);
    __m128 nYAxisY_4x = _mm_set1_ps(nYAxis.y);
    
    real32 Inv255 = 1.0f/255.0f;
    
    // NOTE(Egor): color modulation values in SIMD
    __m128 ColorR_4x = _mm_set1_ps(Color.r);
    __m128 ColorG_4x = _mm_set1_ps(Color.g);
    __m128 ColorB_4x = _mm_set1_ps(Color.b);
    __m128 ColorA_4x = _mm_set1_ps(Color.a);
    
    __m128 ColorA_INV_4x = _mm_set1_ps(Color.a*Inv255);
    
    __m128 One = _mm_set1_ps(1.0f);
    __m128 Two = _mm_set1_ps(2.0f);
    __m128 Zero = _mm_set1_ps(0.0f);
    __m128 Four = _mm_set1_ps(4.0f);
    __m128i MaskFF = _mm_set1_epi32(0xFF);
    __m128i MaskFFFF = _mm_set1_epi32(0xFFFF);
    __m128i MaskFF00FF = _mm_set1_epi32(0x00FF00FF);
    
    __m128 Squared255 = _mm_set1_ps(255.0f*255.0f);
    
    __m128 WidthM2_4x = _mm_set1_ps((real32)(Texture->Width - 2));
    __m128 HeightM2_4x = _mm_set1_ps((real32)(Texture->Height - 2));
    
    int32 TexturePitch = Texture->Pitch;
    __m128i TexturePitch_4x = _mm_set1_epi32(TexturePitch);
    void *TextureMemory = Texture->Memory;
    uint32 RowAdvance = Buffer->Pitch*2;
    
    uint8 *Row = (uint8 *)Buffer->Memory + FillRect.YMin*Buffer->Pitch + FillRect.XMin*BITMAP_BYTES_PER_PIXEL;
#if 0
    int32 Align = (uintptr)Row & (16 - 1);
    Row -= Align;
#endif
    
#define M(a, I) ((real32 *)&(a))[I]
#define Mi(a, I) ((uint32 *)&(a))[I]
#define mm_square(a) _mm_mul_ps(a, a)
    
#if 0 // NOTE(Egor): disable gamma correction
    
#undef mm_square
#define mm_square(a) a;
#define _mm_sqrt_ps(a) a
#endif
    
    __m128 OriginX_4x = _mm_set1_ps(Basis.Origin.x);
    __m128 OriginY_4x = _mm_set1_ps(Basis.Origin.y);
    
    // NOTE(Egor): relative position of actual (ON_SCREEN) pixel inside texture
    // --> we find the vector, that point to a pixel inside a texture basis
    // NOTE(Egor): Get all 4 Pixels (X-OriginX)+3 (X-OriginX)+2 (X-OriginX)+1 (X-OriginX)+0
    // NOTE(Egor): Add3210_m_OriginX has baked in OriginX_4x
    // NOTE(Egor): This is when we reset PixelPX betwen Y - runs
    __m128 Add3210_m_OriginX = _mm_set_ps(3.0f, 2.0f, 1.0f, 0.0f);
    Add3210_m_OriginX = _mm_sub_ps(Add3210_m_OriginX, OriginX_4x);
    __m128 XMin_4x = _mm_set1_ps((real32)FillRect.XMin);
    XMin_4x = _mm_add_ps(XMin_4x, Add3210_m_OriginX);
    
    // NOTE(Egor): Get quad (Y-OriginY) pixel
    __m128 PixelPY = _mm_set1_ps((real32)FillRect.YMin);
    PixelPY = _mm_sub_ps(PixelPY, OriginY_4x);
    
    BEGIN_TIMED_BLOCK(ProcessPixel);
    
    for(int32 Y = FillRect.YMin; Y < FillRect.YMax; Y += 2) {
      
      __m128i ClipMask = StartClipMask;
      
#define TEST_PixelPY_x_NXAxisY 1 // NOTE(Egor): looks like disabled it's faster ¯\_(-_-)_/¯ 
      
#if TEST_PixelPY_x_NXAxisY
      // NOTE(Egor): only changes between Y - runs
      __m128 PixelPY_x_NXAxisY = _mm_mul_ps(nXAxisY_4x, PixelPY);
      __m128 PixelPY_x_NYAxisY = _mm_mul_ps(nYAxisY_4x, PixelPY);
#endif
      
      __m128 PixelPX = XMin_4x;
      
      uint32 *Pixel = (uint32 *)Row;
      
      for(int32 XI = FillRect.XMin; XI < FillRect.XMax; XI += 4) {
        
        IACA_VC64_START;
        
        // NOTE(Egor): this routine works with _ON_SCREEN_ pixels, and correspond them with
        // Texels inside Actual Texture that drawn to that part of _SCREEN_
        
        // NOTE(Egor): inner product that compute X and Y component of vector
        // 1. U and V is a normalized coefficients, U = X_component/XAxis_length
        // 2. nXAxisX is premultiplied to InvXAxis_Length
        // 3. PixelPY_x_NXAxisY is constant across all X - runs
#if TEST_PixelPY_x_NXAxisY
        
        __m128 U = _mm_add_ps(_mm_mul_ps(nXAxisX_4x, PixelPX), PixelPY_x_NXAxisY);
        __m128 V = _mm_add_ps(_mm_mul_ps(nYAxisX_4x, PixelPX), PixelPY_x_NYAxisY);
#else
        
        __m128 U = _mm_add_ps(_mm_mul_ps(nXAxisX_4x, PixelPX), _mm_mul_ps(nXAxisY_4x, PixelPY));
        __m128 V = _mm_add_ps(_mm_mul_ps(nYAxisX_4x, PixelPX), _mm_mul_ps(nYAxisY_4x, PixelPY));
        
#endif
        
        // NOTE(Egor): generate write mask that determine if we should write 
        // New Color Data, if pixels is out of bounds we just masked them out with
        // Old Color Data (Was Called OriginalDest -- possibly renamed)
        __m128i WriteMask = _mm_castps_si128(_mm_and_ps(_mm_and_ps(_mm_cmple_ps(U, One),
                                                                   _mm_cmpge_ps(U, Zero)),
                                                        _mm_and_ps(_mm_cmple_ps(V, One),
                                                                   _mm_cmpge_ps(V, Zero))));
        
        WriteMask = _mm_and_si128(WriteMask, ClipMask);
        // TODO(Egor): check later if it helps
        //      if(_mm_movemask_epi8(WriteMask))
        {
          
          __m128i OriginalDest = _mm_load_si128((__m128i *)Pixel);
          
          // NOTE(Egor): clamp U and V to prevent fetching nonexistent texel data
          // we could have U and V that exceeds 1.0f that can fetch outside of texture
          // buffer
          U = _mm_min_ps(_mm_max_ps(U, Zero), One);
          V = _mm_min_ps(_mm_max_ps(V, Zero), One);
          
          // NOTE(Egor): texture boundary multiplication to determine which pixel 
          // we need to fetch from texture buffer
          __m128 tX = _mm_mul_ps(U, WidthM2_4x);
          __m128 tY = _mm_mul_ps(V, HeightM2_4x);
          
          // NOTE(Egor): round with truncation
          __m128i FetchX_4x = _mm_cvttps_epi32(tX);
          __m128i FetchY_4x =_mm_cvttps_epi32(tY);
          
          // NOTE(Egor): take the fractional part of tX and tY
          __m128 fX = _mm_sub_ps(tX, _mm_cvtepi32_ps(FetchX_4x));
          __m128 fY = _mm_sub_ps(tY, _mm_cvtepi32_ps(FetchY_4x));
          
          // NOTE(Egor): fetch 4 texels from texture buffer
          
          FetchX_4x = _mm_slli_epi32(FetchX_4x, 2); 
          FetchY_4x = _mm_mullo_epi32(FetchY_4x, TexturePitch_4x); 
          FetchX_4x = _mm_add_epi32(FetchX_4x, FetchY_4x); 
          
          int32 Fetch0 = Mi(FetchX_4x, 0);
          int32 Fetch1 = Mi(FetchX_4x, 1);
          int32 Fetch2 = Mi(FetchX_4x, 2);
          int32 Fetch3 = Mi(FetchX_4x, 3);
          
          uint8 *TexelPtr0 = (((uint8 *)TextureMemory) + Fetch0);
          uint8 *TexelPtr1 = (((uint8 *)TextureMemory) + Fetch1);
          uint8 *TexelPtr2 = (((uint8 *)TextureMemory) + Fetch2);
          uint8 *TexelPtr3 = (((uint8 *)TextureMemory) + Fetch3);
          
          __m128i SampleA;
          __m128i SampleB;
          __m128i SampleC;
          __m128i SampleD;
          
          SampleA = _mm_setr_epi32(*(uint32 *)(TexelPtr0), 
                                   *(uint32 *)(TexelPtr1),
                                   *(uint32 *)(TexelPtr2),
                                   *(uint32 *)(TexelPtr3));
          
          SampleB = _mm_setr_epi32(*(uint32 *)(TexelPtr0 + sizeof(uint32)),
                                   *(uint32 *)(TexelPtr1 + sizeof(uint32)),
                                   *(uint32 *)(TexelPtr2 + sizeof(uint32)),
                                   *(uint32 *)(TexelPtr3 + sizeof(uint32)));
          
          SampleC = _mm_setr_epi32(*(uint32 *)(TexelPtr0 + TexturePitch),
                                   *(uint32 *)(TexelPtr1 + TexturePitch),
                                   *(uint32 *)(TexelPtr2 + TexturePitch),
                                   *(uint32 *)(TexelPtr3 + TexturePitch));
          
          SampleD = _mm_setr_epi32(*(uint32 *)(TexelPtr0 + TexturePitch + sizeof(uint32)),
                                   *(uint32 *)(TexelPtr1 + TexturePitch + sizeof(uint32)),
                                   *(uint32 *)(TexelPtr2 + TexturePitch + sizeof(uint32)),
                                   *(uint32 *)(TexelPtr3 + TexturePitch + sizeof(uint32)));
          
          //////
          __m128i TexelArb = _mm_and_si128(SampleA, MaskFF00FF);
          __m128i TexelAag = _mm_and_si128(_mm_srli_epi32(SampleA,  8), MaskFF00FF);
          TexelArb = _mm_mullo_epi16(TexelArb, TexelArb);
          __m128 TexelAa = _mm_cvtepi32_ps(_mm_srli_epi32(TexelAag, 16));
          TexelAag = _mm_mullo_epi16(TexelAag, TexelAag);
          
          //////
          __m128i TexelCrb = _mm_and_si128(SampleC, MaskFF00FF);
          __m128i TexelCag = _mm_and_si128(_mm_srli_epi32(SampleC,  8), MaskFF00FF);
          TexelCrb = _mm_mullo_epi16(TexelCrb, TexelCrb);
          __m128 TexelCa = _mm_cvtepi32_ps(_mm_srli_epi32(TexelCag, 16));
          TexelCag = _mm_mullo_epi16(TexelCag, TexelCag);
          
          //////
          __m128i TexelDrb = _mm_and_si128(SampleD, MaskFF00FF);
          __m128i TexelDag = _mm_and_si128(_mm_srli_epi32(SampleD,  8), MaskFF00FF);
          TexelDrb = _mm_mullo_epi16(TexelDrb, TexelDrb);
          __m128 TexelDa = _mm_cvtepi32_ps(_mm_srli_epi32(TexelDag, 16));
          TexelDag = _mm_mullo_epi16(TexelDag, TexelDag);
          
          // NOTE(Egor): Convert texture from SRGB to 'linear' brightness space
          __m128 TexelAr = _mm_cvtepi32_ps(_mm_srli_epi32(TexelArb, 16));
          __m128 TexelAg = _mm_cvtepi32_ps(_mm_and_si128(TexelAag, MaskFFFF));
          __m128 TexelAb = _mm_cvtepi32_ps(_mm_and_si128(TexelArb, MaskFFFF));
          
          __m128 TexelCr = _mm_cvtepi32_ps(_mm_srli_epi32(TexelCrb, 16));
          __m128 TexelCg = _mm_cvtepi32_ps(_mm_and_si128(TexelCag, MaskFFFF));
          __m128 TexelCb = _mm_cvtepi32_ps(_mm_and_si128(TexelCrb, MaskFFFF));
          
          __m128 TexelDr = _mm_cvtepi32_ps(_mm_srli_epi32(TexelDrb, 16));
          __m128 TexelDg = _mm_cvtepi32_ps(_mm_and_si128(TexelDag, MaskFFFF));
          __m128 TexelDb = _mm_cvtepi32_ps(_mm_and_si128(TexelDrb, MaskFFFF));
          
          // NOTE(Egor): convert to linear brightness space
          // Dest = SRGB255ToLinear1(Dest);
#if 0        
          
          __m128 TexelBb = _mm_cvtepi32_ps(_mm_and_si128(SampleB, MaskFF));
          __m128 TexelBg = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(SampleB, 8), MaskFF));
          __m128 TexelBr = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(SampleB, 16), MaskFF));
          __m128 TexelBa = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(SampleB, 24), MaskFF));
          TexelBr = mm_square(TexelBr);
          TexelBg = mm_square(TexelBg);
          TexelBb = mm_square(TexelBb);
          
          __m128 DestB = _mm_cvtepi32_ps(_mm_and_si128(OriginalDest, MaskFF));
          __m128 DestG = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(OriginalDest, 8), MaskFF));
          __m128 DestR = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(OriginalDest, 16), MaskFF));
          __m128 DestA = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(OriginalDest, 24), MaskFF));
          DestR = mm_square(DestR);
          DestG = mm_square(DestG);
          DestB = mm_square(DestB);
          
#else
          
          //////
          __m128i TexelBrb = _mm_and_si128(SampleB, MaskFF00FF);
          __m128i TexelBag = _mm_and_si128(_mm_srli_epi32(SampleB,  8), MaskFF00FF);
          TexelBrb = _mm_mullo_epi16(TexelBrb, TexelBrb);
          __m128 TexelBa = _mm_cvtepi32_ps(_mm_srli_epi32(TexelBag, 16));
          TexelBag = _mm_mullo_epi16(TexelBag, TexelBag);
          
          __m128 TexelBr = _mm_cvtepi32_ps(_mm_srli_epi32(TexelBrb, 16));
          __m128 TexelBg = _mm_cvtepi32_ps(_mm_and_si128(TexelBag, MaskFFFF));
          __m128 TexelBb = _mm_cvtepi32_ps(_mm_and_si128(TexelBrb, MaskFFFF));
          
          // NOTE(Egor): load destination color
          __m128i Destrb = _mm_and_si128(OriginalDest, MaskFF00FF);
          __m128i Destag = _mm_and_si128(_mm_srli_epi32(OriginalDest,  8), MaskFF00FF);
          Destrb = _mm_mullo_epi16(Destrb, Destrb);
          __m128 DestA = _mm_cvtepi32_ps(_mm_srli_epi32(Destag, 16));
          Destag = _mm_mullo_epi16(Destag, Destag);
          
          __m128 DestR = _mm_cvtepi32_ps(_mm_srli_epi32(Destrb, 16));
          __m128 DestG = _mm_cvtepi32_ps(_mm_and_si128(Destag, MaskFFFF));
          __m128 DestB = _mm_cvtepi32_ps(_mm_and_si128(Destrb, MaskFFFF));
          
#endif
          
          // NOTE(Egor): compute coefficients for for subpixel rendering
          __m128 ifX = _mm_sub_ps(One, fX);
          __m128 ifY = _mm_sub_ps(One, fY);
          __m128 L0 = _mm_mul_ps(ifY, ifX);
          __m128 L1 = _mm_mul_ps(ifY, fX);
          __m128 L2 = _mm_mul_ps(fY, ifX);
          __m128 L3 = _mm_mul_ps(fY, fX);
          
          // NOTE(Egor): subpixel blending
          // lerp 4 pixel square |A|B| into one pixel with weighted coefficients
          //                     |C|D| 
          // fX -> Fractional Part of X Texel from texture
          // fY -> Fractional Part of Y Textl from texture
          __m128 Texelr = _mm_add_ps(_mm_add_ps(_mm_mul_ps(L0, TexelAr),
                                                _mm_mul_ps(L1, TexelBr)),
                                     _mm_add_ps(_mm_mul_ps(L2, TexelCr),
                                                _mm_mul_ps(L3, TexelDr)));
          
          __m128 Texelg = _mm_add_ps(_mm_add_ps(_mm_mul_ps(L0, TexelAg),
                                                _mm_mul_ps(L1, TexelBg)),
                                     _mm_add_ps(_mm_mul_ps(L2, TexelCg),
                                                _mm_mul_ps(L3, TexelDg)));
          
          __m128 Texelb = _mm_add_ps(_mm_add_ps(_mm_mul_ps(L0, TexelAb),
                                                _mm_mul_ps(L1, TexelBb)),
                                     _mm_add_ps(_mm_mul_ps(L2, TexelCb),
                                                _mm_mul_ps(L3, TexelDb)));
          
          __m128 Texela = _mm_add_ps(_mm_add_ps(_mm_mul_ps(L0, TexelAa),
                                                _mm_mul_ps(L1, TexelBa)),
                                     _mm_add_ps(_mm_mul_ps(L2, TexelCa),
                                                _mm_mul_ps(L3, TexelDa)));
          
          // NOTE(Egor): Modulate by incoming color
          Texelr = _mm_mul_ps(Texelr, ColorR_4x);
          Texelg = _mm_mul_ps(Texelg, ColorG_4x);
          Texelb = _mm_mul_ps(Texelb, ColorB_4x);
          Texela = _mm_mul_ps(Texela, ColorA_INV_4x);
          
          // NOTE(Egor): clamp color to valid range
          Texelr = _mm_min_ps(_mm_max_ps(Texelr, Zero), Squared255);
          Texelg = _mm_min_ps(_mm_max_ps(Texelg, Zero), Squared255);
          Texelb = _mm_min_ps(_mm_max_ps(Texelb, Zero), Squared255);
          
          // NOTE(Egor): alpha channel for composited bitmaps is in premultiplied alpha mode
          // for case when we create intermediate buffer with two or more bitmaps blend with 
          // each other 
          
          // NOTE(Egor): destination blend
          //        v4 Blended = (1.0f - Texel.a)*Dest + Texel;
          __m128 OneComplementTexelA = _mm_sub_ps(One, Texela);
          __m128 BlendedR = _mm_add_ps(_mm_mul_ps(OneComplementTexelA, DestR), Texelr);
          __m128 BlendedG = _mm_add_ps(_mm_mul_ps(OneComplementTexelA, DestG), Texelg);
          __m128 BlendedB = _mm_add_ps(_mm_mul_ps(OneComplementTexelA, DestB), Texelb);
          __m128 BlendedA = _mm_add_ps(_mm_mul_ps(OneComplementTexelA, DestA), Texela);
          
          // NOTE(Egor): convert back to gamma 
          // v4 Blended255 = Linear1ToSRGB255(Blended);
          BlendedR = _mm_mul_ps(BlendedR, _mm_rsqrt_ps(BlendedR));
          BlendedG = _mm_mul_ps(BlendedG, _mm_rsqrt_ps(BlendedG));
          BlendedB = _mm_mul_ps(BlendedB, _mm_rsqrt_ps(BlendedB));
          BlendedA = BlendedA;
          
          // NOTE(Egor): set _cvtps_ (round to nearest)
          __m128i IntA = _mm_cvtps_epi32(BlendedA);
          __m128i IntR = _mm_cvtps_epi32(BlendedR);
          __m128i IntG = _mm_cvtps_epi32(BlendedG);
          __m128i IntB = _mm_cvtps_epi32(BlendedB);
          
          IntA = _mm_slli_epi32(IntA, 24);
          IntR = _mm_slli_epi32(IntR, 16);
          IntG = _mm_slli_epi32(IntG, 8);
          IntB = IntB;
          
          __m128i Out = _mm_or_si128(_mm_or_si128(IntR, IntG),
                                     _mm_or_si128(IntB, IntA));
          
          __m128i New = _mm_and_si128(WriteMask, Out);
          __m128i Old = _mm_andnot_si128(WriteMask, OriginalDest);
          __m128i MaskedOut = _mm_or_si128(New, Old);
          
          _mm_store_si128((__m128i *)Pixel, MaskedOut);
        }
        
        PixelPX = _mm_add_ps(PixelPX, Four);
        Pixel += 4;
        
        if((XI + 8) < FillRect.XMax) {
          
          // NOTE(Egor): we clipped first 4 pixels
          ClipMask = _mm_set1_epi8(-1);
        }
        else {
          
          ClipMask = EndClipMask;
        }
          
        IACA_VC64_END;
      }
      
      // NOTE(Egor): alternating lines 
      Row += RowAdvance;
      PixelPY = _mm_add_ps(PixelPY, Two);
    }
    
    int32 PixelCount = GetClampedArea(FillRect)/2;
    END_TIMED_BLOCK_COUNTED(ProcessPixel, PixelCount);
    
    END_TIMED_BLOCK(DrawRectangleSlowly);
  }
}


internal void
RenderPushBuffer(render_group *Group, loaded_bitmap *Output,
                 rectangle2i ClipRect, bool32 Even) {
  
  BEGIN_TIMED_BLOCK(RenderPushBuffer);
  
  real32 NullPtM = 1.0f;
  
  v2 ScreenDim = V2i(Output->Width, Output->Height);
//  real32 PtM = 1.0f / Group->MtP;
  
  // NOTE(Egor): render
  for(uint32 BaseAddress = 0; BaseAddress < Group->PushBufferSize;) {
    
    render_group_entry_header *Header = (render_group_entry_header *)(Group->PushBufferBase + BaseAddress);
    
    BaseAddress += sizeof(*Header);
    
    void *Data = (uint8 *)Header + sizeof(*Header);
    
    switch(Header->Type) {
      
      case RenderGroupEntryType_render_entry_clear: {
        
        render_entry_clear *Entry = (render_entry_clear *)Data;
        
        DrawRectangle(Output, V2(0.0f, 0.0f),
                      V2((real32)Output->Width, (real32)Output->Height),
                      Entry->Color, 
                      ClipRect, Even);
        
        BaseAddress += sizeof(*Entry);
        
      } break;
      case RenderGroupEntryType_render_entry_bitmap: {
        
        render_entry_bitmap *Entry = (render_entry_bitmap *)Data;
        BaseAddress += sizeof(*Entry);
        
        render_v2_basis Basis;
        Basis.Origin = Entry->P;
        Basis.XAxis = V2(1.0f, 0.0f);
        Basis.YAxis = Perp(Basis.XAxis);
        Basis.XAxis *= (real32)Entry->Size.x;
        Basis.YAxis *= (real32)Entry->Size.y;
        
        DrawRectangleSlowly(Output, Basis, Entry->Color, Entry->Bitmap, ClipRect, NullPtM, Even);
        
      } break;
      case RenderGroupEntryType_render_entry_rectangle: {
        
        render_entry_rectangle *Entry = (render_entry_rectangle *)Data;
        BaseAddress += sizeof(*Entry);
        
        if(Entry->Flag == 1) {
          int a = 3;
        }
        
        DrawRectangle(Output, Entry->P, Entry->P + Entry->Dim, Entry->Color, ClipRect, Even); 
      } break;
      
      InvalidDefaultCase;
    }
  }
  
  END_TIMED_BLOCK(RenderPushBuffer);
}

struct tile_render_work {
  
  render_group *Group;
  loaded_bitmap *Output;
  rectangle2i ClipRect;
};

internal 
PLATFORM_WORK_QUEUE_CALLBACK(DoTiledRenderingWork) {
  
  tile_render_work *Work = (tile_render_work *)Data;
  
  RenderPushBuffer(Work->Group, Work->Output, Work->ClipRect, false);
  RenderPushBuffer(Work->Group, Work->Output, Work->ClipRect, true);    
}


internal void
TiledRenderPushBuffer(platform_work_queue *RenderQueue, render_group *Group,
                      loaded_bitmap *Output) {
  
  bool32 Even = false;
  
#if 1
  int const TileCountX = 4;
  int const TileCountY = 4;
  
  tile_render_work WorkArray[TileCountX*TileCountY];
  
  Assert(((uintptr)Output->Memory & 0xF) == 0);
  
  int TileWidth = Output->Width / TileCountX;
  int TileHeight = Output->Height / TileCountY;
  TileWidth = ((TileWidth + 3) / 4)*4;
  
  uint32 WorkCount = 0;
  
  for(int32 TileY = 0; TileY < TileCountY; ++TileY) {
    for(int32 TileX = 0; TileX < TileCountX; ++TileX) {
      
      tile_render_work *Work = WorkArray + WorkCount++;
      
      rectangle2i ClipRect;
      
      ClipRect.XMin = TileX*TileWidth;
      ClipRect.XMax = ClipRect.XMin + TileWidth;
      ClipRect.YMin = TileY*TileHeight;
      ClipRect.YMax = ClipRect.YMin + TileHeight;
      
      if(ClipRect.XMax > Output->Width)
        ClipRect.XMax = Output->Width;
      
      Work->ClipRect = ClipRect;
      Work->Output = Output;
      Work->Group = Group;
      
#if 1
      PlatformAddEntry(RenderQueue, DoTiledRenderingWork, Work);
#else
      DoTiledRenderingWork(RenderQueue, Work);
#endif
    }
  }
  
  PlatformCompleteAllWork(RenderQueue);
#else
  
  rectangle2i ClipRect = {0, 0, Output->Width, Output->Height};
  RenderPushBuffer(Group, Output, ClipRect, false);
  RenderPushBuffer(Group, Output, ClipRect, true);    
  
#endif
}


inline void
Clear(render_group *Group, v4 Color) {
  
  render_entry_clear *Entry = PushRenderElement(Group, render_entry_clear);
  
  if(Entry) {
    
    Entry->Color = Color;
  }
}

inline void
PushCoordinateSystem(render_group *Group, v2 Origin, v2 XAxis, v2 YAxis, v4 Color,
                     loaded_bitmap *Bitmap, loaded_bitmap *NormalMap,
                     environment_map *Top, environment_map *Middle, environment_map *Bottom) {

  #if 0
  entity_basis_p_result BasisP = GetRenderEntityBasisP(&Group->Transform, ToV3(Origin, 0));
  
  if(Basis.Valid) {  
    
    render_entry_coordinate_system *Entry = PushRenderElement(Group, render_entry_coordinate_system);
    
    if(Entry) {
      
      Entry->EntityBasis.Basis = Group->DefaultBasis;
      Entry->Origin = Origin;
      Entry->XAxis = XAxis;
      Entry->YAxis = YAxis;
      Entry->Color = Color;
      Entry->Texture = Bitmap;
      Entry->NormalMap = NormalMap;
      Entry->Top = Top;
      Entry->Middle = Middle;
      Entry->Bottom = Bottom;
    }
  }
  #endif
}

inline v2
Unproject(render_group *Group, v2 ProjectedXY, real32 AtDistanceFromCamera) {
 
  v2 WorldXY = (AtDistanceFromCamera / Group->Transform.FocalLength)*ProjectedXY;
  return WorldXY;
}

inline rectangle2 
GetCameraRectangleAtDistance(render_group *Group, real32 DistanceFromCamera) {
  
  rectangle2 Result;
  v2 RawXY = Unproject(Group, Group->MonitorHalfDimInMeters, DistanceFromCamera);
  Result = RectCenterHalfDim(V2(0,0), RawXY);
  
  return Result;
}

inline rectangle2 
GetCameraRectangleAtTarget(render_group *Group) {
  
  rectangle2 Result = GetCameraRectangleAtDistance(Group, Group->Transform.CameraDistanceAboveGround);
  return Result;
}