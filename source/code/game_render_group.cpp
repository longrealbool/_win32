
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

  render_entry_rectangle *Piece = PushRenderElement(Group, render_entry_rectangle);
  if(Piece) {
    
    Piece->Basis = Group->DefaultBasis;
    Piece->Bitmap = Bitmap;
    Piece->Offset = Group->MtP*V2(Offset.X, -Offset.Y) - Align;
    Piece->OffsetZ = OffsetZ;
    Piece->OffsetZC = OffsetZC;
    
    Piece->Dim = Dim;
    
    Piece->R = Color.R;
    Piece->G = Color.G;
    Piece->B = Color.B;
    Piece->A = Color.A;
  }
}

internal void 
PushBitmap(render_group *Group, loaded_bitmap *Bitmap, v2 Offset, real32 OffsetZ,
           v2 Align, real32 A = 1.0f, real32 OffsetZC = 1.0f) {
  
  PushPiece(Group, Bitmap, Offset, OffsetZ, OffsetZC,  Align, V2(0,0), V4(1.0f, 1.0f, 1.0f, A));
}

internal void 
PushRect(render_group *Group, v2 Offset, real32 OffsetZ, real32 OffsetZC, v2 Dim, v4 Color) {
  
  PushPiece(Group, 0, Offset, OffsetZ, OffsetZC, V2(0,0), Dim, Color);
}

internal void 
PushRectOutline(render_group *Group, v2 Offset, real32 OffsetZ, real32 OffsetZC,
                v2 Dim, v4 Color) {
  
  real32 Thickness = 0.1f;
  
  PushPiece(Group, 0, Offset - V2(0, Dim.Y)*0.5f, OffsetZ, OffsetZC, V2(0,0), V2(Dim.X, Thickness), Color);
  PushPiece(Group, 0, Offset + V2(0, Dim.Y)*0.5f, OffsetZ, OffsetZC, V2(0,0), V2(Dim.X, Thickness), Color);
  
  PushPiece(Group, 0, Offset - V2(Dim.X, 0)*0.5f, OffsetZ, OffsetZC, V2(0,0), V2(Thickness, Dim.Y), Color);
  PushPiece(Group, 0, Offset + V2(Dim.X, 0)*0.5f, OffsetZ, OffsetZC, V2(0,0), V2(Thickness, Dim.Y), Color);
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
              v3 Color)
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
  
  uint32 PixColor = ((RoundReal32ToUInt32(Color.R * 255.0f) << 16) |
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
        
        BaseAddress += sizeof(*Entry);
        
      } break;
      case RenderGroupEntryType_render_entry_rectangle: {
        
        render_entry_rectangle *Entry = (render_entry_rectangle *)Header;
        BaseAddress += sizeof(*Entry);
        
        v3 EntityBaseP = Entry->Basis->P;
        real32 ZFudge = (1.0f + 0.05f*(EntityBaseP.Z + Entry->OffsetZ));
        
        real32 EntityGroundPointX = ScreenCenter.X + EntityBaseP.X*MtP*ZFudge;
        real32 EntityGroundPointY = ScreenCenter.Y - EntityBaseP.Y*MtP*ZFudge;
        real32 EntityZ = -EntityBaseP.Z*MtP;
        
        v2 Cen = {
          EntityGroundPointX + Entry->Offset.X,
          EntityGroundPointY + Entry->Offset.Y  + EntityZ*Entry->OffsetZC
        };
        
        if(Entry->Bitmap) {
          
          DrawBitmap(Output, Entry->Bitmap, Cen.X, Cen.Y, 1.0f);
        }
        else {
          
          v2 HalfDim = Entry->Dim*MtP*0.5f;
          DrawRectangle(Output, Cen - HalfDim, Cen + HalfDim,
                        V3(Entry->R, Entry->G, Entry->B)); 
        }
        
      } break;
      
      InvalidDefaultCase;
      
    }
    

    

    
  }
}