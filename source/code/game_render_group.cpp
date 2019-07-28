
internal render_group *
AllocateRenderGroup(memory_arena *Arena, uint32 MaxPushBufferSize, real32 MtP) {
 
  render_group *Result = PushStruct(Arena, render_group);
  Result->PushBufferBase = (uint8 *)PushSize(Arena, MaxPushBufferSize);
  Result->PushBufferSize = 0;
  Result->MaxPushBufferSize = MaxPushBufferSize;
  
  Result->DefaultBasis = PushStruct(Arena, render_basis);
  Result->DefaultBasis->P = V3(0.0f, 0.0f, 0.0f);
  Result->Count = 0;
  Result->MtP = MtP;
  
  return Result;
}


internal void *
PushRenderElement(render_group *Group, uint32 Size) {
 
  void *Result = 0;
  
  if((Group->PushBufferSize + Size) < Group->MaxPushBufferSize) {
   
    Result = (Group->PushBufferBase + Group->PushBufferSize);
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

  Group->Count++;
  render_piece *Piece = (render_piece *)PushRenderElement(Group, sizeof(render_piece));;
  
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