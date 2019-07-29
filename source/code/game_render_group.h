#if !defined(GAME_RENDER_GROUP_H)

struct render_basis {
  
  v3 P;
};

enum render_group_entry_type {
  
  RenderGroupEntryType_render_entry_clear,
  RenderGroupEntryType_render_entry_rectangle,
};

struct render_group_entry_header {
  
  render_group_entry_type Type;
};

struct render_entry_clear {

  render_group_entry_header Header;
  real32 R, G, B, A;
};

struct render_entry_rectangle {
  
  render_group_entry_header Header;
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

  uint32 MaxPushBufferSize;
  uint32 PushBufferSize;
  uint8 *PushBufferBase;
};


#define GAME_RENDER_GROUP_H
#endif
