#if !defined(GAME_RENDER_GROUP_H)

struct render_basis {
  
  v3 P;
};

struct render_entity_basis {
  
  render_basis *Basis;
  v2 Offset;
  real32 OffsetZ;
  real32 OffsetZC;
};



enum render_group_entry_type {
  
  RenderGroupEntryType_render_entry_clear,
  RenderGroupEntryType_render_entry_bitmap,
  RenderGroupEntryType_render_entry_rectangle,
  RenderGroupEntryType_render_entry_coordinate_system,
};

struct render_group_entry_header {
  
  render_group_entry_type Type;
};

struct render_entry_clear {

  render_group_entry_header Header;
  v4 Color;
};

struct render_entry_bitmap {
  
  render_group_entry_header Header;
  loaded_bitmap *Bitmap;
  v4 Color;
  render_entity_basis EntityBasis;
};

struct render_entry_rectangle {
  
  render_group_entry_header Header;
  v4 Color;
  render_entity_basis EntityBasis;
  v2 Dim;
};


struct render_entry_coordinate_system {
  
  render_group_entry_header Header;
  render_entity_basis EntityBasis;
  v2 Origin;
  v2 XAxis;
  v2 YAxis;
  v4 Color;
  
  v2 Points[25];
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
