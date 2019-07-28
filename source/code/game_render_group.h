#if !defined(GAME_RENDER_GROUP_H)


struct render_basis {
  
  v3 P;
  
};

struct render_piece {
  
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
  real32 MtP;
  uint32 Count;

  uint32 MaxPushBufferSize;
  uint32 PushBufferSize;
  uint8 *PushBufferBase;
};




#define GAME_RENDER_GROUP_H
#endif
