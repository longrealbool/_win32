
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
AllocateRenderGroup(memory_arena *Arena, uint32 MaxPushBufferSize, real32 MtP) {
  
  render_group *Result = PushStruct(Arena, render_group);
  Result->PushBufferBase = (uint8 *)PushSize(Arena, MaxPushBufferSize);
  Result->PushBufferSize = 0;
  Result->MaxPushBufferSize = MaxPushBufferSize;
  
  Result->DefaultBasis = PushStruct(Arena, render_basis);
  Result->DefaultBasis->P = V3(0.0f, 0.0f, 0.0f);
  Result->MtP = MtP;
  
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
PushBitmap(render_group *Group, loaded_bitmap *Bitmap, v3 Offset,
           v4 Color = V4(1,1,1,1)) {
  
  
  render_entry_bitmap *Piece = PushRenderElement(Group, render_entry_bitmap);
  if(Piece) {
    
    v2 Align = Bitmap->Align;
    
    render_entity_basis EntityBasis;
    EntityBasis.Basis = Group->DefaultBasis;
    EntityBasis.Offset = Group->MtP*Offset - V3(Align, 0);
    
    Piece->EntityBasis = EntityBasis;
    Piece->Bitmap = Bitmap;
    Piece->Color = Color;
  }
}

internal void 
PushRect(render_group *Group, v3 Offset, v2 Dim, v4 Color) {
  
  render_entry_rectangle *Piece = PushRenderElement(Group, render_entry_rectangle);
  if(Piece) {
    
    render_entity_basis EntityBasis;
    EntityBasis.Basis = Group->DefaultBasis;
    EntityBasis.Offset = Group->MtP*(Offset - V3(0.5f*Dim, 0));
    
    Piece->EntityBasis = EntityBasis;
    Piece->Dim = Dim*Group->MtP;
    Piece->Color = Color;
    
    Assert(Piece->EntityBasis.Basis);
  }
}

internal void 
PushRectOutline(render_group *Group, v3 Offset, v2 Dim, v4 Color) {
  
  real32 Thickness = 0.05f;
  
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
};

inline entity_basis_p_result
GetRenderEntityBasisP(render_group *Group, render_entity_basis *EntityBasis, v2 ScreenCenter) {
  
  entity_basis_p_result Result;
  // TODO(Egor): ZHANDLING
  real32 MtP = Group->MtP;
  
  v3 EntityBaseP = MtP*EntityBasis->Basis->P;
  real32 ZFudge = (1.0f + 0.01f*(EntityBaseP.z));
  
  v2 EntityGroundPoint = ScreenCenter + EntityBaseP.xy*ZFudge + EntityBasis->Offset.xy;
  Result.P = EntityGroundPoint + V2(0, EntityBaseP.z + EntityBasis->Offset.z);
  Result.Scale = ZFudge;
  
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
  
  uint8 *TexelPtr = ((uint8 *)Texture->Memory)
    + Y*Texture->Pitch + X*BITMAP_BYTES_PER_PIXEL;
  
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




internal void
DrawRectangleSlowly(loaded_bitmap *Buffer, render_v2_basis Basis, v4 Color,
                    loaded_bitmap *Texture, loaded_bitmap *NormalMap,
                    environment_map *Top, environment_map *Middle, environment_map *Bottom,
                    real32 PtM) {
  
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
  
  for(int32 Y = YMin; Y <= YMax; ++Y) {
    
    uint32 *Pixel = (uint32 *)Row;
    for(int32 X = XMin; X <= XMax; ++X) {
      
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
        
        if(NormalMap) {
          
          bilinear_sample NormalSample = BilinearSample(NormalMap, PixelX, PixelY);
          
          v4 NormalA  = Unpack4x8(NormalSample.A);
          v4 NormalB  = Unpack4x8(NormalSample.B);
          v4 NormalC  = Unpack4x8(NormalSample.C);
          v4 NormalD  = Unpack4x8(NormalSample.D);
          
          v4 Normal = Lerp(Lerp(NormalA, fX, NormalB),
                           fY,
                           Lerp(NormalC, fX, NormalD));
          
          Normal = UnscaleAndBiasNormal(Normal);
          
          Normal.xy = NXAxis*Normal.x + NYAxis*Normal.y;
          Normal.z *= NZScale;
          Normal.xyz = Normalize(Normal.xyz);
          
          // NOTE(Egor): this vector is pointing straing out of monitor
          // EyeVector = (0, 0, 1) 
          // NOTE(Egor): simplified version of <EyeVector, NormalVector>*NormalVecor*2 - EyeVector
          // TODO(Egor): reconstruct the math, to have The EYE casting EYE vector,
          // not the positioning vector of EYE
          
          v3 BounceDirection = 2.0f*Normal.z*Normal.xyz;
          BounceDirection.z -= 1.0f;
          BounceDirection.z = -BounceDirection.z;
#if 1
          
          environment_map *FarMap = 0;
          real32 Pz = OriginZ + ZDiff;
          real32 MapZ = 2.0f;          
          real32 tFarMap = 0.0f;
          real32 tEnvMap = BounceDirection.y;
          if(tEnvMap < -0.5f) {
            
            FarMap = Bottom;
            tFarMap = -1.0f - tEnvMap*2.0f;
          }
          else if(tEnvMap > 0.5f) {
            
            FarMap = Top;
            tFarMap = tEnvMap*2.0f - 1.0f;
          }
          
          tFarMap *= tFarMap;
          tFarMap *= tFarMap;
          
          v3 LightColor = V3(0, 0, 0); //SampleEnvironmentMap(ScreenSpaceUV, Normal.xYZ, Normal.w, Middle);
          if(FarMap) {
            
            real32 DistanceFromMapInZ = FarMap->Pz - Pz;
            v3 FarMapColor = SampleEnvironmentMap(ScreenSpaceUV, BounceDirection,
                                                  Normal.w, FarMap, DistanceFromMapInZ);
            LightColor = Lerp(LightColor, tFarMap, FarMapColor);
          }
          
          Texel.rgb = Texel.rgb + LightColor*Texel.a;
          
#if 0
          Texel.rgb = V3(0.5f, 0.5f, 0.5f) + 0.5f*BounceDirection;
          Texel.rgb *= Texel.a;
#endif
          
#else
          
          /*real32 IsoLine = -0.9f;
          
          if(BounceDirection.y >= IsoLine - 0.05f &&
             BounceDirection.y <= IsoLine + 0.05f) {
             
            Texel.rgb = V3(1, 1, 1); 
          }
          else {
          
            Texel.rgb = V3(0, 0, 0);
          }*/
          
          /*
          Texel.rgb = V3(0.5f, 0.5f, 0.5f) + 0.5f*BounceDirection;
          Texel.r = 0.0f;
          Texel.b = 0.0f;
          Texel.a = 1.0f;
          */
          
          /*
          Texel.rgb = V3(0.5f, 0.5f, 0.5f) + 0.5f*Normal.rgb;
          Texel.a = 1.0f;
          */
#endif
          
          
          // NOTE(Egor): end of normal processing
        }
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
}



internal void
RenderPushBuffer(render_group *Group, loaded_bitmap *Output) {
  
  real32 MtP = Group->MtP; // NOTE(Egor): meters to pixels
  real32 PtM = 1.0f/Group->MtP; // NOTE(Egor): pixel to meters
  
  v2 ScreenCenter = V2i(Output->Width, Output->Height)*0.5f;
  
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
        
        entity_basis_p_result BasisP = GetRenderEntityBasisP(Group, &Entry->EntityBasis, ScreenCenter);
        Assert(Entry->Bitmap);
#if 0        

        DrawBitmap(Output, Entry->Bitmap, P.x, P.y, 1.0f);
#else
        
        render_v2_basis Basis;
        Basis.Origin = BasisP.P;
        Basis.XAxis = V2(1.0f, 0.0f);
        Basis.YAxis = Perp(Basis.XAxis);
        Basis.XAxis *= BasisP.Scale*(real32)Entry->Bitmap->Width;
        Basis.YAxis *= BasisP.Scale*(real32)Entry->Bitmap->Height;
        
        
        DrawRectangleSlowly(Output, Basis, Entry->Color,Entry->Bitmap, 
                            0, 0, 0, 0, PtM);
#endif
        
      } break;
      case RenderGroupEntryType_render_entry_rectangle: {
        
        
        render_entry_rectangle *Entry = (render_entry_rectangle *)Data;
        BaseAddress += sizeof(*Entry);
        
        entity_basis_p_result BasisP = GetRenderEntityBasisP(Group, &Entry->EntityBasis, ScreenCenter);
        DrawRectangle(Output, BasisP.P, BasisP.P + Entry->Dim, Entry->Color); 
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
                            1.0f/MtP);
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


