#if !defined(GAME_RENDER_GROUP_H)


struct render_basis {
  
  v3 P;
  
};

struct entity_visible_piece {
  
  render_basis *Basis;
  loaded_bitmap *Bitmap;
  v2 Offset;
  real32 OffsetZ;
  real32 OffsetZC;
  real32 R, G, B, A;
  v2 Dim;
};

struct render_group {

  render_basis *DefaultBasis;
  game_state *GameState;
  uint32 Count;
  entity_visible_piece Pieces[4096];
};



inline void
PushPiece(render_group *Group, loaded_bitmap *Bitmap,
          v2 Offset, real32 OffsetZ, real32 OffsetZC, v2 Align, v2 Dim, v4 Color) {
  
  Assert(Group->Count < ArrayCount(Group->Pieces));
  entity_visible_piece *Piece = Group->Pieces + Group->Count++;
  Piece->Basis = Group->DefaultBasis;
  Piece->Bitmap = Bitmap;
  Piece->Offset = Group->GameState->MtP*V2(Offset.X, -Offset.Y) - Align;
  Piece->OffsetZ = OffsetZ;
  Piece->OffsetZC = OffsetZC;
  
  Piece->Dim = Dim;
  
  Piece->R = Color.R;
  Piece->G = Color.G;
  Piece->B = Color.B;
  Piece->A = Color.A;
}

internal void 
PushBitmap(render_group *Group, loaded_bitmap *Bitmap,
           v2 Offset, real32 OffsetZ, v2 Align, real32 A = 1.0f, real32 OffsetZC = 1.0f)
{
  
  PushPiece(Group, Bitmap, Offset, OffsetZ, OffsetZC,  Align, V2(0,0), V4(1.0f, 1.0f, 1.0f, A));
}

internal void 
PushRect(render_group *Group,
         v2 Offset, real32 OffsetZ, real32 OffsetZC,
         v2 Dim, v4 Color)

{
  
  PushPiece(Group, 0, Offset, OffsetZ, OffsetZC, V2(0,0), Dim,
            Color);
}

internal void 
PushRectOutline(render_group *Group,
                v2 Offset, real32 OffsetZ, real32 OffsetZC,
                v2 Dim, v4 Color){
  
  real32 Thickness = 0.1f;
  
  PushPiece(Group, 0, Offset - V2(0, Dim.Y)*0.5f, OffsetZ, OffsetZC, V2(0,0), V2(Dim.X, Thickness), Color);
  PushPiece(Group, 0, Offset + V2(0, Dim.Y)*0.5f, OffsetZ, OffsetZC, V2(0,0), V2(Dim.X, Thickness), Color);
  
  PushPiece(Group, 0, Offset - V2(Dim.X, 0)*0.5f, OffsetZ, OffsetZC, V2(0,0), V2(Thickness, Dim.Y), Color);
  PushPiece(Group, 0, Offset + V2(Dim.X, 0)*0.5f, OffsetZ, OffsetZC, V2(0,0), V2(Thickness, Dim.Y), Color);
}



#define GAME_RENDER_GROUP_H
#endif
