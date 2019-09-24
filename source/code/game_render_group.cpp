
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
AllocateRenderGroup(memory_arena *Arena, uint32 MaxPushBufferSize, v2 ResolutionPixels) {
  
  render_group *Result = PushStruct(Arena, render_group);
  Result->PushBufferBase = (uint8 *)PushSize(Arena, MaxPushBufferSize);
  Result->PushBufferSize = 0;
  Result->MaxPushBufferSize = MaxPushBufferSize;
  
  Result->DefaultBasis = PushStruct(Arena, render_basis);
  Result->DefaultBasis->P = V3(0.0f, 0.0f, 0.0f);
  Result->GlobalAlpha = 1.0f;

  // NOTE(Egor): camera parameteres
  // TODO(Egor): values looks wrong
  Result->GameCamera.FocalLength = 0.6f;
  Result->GameCamera.CameraDistanceAboveGround = 9.0f;
  Result->GameCamera.NearClipPlane = 0.2f;
  
  Result->RenderCamera = Result->GameCamera;
//  Result->RenderCamera.CameraDistanceAboveGround = 35.0f;
  
  // NOTE(Egor): monitor properties 0.635 m -- is average length of the monitor
  real32 WidthOfMonitor = 0.635f;
  Result->MtP = ResolutionPixels.x*WidthOfMonitor;
  real32 PtM = 1.0f / Result->MtP;
  Result->MonitorHalfDimInMeters = 0.5f*PtM*ResolutionPixels;
  
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
  
  
  render_entry_bitmap *Piece = PushRenderElement(Group, render_entry_bitmap);
  if(Piece) {
    
    // NOTE(Egor): this is actual width and height in meters
    v2 Size = V2(Height * Bitmap->WidthOverHeight, Height);
    v2 Align = Hadamard(Bitmap->AlignPercentage, Size);

    Piece->Size = Size;
    Piece->EntityBasis.Basis = Group->DefaultBasis;
    Piece->EntityBasis.Offset = Offset - ToV3(Align, 0);
    Piece->Bitmap = Bitmap;
    Piece->Color = Group->GlobalAlpha*Color;
    //Piece->Color.a *= Group->GlobalAlpha;
  }
}

internal void 
PushRect(render_group *Group, v3 Offset, v2 Dim, v4 Color) {
  
  render_entry_rectangle *Piece = PushRenderElement(Group, render_entry_rectangle);
  if(Piece) {
    
    render_entity_basis EntityBasis;
    EntityBasis.Basis = Group->DefaultBasis;
    EntityBasis.Offset = (Offset - ToV3(0.5f*Dim, 0));
    
    Piece->EntityBasis = EntityBasis;
    Piece->Dim = Dim;
    Piece->Color = Color;
    
    Assert(Piece->EntityBasis.Basis);
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
DrawRectangle(loaded_bitmap *Buffer, v2 vMin, v2 vMax, v4 Color)
{
  
  int32 MinX = RoundReal32ToInt32(vMin.x);
  int32 MinY = RoundReal32ToInt32(vMin.y);
  int32 MaxX = RoundReal32ToInt32(vMax.x);
  int32 MaxY = RoundReal32ToInt32(vMax.y);
  
  // clipping value to actual size of the backbuffer
  if(MinX < 0) MinX = 0;
  if(MinY < 0) MinY = 0;
  // we will write up to, but not including to final row and column
  if(MaxX > Buffer->Width) MaxX = Buffer->Width;
  if(MaxY > Buffer->Height) MaxY = Buffer->Height;
  
  uint32 PixColor = ((RoundReal32ToUInt32(Color.a * 255.0f) << 24) |
                     (RoundReal32ToUInt32(Color.r * 255.0f) << 16) |
                     (RoundReal32ToUInt32(Color.g * 255.0f) << 8)  |
                     (RoundReal32ToUInt32(Color.b * 255.0f) << 0));
  
  
  uint8 *EndOfBuffer = (uint8 *)Buffer->Memory + Buffer->Pitch*Buffer->Height;
  
  // go to line to draw
  uint8 *Row = ((uint8 *)Buffer->Memory + MinY*Buffer->Pitch + MinX*BITMAP_BYTES_PER_PIXEL);
  // TODO: color
  for(int Y = MinY; Y < MaxY; ++Y)
  {
    uint32 *Pixel = (uint32 *)Row;
    for(int X = MinX; X < MaxX; ++X)
    {
      *Pixel++ = PixColor;
    }
    Row += Buffer->Pitch;
  }
}


struct entity_basis_p_result {
  
  v2 P;
  real32 Scale;
  bool32 Valid;
};

inline entity_basis_p_result
GetRenderEntityBasisP(render_group *Group, render_entity_basis *EntityBasis, v2 ScreenDim) {
  
  v2 ScreenCenter = 0.5f*ScreenDim;
  entity_basis_p_result Result = {};
  
  v3 EntityBaseP = EntityBasis->Basis->P;

  real32 FocalLength = Group->RenderCamera.FocalLength;
  real32 CameraDistanceAboveGround = Group->RenderCamera.CameraDistanceAboveGround;
  real32 NearClipPlane = Group->RenderCamera.NearClipPlane;
  
  real32 DistanceToPz = (CameraDistanceAboveGround - EntityBaseP.z);
  v3 RawXY = ToV3(EntityBaseP.xy + EntityBasis->Offset.xy, 1.0f);
  
  if(DistanceToPz > NearClipPlane) {
    
    v3 ProjectedXY = (1.0f/DistanceToPz) * RawXY*FocalLength;

    Result.P = ScreenCenter + Group->MtP*ProjectedXY.xy;
    Result.Scale = Group->MtP*ProjectedXY.z;
    Result.Valid = true;
  }
  
  return Result;
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



#if 0

internal void
DrawRectangleSlowly(loaded_bitmap *Buffer, render_v2_basis Basis, v4 Color,
                    loaded_bitmap *Texture, loaded_bitmap *NormalMap,
                    environment_map *Top, environment_map *Middle, environment_map *Bottom,
                    real32 PtM) {
  
  BEGIN_TIMED_BLOCK(DrawRectangleSlowly);
  
  real32 XAxisLength = Length(Basis.XAxis);
  real32 YAxisLength = Length(Basis.YAxis);
  
  v2 NXAxis = YAxisLength/XAxisLength * Basis.XAxis;
  v2 NYAxis = XAxisLength/YAxisLength * Basis.YAxis;
  
  real32 NZScale = (XAxisLength + YAxisLength)*0.5f;
  
  //NOTE(Egor): premultiply color
  Color.rgb *= Color.a;
  
  int32 Width = Buffer->Width - 1;
  int32 Height = Buffer->Height - 1;
  real32 InvWidth = 1.0f/(real32)Width;
  real32 InvHeight = 1.0f/(real32)Height;
  
  real32 OriginZ = 0.0f;
  real32 OriginY = (Basis.Origin + 0.5f*Basis.YAxis + 0.5f*Basis.XAxis).y;
  real32 FixedCastY = InvHeight*OriginY;
  
  int32 YMin = Height;
  int32 XMin = Width;
  int32 XMax = 0;
  int32 YMax = 0;
  
  v2 Min = Basis.Origin;
  v2 Max = Basis.Origin + Basis.XAxis + Basis.YAxis;
  v2 toAxisX = Min + Basis.XAxis;
  v2 toAxisY = Min + Basis.YAxis;
  
  real32 InvXAxisLengthSq = 1.0f/LengthSq(Basis.XAxis);
  real32 InvYAxisLengthSq = 1.0f/LengthSq(Basis.YAxis);
  
  
  v2 P[4] = {Min, Min + Basis.XAxis, Min + Basis.YAxis, Max};
  
  for(uint32 Index = 0; Index < ArrayCount(P); ++Index) {
    
    v2 *TestP = P + Index;
    
    int32 CeilX = CeilReal32ToInt32(TestP->x);
    int32 FloorX = FloorReal32ToInt32(TestP->x);
    int32 CeilY = CeilReal32ToInt32(TestP->y);
    int32 FloorY = FloorReal32ToInt32(TestP->y);
    
    if(XMin > FloorX) XMin = FloorX;
    if(YMin > FloorY) YMin = FloorY;
    if(XMax < CeilX) XMax = CeilX;
    if(YMax < CeilY) YMax = CeilY;
  }
  
  if(XMin < 0) XMin = 0;
  if(YMin < 0) YMin = 0;
  if(XMax > Width) XMax = Width;
  if(YMax > Height) YMax = Height;
  
  uint32 PixColor = ((RoundReal32ToUInt32(Color.a * 255.0f) << 24) |
                     (RoundReal32ToUInt32(Color.r * 255.0f) << 16) |
                     (RoundReal32ToUInt32(Color.g * 255.0f) << 8)  |
                     (RoundReal32ToUInt32(Color.b * 255.0f) << 0));
  
  uint8 *Row = (uint8 *)Buffer->Memory + YMin*Buffer->Pitch + XMin*BITMAP_BYTES_PER_PIXEL;
  
  BEGIN_TIMED_BLOCK(ProcessPixel);
  
  for(int32 Y = YMin; Y <= YMax; ++Y) {
    
    uint32 *Pixel = (uint32 *)Row;
    for(int32 X = XMin; X <= XMax; ++X) {
      
      BEGIN_TIMED_BLOCK(TestPixel);
      
      v2 PixelP = V2i(X, Y);
      v2 d = PixelP - Basis.Origin;
      
      
      real32 Edge0 = Inner(d - Basis.XAxis - Basis.YAxis, Basis.XAxis);
      real32 Edge1 = Inner(d - Basis.XAxis - Basis.YAxis, Basis.YAxis); 
      real32 Edge2 = Inner(d, -Basis.XAxis); 
      real32 Edge3 = Inner(d, -Basis.YAxis); 
      
      if(Edge0 < 0 &&
         Edge1 < 0 &&
         Edge2 < 0 &&
         Edge3 < 0) {
        
        v2 ScreenSpaceUV = V2((real32)X*InvWidth, FixedCastY);
        
        real32 ZDiff = PtM*((real32)Y - OriginY);
        
        v2 UV =  V2(InvXAxisLengthSq * Inner(Basis.XAxis,d),
                    InvYAxisLengthSq * Inner(Basis.YAxis,d));
        
        // TODO(Egor): clamp this later
        //Assert(UV.x >= 0 && UV.x <= 1.0f);
        //Assert(UV.y >= 0 && UV.y <= 1.0f);
        
        real32 tX = (UV.x * (real32)(Texture->Width - 2));
        real32 tY = (UV.y * (real32)(Texture->Height - 2));
        
        int32 PixelX = (int32)tX;
        int32 PixelY = (int32)tY;
        
        // NOTE(Egor): take the fractional part of tX and tY
        real32 fX = tX - (real32)PixelX;
        real32 fY = tY - (real32)PixelY;
        
        Assert(PixelX >= 0 && PixelX <= (Texture->Width - 1));
        Assert(PixelY >= 0 && PixelY <= (Texture->Height - 1));
        
        bilinear_sample TexelSample = BilinearSample(Texture, PixelX, PixelY);
        v4 Texel = SRGBBilinearBlend(TexelSample, fX, fY);
        
        // NOTE(Egor): color tinting and external opacity
        Texel = Hadamard(Texel, Color);        
        
        Texel.r = Clamp01(Texel.r);
        Texel.g = Clamp01(Texel.g);
        Texel.b = Clamp01(Texel.b);
        
        v4 Dest = Unpack4x8(*Pixel);
        Dest = SRGB255ToLinear1(Dest);
        // NOTE(Egor): alpha channel for compisited bitmaps in premultiplied alpha mode
        // for case when we create intermediate buffer with two or more bitmaps blend with 
        // each other 
        // (Texel.a + Dest.a - Texel.a*Dest.a))
        v4 Blended = (1.0f - Texel.a)*Dest + Texel;
        v4 Blended255 = Linear1ToSRGB255(Blended);
        
        *Pixel = (((uint32)(Blended255.a + 0.5f) << 24) |
                  ((uint32)(Blended255.r + 0.5f) << 16) |
                  ((uint32)(Blended255.g + 0.5f) << 8)  |
                  ((uint32)(Blended255.b + 0.5f) << 0));
        
      }
      
      Pixel++;
    }
    
    Row += Buffer->Pitch;
  }
  
  END_TIMED_BLOCK_COUNTED(ProcessPixel, (XMax - XMin + 1)*(YMax - YMin + 1));
  END_TIMED_BLOCK(DrawRectangleSlowly);
}


#else

struct OpCounts {
  
  uint32 mm_add_ps; 
  uint32 mm_sub_ps; 
  uint32 mm_mul_ps; 
  uint32 mm_and_ps; 
  uint32 mm_and_si128; 
  uint32 mm_or_ps; 
  uint32 mm_or_si128; 
  uint32 mm_andnot_si128;
  uint32 mm_castps_si128; 
  uint32 mm_cvtepi32_ps; 
  uint32 mm_cvttps_epi32;
  uint32 mm_cvtps_epi32; 
  uint32 mm_cmple_ps; 
  uint32 mm_cmpge_ps; 
  uint32 mm_max_ps; 
  uint32 mm_min_ps; 
  uint32 mm_slli_epi32; 
  uint32 mm_srli_epi32; 
  uint32 mm_sqrt_ps; 
};


#if 1
#include "iacaMarks.h"
#else

#define IACA_VC64_START
#define IACA_VC64_END

#endif


internal void
DrawRectangleSlowly(loaded_bitmap *Buffer, render_v2_basis Basis, v4 Color,
                    loaded_bitmap *Texture, loaded_bitmap *NormalMap,
                    environment_map *Top, environment_map *Middle, environment_map *Bottom,
                    real32 PtM) {
  
  BEGIN_TIMED_BLOCK(DrawRectangleSlowly);
  
  real32 XAxisLength = Length(Basis.XAxis);
  real32 YAxisLength = Length(Basis.YAxis);
  
  v2 NXAxis = YAxisLength/XAxisLength * Basis.XAxis;
  v2 NYAxis = XAxisLength/YAxisLength * Basis.YAxis;
  
  real32 NZScale = (XAxisLength + YAxisLength)*0.5f;
  
  //NOTE(Egor): premultiply color
  Color.rgb *= Color.a;
  
  // NOTE(Egor): Clip buffers to not overwrite on the next line
  int32 Width = Buffer->Width - 1 - 3;
  int32 Height = Buffer->Height - 1 - 3;
  real32 InvWidth = 1.0f/(real32)Width;
  real32 InvHeight = 1.0f/(real32)Height;
  
  real32 OriginZ = 0.0f;
  real32 OriginY = (Basis.Origin + 0.5f*Basis.YAxis + 0.5f*Basis.XAxis).y;
  real32 FixedCastY = InvHeight*OriginY;
  
  int32 YMin = Height;
  int32 XMin = Width;
  int32 XMax = 0;
  int32 YMax = 0;
  
  v2 Min = Basis.Origin;
  v2 Max = Basis.Origin + Basis.XAxis + Basis.YAxis;
  v2 toAxisX = Min + Basis.XAxis;
  v2 toAxisY = Min + Basis.YAxis;
  
  real32 InvXAxisLengthSq = 1.0f/LengthSq(Basis.XAxis);
  real32 InvYAxisLengthSq = 1.0f/LengthSq(Basis.YAxis);
  
  
  v2 P[4] = {Min, Min + Basis.XAxis, Min + Basis.YAxis, Max};
  
  for(uint32 Index = 0; Index < ArrayCount(P); ++Index) {
    
    v2 *TestP = P + Index;
    
    int32 CeilX = CeilReal32ToInt32(TestP->x);
    int32 FloorX = FloorReal32ToInt32(TestP->x);
    int32 CeilY = CeilReal32ToInt32(TestP->y);
    int32 FloorY = FloorReal32ToInt32(TestP->y);
    
    if(XMin > FloorX) XMin = FloorX;
    if(YMin > FloorY) YMin = FloorY;
    if(XMax < CeilX) XMax = CeilX;
    if(YMax < CeilY) YMax = CeilY;
  }
  
  if(XMin < 0) XMin = 0;
  if(YMin < 0) YMin = 0;
  if(XMax > Width) XMax = Width;
  if(YMax > Height) YMax = Height;
  
  uint8 *Row = (uint8 *)Buffer->Memory + YMin*Buffer->Pitch + XMin*BITMAP_BYTES_PER_PIXEL;
  
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
  __m128 Zero = _mm_set1_ps(0.0f);
  __m128 Four = _mm_set1_ps(4.0f);
  __m128 OneHalf_4x = _mm_set1_ps(0.5f);
  __m128i Mask1FF = _mm_set1_epi32(0x1FF);
  __m128i Mask10000 = _mm_set1_epi32(0x10000);
  __m128i MaskFF = _mm_set1_epi32(0xFF);
  __m128i MaskFFFF = _mm_set1_epi32(0xFFFF);
  __m128i MaskFF00FF = _mm_set1_epi32(0x00FF00FF);

  __m128 Inv255_4x = _mm_set1_ps(Inv255);
  
  __m128 One255_4x = _mm_set1_ps(255.0f);
  
  __m128 Squared255 = _mm_set1_ps(255.0f*255.0f);
  
  __m128 OriginX_4x = _mm_set1_ps(Basis.Origin.x);
  __m128 OriginY_4x = _mm_set1_ps(Basis.Origin.y);
  
  __m128 WidthM2_4x = _mm_set1_ps((real32)(Texture->Width - 2));
  __m128 HeightM2_4x = _mm_set1_ps((real32)(Texture->Height - 2));
  
  __m128 Add3210_m_OriginX = _mm_set_ps(3.0f, 2.0f, 1.0f, 0.0f);
  Add3210_m_OriginX = _mm_sub_ps(Add3210_m_OriginX, OriginX_4x);
  
  __m128 XMin_4x = _mm_set1_ps((real32)XMin);
  XMin_4x = _mm_add_ps(XMin_4x, Add3210_m_OriginX);
  
#define M(a, I) ((real32 *)&(a))[I]
#define Mi(a, I) ((uint32 *)&(a))[I]
#define mm_square(a) _mm_mul_ps(a, a)

  
  
  BEGIN_TIMED_BLOCK(ProcessPixel);

  
  // NOTE(Egor): this routine works with _ON_SCREEN_ pixels, and correspond them with
  // Texels inside Actual Texture that drawn to that part of _SCREEN_
  
  // NOTE(Egor): Get quad (Y-OriginY) pixel
  __m128 PixelPY = _mm_set1_ps((real32)YMin);
  PixelPY = _mm_sub_ps(PixelPY, OriginY_4x);
  
  for(int32 Y = YMin; Y <= YMax; ++Y) {

    // NOTE(Egor): relative position of actual (ON_SCREEN) pixel inside texture
    // --> we find the vector, that point to a pixel inside a texture basis
    // NOTE(Egor): Get all 4 Pixels (X-OriginX)+3 (X-OriginX)+2 (X-OriginX)+1 (X-OriginX)+0
    // NOTE(Egor): Add3210_m_OriginX has baked in OriginX_4x
    // NOTE(Egor): This is when we reset PixelPX betwen Y - runs
    __m128 PixelPX = XMin_4x;
    
#define TEST_PixelPY_x_NXAxisY 1 // NOTE(Egor): looks like it's faster ¯\_(-_-)_/¯ 
    
#if TEST_PixelPY_x_NXAxisY
    // NOTE(Egor): only changes between Y - runs
    __m128 PixelPY_x_NXAxisY = _mm_mul_ps(nXAxisY_4x, PixelPY);
    __m128 PixelPY_x_NYAxisY = _mm_mul_ps(nYAxisY_4x, PixelPY);
#endif
    
    uint32 *Pixel = (uint32 *)Row;
    for(int32 XI = XMin; XI <= XMax; XI += 4) {
      
      
#if 0 // NOTE(Egor): disable gamma correction
      
      //#undef mm_square
      //#define mm_square(a) a;
      //#define _mm_sqrt_ps(a) a
#endif
      
      IACA_VC64_START;
      
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
      // TODO(Egor): check later if it helps
      //      if(_mm_movemask_epi8(WriteMask))
      {
        
        __m128i OriginalDest = _mm_loadu_si128((__m128i *)Pixel);
        
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
        
        __m128i SampleA;
        __m128i SampleB;
        __m128i SampleC;
        __m128i SampleD;
        

        
        // NOTE(Egor): fetch 4 texels from texture buffer
        for(int32 I = 0; I < 4; ++I) {
          
          int32 FetchX = Mi(FetchX_4x, I);
          int32 FetchY = Mi(FetchY_4x, I);
          
          // NOTE(Egor): don't need that when we clamp, USE AS NEEDED, DO NOT DELETE
          //          Assert(FetchX >= 0 && FetchX <= (Texture->Width - 1));
          //          Assert(FetchY >= 0 && FetchY <= (Texture->Height - 1));
          
          uint8 *TexelPtr = (((uint8 *)Texture->Memory) +
                             FetchY*Texture->Pitch + FetchX*BITMAP_BYTES_PER_PIXEL);
          Mi(SampleA, I) = *(uint32 *)(TexelPtr);
          Mi(SampleB, I) = *(uint32 *)(TexelPtr + sizeof(uint32));
          Mi(SampleC, I) = *(uint32 *)(TexelPtr + Texture->Pitch);
          Mi(SampleD, I) = *(uint32 *)(TexelPtr + Texture->Pitch + sizeof(uint32));
        }
        
        
        //////
        __m128i TexelArb = _mm_and_si128(SampleA, MaskFF00FF);
        __m128i TexelAag = _mm_and_si128(_mm_srli_epi32(SampleA,  8), MaskFF00FF);
        TexelArb = _mm_mullo_epi16(TexelArb, TexelArb);
        
        __m128 TexelAa = _mm_cvtepi32_ps(_mm_srli_epi32(TexelAag, 16));
        TexelAag = _mm_mullo_epi16(TexelAag, TexelAag);
        
        //////
        __m128i TexelBrb = _mm_and_si128(SampleB, MaskFF00FF);
        __m128i TexelBag = _mm_and_si128(_mm_srli_epi32(SampleB,  8), MaskFF00FF);
        TexelBrb = _mm_mullo_epi16(TexelBrb, TexelBrb);
        
        __m128 TexelBa = _mm_cvtepi32_ps(_mm_srli_epi32(TexelBag, 16));
        TexelBag = _mm_mullo_epi16(TexelBag, TexelBag);
        
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
        
        // NOTE(Egor): load destination color
        __m128i Destrb = _mm_and_si128(OriginalDest, MaskFF00FF);
        __m128i Destag = _mm_and_si128(_mm_srli_epi32(OriginalDest,  8), MaskFF00FF);
        Destrb = _mm_mullo_epi16(Destrb, Destrb);
        
        __m128 DestA = _mm_cvtepi32_ps(_mm_srli_epi32(Destag, 16));
        Destag = _mm_mullo_epi16(Destag, Destag);
        
        // NOTE(Egor): Convert texture from SRGB to 'linear' brightness space

        __m128 TexelAr = _mm_cvtepi32_ps(_mm_srli_epi32(TexelArb, 16));
        __m128 TexelAg = _mm_cvtepi32_ps(_mm_and_si128(TexelAag, MaskFFFF));
        __m128 TexelAb = _mm_cvtepi32_ps(_mm_and_si128(TexelArb, MaskFFFF));
        

        __m128 TexelBr = _mm_cvtepi32_ps(_mm_srli_epi32(TexelBrb, 16));
        __m128 TexelBg = _mm_cvtepi32_ps(_mm_and_si128(TexelBag, MaskFFFF));
        __m128 TexelBb = _mm_cvtepi32_ps(_mm_and_si128(TexelBrb, MaskFFFF));
        

        __m128 TexelCr = _mm_cvtepi32_ps(_mm_srli_epi32(TexelCrb, 16));
        __m128 TexelCg = _mm_cvtepi32_ps(_mm_and_si128(TexelCag, MaskFFFF));
        __m128 TexelCb = _mm_cvtepi32_ps(_mm_and_si128(TexelCrb, MaskFFFF));


        __m128 TexelDr = _mm_cvtepi32_ps(_mm_srli_epi32(TexelDrb, 16));
        __m128 TexelDg = _mm_cvtepi32_ps(_mm_and_si128(TexelDag, MaskFFFF));
        __m128 TexelDb = _mm_cvtepi32_ps(_mm_and_si128(TexelDrb, MaskFFFF));
        
        // NOTE(Egor): convert to linear brightness space
        // Dest = SRGB255ToLinear1(Dest);

        __m128 DestR = _mm_cvtepi32_ps(_mm_srli_epi32(Destrb, 16));
        __m128 DestG = _mm_cvtepi32_ps(_mm_and_si128(Destag, MaskFFFF));
        __m128 DestB = _mm_cvtepi32_ps(_mm_and_si128(Destrb, MaskFFFF));
        
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
        BlendedR = _mm_sqrt_ps(BlendedR);
        BlendedG = _mm_sqrt_ps(BlendedG);
        BlendedB = _mm_sqrt_ps(BlendedB);
        BlendedA = _mm_mul_ps(One255_4x, BlendedA);
        
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
        
        IACA_VC64_END;
      }
      
      
      PixelPX = _mm_add_ps(PixelPX, Four);
      Pixel += 4;
    }
    
    Row += Buffer->Pitch;
    PixelPY = _mm_add_ps(PixelPY, One);
  }
  

  END_TIMED_BLOCK_COUNTED(ProcessPixel, (XMax - XMin + 1)*(YMax - YMin + 1));
  
  
  END_TIMED_BLOCK(DrawRectangleSlowly);
}

#endif

internal void
RenderPushBuffer(render_group *Group, loaded_bitmap *Output) {
  
  BEGIN_TIMED_BLOCK(RenderPushBuffer);
  
  v2 ScreenDim = V2i(Output->Width, Output->Height);
  real32 PtM = 1.0f / Group->MtP;
  
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
                      Entry->Color);
        
        BaseAddress += sizeof(*Entry);
        
      } break;
      case RenderGroupEntryType_render_entry_bitmap: {
        
        render_entry_bitmap *Entry = (render_entry_bitmap *)Data;
        BaseAddress += sizeof(*Entry);
        
        entity_basis_p_result BasisP = GetRenderEntityBasisP(Group, &Entry->EntityBasis, ScreenDim);
        Assert(Entry->Bitmap);
#if 0        

        DrawBitmap(Output, Entry->Bitmap, P.x, P.y, 1.0f);
#else
        
        render_v2_basis Basis;
        Basis.Origin = BasisP.P;
        Basis.XAxis = V2(1.0f, 0.0f);
        Basis.YAxis = Perp(Basis.XAxis);
        Basis.XAxis *= BasisP.Scale*(real32)Entry->Size.x;
        Basis.YAxis *= BasisP.Scale*(real32)Entry->Size.y;
        
        DrawRectangleSlowly(Output, Basis, Entry->Color,Entry->Bitmap, 
                            0, 0, 0, 0, PtM);
#endif
        
      } break;
      case RenderGroupEntryType_render_entry_rectangle: {
        
        
        render_entry_rectangle *Entry = (render_entry_rectangle *)Data;
        BaseAddress += sizeof(*Entry);
        
        entity_basis_p_result BasisP = GetRenderEntityBasisP(Group, &Entry->EntityBasis, ScreenDim);
        DrawRectangle(Output, BasisP.P, BasisP.P + BasisP.Scale*Entry->Dim, Entry->Color); 
      } break;
      case RenderGroupEntryType_render_entry_coordinate_system: {
        
        
        render_entry_coordinate_system *Entry = (render_entry_coordinate_system *)Data;
        BaseAddress += sizeof(*Entry);
        
        v2 DimOrigin = V2(2, 2);
        
        v2 P = Entry->Origin;
        DrawRectangle(Output, P - DimOrigin, P + DimOrigin, Entry->Color); 
        
        P = Entry->Origin + Entry->XAxis;
        DrawRectangle(Output, P - DimOrigin, P + DimOrigin, Entry->Color); 
        
        P = Entry->Origin + Entry->YAxis;
        DrawRectangle(Output, P - DimOrigin, P + DimOrigin, Entry->Color); 
        
        
        P = Entry->Origin + Entry->YAxis + Entry->XAxis; 
        DrawRectangle(Output, P - DimOrigin, P + DimOrigin, Entry->Color); 
        
        render_v2_basis V2Basis;
        V2Basis.Origin = Entry->Origin;
        V2Basis.XAxis = Entry->XAxis;
        V2Basis.YAxis = Entry->YAxis;
        
#if 1
        DrawRectangleSlowly(Output, V2Basis, Entry->Color,
                            Entry->Texture, Entry->NormalMap,
                            Entry->Top, Entry->Middle, Entry->Bottom,
                            PtM);
#endif
        
#if 0
        for(uint32 I = 0; I < ArrayCount(Entry->Points); ++I) {
          
          v2 Point = Entry->Points[I];
          
          P = Entry->Origin + Point.x*Entry->XAxis + Point.y*Entry->YAxis; 
          
          DrawRectangle(Output, P - DimOrigin, P + DimOrigin, Entry->Color); 
        }
#endif
      } break;
      
      InvalidDefaultCase;
    }
  }
  
  END_TIMED_BLOCK(RenderPushBuffer);
}


inline void
Clear(render_group *Group, v4 Color) {
  
  render_entry_clear *Entry = PushRenderElement(Group, render_entry_clear);
  
  if(Entry) {
    
    Entry->Color = Color;
  }
}

inline render_entry_coordinate_system *
PushCoordinateSystem(render_group *Group, v2 Origin, v2 XAxis, v2 YAxis, v4 Color,
                     loaded_bitmap *Bitmap, loaded_bitmap *NormalMap,
                     environment_map *Top, environment_map *Middle, environment_map *Bottom) {
  
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
  
  return Entry;
}

inline v2
Unproject(render_group *Group, v2 ProjectedXY, real32 AtDistanceFromCamera) {
 
  v2 WorldXY = (AtDistanceFromCamera / Group->GameCamera.FocalLength)*ProjectedXY;
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
  
  rectangle2 Result = GetCameraRectangleAtDistance(Group, Group->GameCamera.CameraDistanceAboveGround);
  return Result;
}