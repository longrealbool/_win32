
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

internal render_group_entry_header *
PushRenderElement_(render_group *Group, uint32 Size, render_group_entry_type Type) {
 
  render_group_entry_header *Result = 0;
  
  if((Group->PushBufferSize + Size) < Group->MaxPushBufferSize) {
   
    Result = (render_group_entry_header *)(Group->PushBufferBase + Group->PushBufferSize);
    Result->Type = Type;
    Group->PushBufferSize += Size;
  }
  else {
    
    InvalidCodePath;
  }
  
  return Result;
}

inline void
PushPiece(render_group *Group, loaded_bitmap *Bitmap,
          v2 Offset, real32 OffsetZ, real32 OffsetZC, v2 Align, v2 Dim, v4 Color) {

  render_entry_bitmap *Piece = PushRenderElement(Group, render_entry_bitmap);
  if(Piece) {
    
    render_entity_basis EntityBasis;
    EntityBasis.Basis = Group->DefaultBasis;
    EntityBasis.Offset = Group->MtP*V2(Offset.X, -Offset.Y) - Align;
    EntityBasis.OffsetZ = OffsetZ;
    EntityBasis.OffsetZC = OffsetZC;
    
    Piece->EntityBasis = EntityBasis;
    Piece->Bitmap = Bitmap;
    Piece->Color = Color;
  }
}

internal void 
PushBitmap(render_group *Group, loaded_bitmap *Bitmap, v2 Offset,
           v2 Align, real32 A = 1.0f,  real32 OffsetZ = 0.0f, real32 OffsetZC = 1.0f) {
  
  PushPiece(Group, Bitmap, Offset, OffsetZ, OffsetZC,  Align, V2(0,0), V4(1.0f, 1.0f, 1.0f, A));
}

internal void 
PushRect(render_group *Group, v2 Offset, real32 OffsetZ, real32 OffsetZC, v2 Dim, v4 Color) {
  
  render_entry_rectangle *Piece = PushRenderElement(Group, render_entry_rectangle);
  if(Piece) {
    
    v2 HalfDim = Dim*Group->MtP*0.5f;
    
    render_entity_basis EntityBasis;
    EntityBasis.Basis = Group->DefaultBasis;
    EntityBasis.Offset = Group->MtP*V2(Offset.X, -Offset.Y) - HalfDim;
    EntityBasis.OffsetZ = OffsetZ;
    EntityBasis.OffsetZC = OffsetZC;
    
    Piece->EntityBasis = EntityBasis;
    Piece->Dim = Dim*Group->MtP;
    Piece->Color = Color;
    
    Assert(Piece->EntityBasis.Basis);
  }
}

internal void 
PushRectOutline(render_group *Group, v2 Offset, v2 Dim, v4 Color,
                real32 OffsetZ = 0.0f, real32 OffsetZC = 1.0f ) {
  
  real32 Thickness = 0.05f;
  
  PushRect(Group, Offset - V2(0, Dim.Y)*0.5f, OffsetZ, OffsetZC, V2(Dim.X, Thickness), Color);
  PushRect(Group, Offset + V2(0, Dim.Y)*0.5f, OffsetZ, OffsetZC, V2(Dim.X, Thickness), Color);
  
  PushRect(Group, Offset - V2(Dim.X, 0)*0.5f, OffsetZ, OffsetZC, V2(Thickness, Dim.Y), Color);
  PushRect(Group, Offset + V2(Dim.X, 0)*0.5f, OffsetZ, OffsetZC, V2(Thickness, Dim.Y), Color);
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
                      + LOADED_BITMAP_BYTES_PER_PIXEL*SourceOffsetX;
  
  // go to line to draw
  uint8 *DestRow = ((uint8 *)Buffer->Memory + MinY*Buffer->Pitch + MinX*LOADED_BITMAP_BYTES_PER_PIXEL);
  
  for(int Y = MinY; Y < MaxY; ++Y)
  {
    uint32 *Dest = (uint32 *)DestRow;
    uint32 *Source = (uint32 *)SourceRow;
    for(int X = MinX; X < MaxX; ++X)
    {

      real32 As = (real32)((*Source >> 24) & 0xFF);
      real32 RAs = (As/255.0f) * CAlpha;
      real32 Rs = CAlpha*(real32)((*Source >> 16) & 0xFF);
      real32 Gs = CAlpha*(real32)((*Source >> 8) & 0xFF);
      real32 Bs = CAlpha*(real32)((*Source >> 0) & 0xFF);
      
      real32 Ad = (real32)((*Dest >> 24) & 0xFF);
      real32 Rd = (real32)((*Dest >> 16) & 0xFF);
      real32 Gd = (real32)((*Dest >> 8) & 0xFF);
      real32 Bd = (real32)((*Dest >> 0) & 0xFF);
      
      real32 RAd = (Ad / 255.0f);
      
      real32 RAsComplement = (1 - RAs );
      
      // NOTE(Egor): alpha channel for compisited bitmaps in premultiplied alpha mode
      // for case when we create intermediate buffer with two or more bitmaps blend with 
      // each other
      real32 A = (RAs + RAd - RAs*RAd)*255.0f; 
      real32 R = RAsComplement*Rd + Rs;
      real32 G = RAsComplement*Gd + Gs;
      real32 B = RAsComplement*Bd + Bs;
      
      
      *Dest = (((uint32)(A + 0.5f) << 24) |
               ((uint32)(R + 0.5f) << 16) |
               ((uint32)(G + 0.5f) << 8)  |
               ((uint32)(B + 0.5f) << 0));
      
      Dest++;
      Source++;
    }
    DestRow += Buffer->Pitch;
    SourceRow += Bitmap->Pitch;
    
  }
}

internal void
DrawRectangle(loaded_bitmap *Buffer, v2 vMin, v2 vMax,
              v4 Color)
{
  
  int32 MinX = RoundReal32ToInt32(vMin.X);
  int32 MinY = RoundReal32ToInt32(vMin.Y);
  int32 MaxX = RoundReal32ToInt32(vMax.X);
  int32 MaxY = RoundReal32ToInt32(vMax.Y);
  
  // clipping value to actual size of the backbuffer
  if(MinX < 0) MinX = 0;
  if(MinY < 0) MinY = 0;
  // we will write up to, but not including to final row and column
  if(MaxX > Buffer->Width) MaxX = Buffer->Width;
  if(MaxY > Buffer->Height) MaxY = Buffer->Height;
  
  uint32 PixColor = ((RoundReal32ToUInt32(Color.A * 255.0f) << 24) |
                     (RoundReal32ToUInt32(Color.R * 255.0f) << 16) |
                     (RoundReal32ToUInt32(Color.G * 255.0f) << 8)  |
                     (RoundReal32ToUInt32(Color.B * 255.0f) << 0));
  
  
  uint8 *EndOfBuffer = (uint8 *)Buffer->Memory + Buffer->Pitch*Buffer->Height;
  
  // go to line to draw
  uint8 *Row = ((uint8 *)Buffer->Memory + MinY*Buffer->Pitch + MinX*LOADED_BITMAP_BYTES_PER_PIXEL);
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



inline v2
GetRenderEntityBasisP(render_group *Group, render_entity_basis *EntityBasis, v2 ScreenCenter) {
  
  real32 MtP = Group->MtP;
  
  v3 EntityBaseP = EntityBasis->Basis->P;
  real32 ZFudge = (1.0f + 0.05f*(EntityBaseP.Z + EntityBasis->OffsetZ));
  
  real32 EntityGroundPointX = ScreenCenter.X + EntityBaseP.X*MtP*ZFudge;
  real32 EntityGroundPointY = ScreenCenter.Y - EntityBaseP.Y*MtP*ZFudge;
  real32 EntityZ = -EntityBaseP.Z*MtP;
  
  v2 Cen = {
    EntityGroundPointX + EntityBasis->Offset.X,
    EntityGroundPointY + EntityBasis->Offset.Y  + EntityZ*EntityBasis->OffsetZC
  };
  
  return Cen;
}


inline v4 
SRGB255ToLinear1(v4 C) {
  
  v4 Result;
  
  real32 Inv255 = 1.0f/255.0f;
  
  Result.R = Square(Inv255*C.R);
  Result.G = Square(Inv255*C.G);
  Result.B = Square(Inv255*C.B);
  Result.A = Inv255*C.A;
  
  return Result;
};


inline v4 
Linear1ToSRGB255(v4 C) {
  
  v4 Result;
  
  255.0f;
  
  Result.R = 255.0f*SquareRoot(C.R);
  Result.G = 255.0f*SquareRoot(C.G);
  Result.B = 255.0f*SquareRoot(C.B);
  Result.A = 255.0f*C.A;
  
  return Result;
}


internal void
DrawRectangleSlowly(loaded_bitmap *Buffer, render_v2_basis Basis, v4 Color, loaded_bitmap *Texture) {
  
  int32 Width = Buffer->Width - 1;
  int32 Height = Buffer->Height - 1;
  
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
  
#if 1
  
  v2 P[4] = {Min, Min + Basis.XAxis, Min + Basis.YAxis, Max};
  
  for(uint32 Index = 0; Index < ArrayCount(P); ++Index) {
    
    v2 *TestP = P + Index;
    
    int32 CeilX = CeilReal32ToInt32(TestP->X);
    int32 FloorX = FloorReal32ToInt32(TestP->X);
    int32 CeilY = CeilReal32ToInt32(TestP->Y);
    int32 FloorY = FloorReal32ToInt32(TestP->Y);
    
    if(XMin > FloorX) XMin = FloorX;
    if(YMin > FloorY) YMin = FloorY;
    if(XMax < CeilX) XMax = CeilX;
    if(YMax < CeilY) YMax = CeilY;
  }
  
#else 
  // TODO(Egor): Test which is faster
  // TODO(Egor): we could push the speed by refusing from Min\Max
  v2 Result = Max - Min;
  if(AbsoluteValue(Result.X) > AbsoluteValue(Result.Y)) {
    
    XMax = RoundReal32ToInt32(Max(Max.X, Min.X));
    XMin = RoundReal32ToInt32(Min(Max.X, Min.X));
  }
  else {
    
    YMax = RoundReal32ToInt32(Max(Max.Y, Min.Y));
    YMin = RoundReal32ToInt32(Min(Max.Y, Min.Y));
  }
  
  Result = toAxisX - toAxisY;
  if(AbsoluteValue(Result.X) > AbsoluteValue(Result.Y)) {
    
    XMax = RoundReal32ToInt32(Max(toAxisX.X, toAxisY.X));
    XMin = RoundReal32ToInt32(Min(toAxisX.X, toAxisY.X));
  }
  else {
    
    YMax = RoundReal32ToInt32(Max(toAxisX.Y, toAxisY.Y));
    YMin = RoundReal32ToInt32(Min(toAxisX.Y, toAxisY.Y));
  }
  
#endif
  
  if(XMin < 0) XMin = 0;
  if(YMin < 0) YMin = 0;
  if(XMax > Width) XMax = Width;
  if(YMax > Height) YMax = Height;
  
  uint32 PixColor = ((RoundReal32ToUInt32(Color.A * 255.0f) << 24) |
                     (RoundReal32ToUInt32(Color.R * 255.0f) << 16) |
                     (RoundReal32ToUInt32(Color.G * 255.0f) << 8)  |
                     (RoundReal32ToUInt32(Color.B * 255.0f) << 0));
  
  uint8 *Row = (uint8 *)Buffer->Memory + YMin*Buffer->Pitch + XMin*LOADED_BITMAP_BYTES_PER_PIXEL;

  for(int32 Y = YMin; Y <= YMax; ++Y) {
    
    uint32 *Pixel = (uint32 *)Row;
    for(int32 X = XMin; X <= XMax; ++X) {
      
      v2 PixelP = V2i(X, Y);
      v2 d = PixelP - Basis.Origin;
      
      
      real32 Edge0 = Inner(d - Basis.XAxis - Basis.YAxis, Basis.XAxis);
      real32 Edge1 = Inner(d - Basis.XAxis - Basis.YAxis, Basis.YAxis); 
      real32 Edge2 = Inner(d, -Basis.XAxis); 
      real32 Edge3 = Inner(d, -Basis.YAxis); 
      
#if 1
      if(Edge0 < 0 &&
         Edge1 < 0 &&
         Edge2 < 0 &&
         Edge3 < 0) {
        
        v2 UV =  V2(InvXAxisLengthSq * Inner(Basis.XAxis,d),
                    InvYAxisLengthSq * Inner(Basis.YAxis,d));
        
        // TODO(Egor): clamp this later
        Assert(UV.X >= 0 && UV.X <= 1.0f);
        Assert(UV.Y >= 0 && UV.Y <= 1.0f);
        
        real32 tX = 1.0f + (UV.X * (real32)(Texture->Width - 3) + 0.5f);
        real32 tY = 1.0f + (UV.Y * (real32)(Texture->Height - 3) + 0.5f);
        
        int32 PixelX = (int32)tX;
        int32 PixelY = (int32)tY;
        
        // NOTE(Egor): take the fractional part of tX and tY
        real32 fX = tX - (real32)PixelX;
        real32 fY = tY - (real32)PixelY;
        
        Assert(PixelX >= 0 && PixelX <= (Texture->Width - 1));
        Assert(PixelY >= 0 && PixelY <= (Texture->Height - 1));
        
        uint8 *TexelPtr = ((uint8 *)Texture->Memory)
          + PixelY*Texture->Pitch + PixelX*LOADED_BITMAP_BYTES_PER_PIXEL;
        
        uint32 TexelValA = *(uint32 *)(TexelPtr);
        uint32 TexelValB = *(uint32 *)(TexelPtr + sizeof(uint32));
        uint32 TexelValC = *(uint32 *)(TexelPtr + Texture->Pitch);
        uint32 TexelValD = *(uint32 *)(TexelPtr + Texture->Pitch + sizeof(uint32));
        
        v4 TexelA  = V4((real32)((TexelValA >> 16) & 0xFF),
                        (real32)((TexelValA >> 8) & 0xFF),
                        (real32)((TexelValA >> 0) & 0xFF),
                        (real32)((TexelValA >> 24) & 0xFF));
        
        v4 TexelB  = V4((real32)((TexelValB >> 16) & 0xFF),
                        (real32)((TexelValB >> 8) & 0xFF),
                        (real32)((TexelValB >> 0) & 0xFF),
                        (real32)((TexelValB >> 24) & 0xFF));
        
        v4 TexelC  = V4((real32)((TexelValC >> 16) & 0xFF),
                        (real32)((TexelValC >> 8) & 0xFF),
                        (real32)((TexelValC >> 0) & 0xFF),
                        (real32)((TexelValC >> 24) & 0xFF));
        
        v4 TexelD  = V4((real32)((TexelValD >> 16) & 0xFF),
                        (real32)((TexelValD >> 8) & 0xFF),
                        (real32)((TexelValD >> 0) & 0xFF),
                        (real32)((TexelValD >> 24) & 0xFF));
        
        // NOTE(Egor): from SRGB to 'linear' brightness space
        TexelA = SRGB255ToLinear1(TexelA);
        TexelB = SRGB255ToLinear1(TexelB);
        TexelC = SRGB255ToLinear1(TexelC);
        TexelD = SRGB255ToLinear1(TexelD);
        
        v4 Texel = Lerp(Lerp(TexelA, TexelB, fX),
                        Lerp(TexelC, TexelD, fX),
                        fY);
        
        
        real32 RAs = Texel.A * Color.A;
        
        v4 Dest = V4((real32)((*Pixel >> 16) & 0xFF),
                     (real32)((*Pixel >> 8) & 0xFF),
                     (real32)((*Pixel >> 0) & 0xFF),
                     (real32)((*Pixel >> 24) & 0xFF));
        
        Dest = SRGB255ToLinear1(Dest);
        
        real32 RAd = Dest.A;
        real32 RAsComplement = (1 - RAs);
        
        // NOTE(Egor): alpha channel for compisited bitmaps in premultiplied alpha mode
        // for case when we create intermediate buffer with two or more bitmaps blend with 
        // each other
        v4 Blended = V4(RAsComplement*Dest.R + Texel.R,
                        RAsComplement*Dest.G + Texel.G,
                        RAsComplement*Dest.B + Texel.B,
                        (RAs + RAd - RAs*RAd)); 
        
        v4 Blended255 = Linear1ToSRGB255(Blended);
        
        
        *Pixel = (((uint32)(Blended255.A + 0.5f) << 24) |
                  ((uint32)(Blended255.R + 0.5f) << 16) |
                  ((uint32)(Blended255.G + 0.5f) << 8)  |
                  ((uint32)(Blended255.B + 0.5f) << 0));
        
      }
#else
      *Pixel = PixColor;
#endif
      
      Pixel++;
    }
    Row += Buffer->Pitch;
  }
}




internal void
RenderPushBuffer(render_group *Group, loaded_bitmap *Output) {
  
  real32 MtP = Group->MtP;
  
  v2 ScreenCenter = V2i(Output->Width, Output->Height)*0.5f;
  
  // NOTE(Egor): render
  for(uint32 BaseAddress = 0; BaseAddress < Group->PushBufferSize;) {
    
    render_group_entry_header *Header = (render_group_entry_header *)(Group->PushBufferBase + BaseAddress);
    
    switch(Header->Type) {
      
      case RenderGroupEntryType_render_entry_clear: {
        
        render_entry_clear *Entry = (render_entry_clear *)Header;
        
        DrawRectangle(Output, V2(0.0f, 0.0f),
                      V2((real32)Output->Width, (real32)Output->Height),
                      Entry->Color);
        
        BaseAddress += sizeof(*Entry);
        
      } break;
      case RenderGroupEntryType_render_entry_bitmap: {
        
        
        render_entry_bitmap *Entry = (render_entry_bitmap *)Header;
        BaseAddress += sizeof(*Entry);
        
        v2 P = GetRenderEntityBasisP(Group, &Entry->EntityBasis, ScreenCenter);
        
        Assert(Entry->Bitmap);
        DrawBitmap(Output, Entry->Bitmap, P.X, P.Y, 1.0f);
        
      } break;
      case RenderGroupEntryType_render_entry_rectangle: {
        
        
        render_entry_rectangle *Entry = (render_entry_rectangle *)Header;
        BaseAddress += sizeof(*Entry);
        
        v2 P = GetRenderEntityBasisP(Group, &Entry->EntityBasis, ScreenCenter);
        DrawRectangle(Output, P, P + Entry->Dim, Entry->Color); 
      } break;
      case RenderGroupEntryType_render_entry_coordinate_system: {
        
        
        render_entry_coordinate_system *Entry = (render_entry_coordinate_system *)Header;
        BaseAddress += sizeof(*Entry);
        
        v2 DimOrigin = V2(5, 5);
        
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
        
        
        DrawRectangleSlowly(Output, V2Basis, V4(1,0,1,1), Entry->Texture);
        
#if 0
        for(uint32 I = 0; I < ArrayCount(Entry->Points); ++I) {
          
          v2 Point = Entry->Points[I];
          
          P = Entry->Origin + Point.X*Entry->XAxis + Point.Y*Entry->YAxis; 
          
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
PushCoordinateSystem(render_group *Group, v2 Origin, v2 XAxis, v2 YAxis, v4 Color, loaded_bitmap* Bitmap) {
  
  render_entry_coordinate_system *Entry = PushRenderElement(Group, render_entry_coordinate_system);
  
  if(Entry) {
    
    
    Entry->EntityBasis.Basis = Group->DefaultBasis;
    Entry->Origin = Origin;
    Entry->XAxis = XAxis;
    Entry->YAxis = YAxis;
    Entry->Color = Color;
    Entry->Texture = Bitmap;
    
  }
  
  return Entry;
}

