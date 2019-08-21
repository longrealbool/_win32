#if !defined(GAME_RENDER_GROUP_H)

/* NOTE(Egor):

   1. Everywhere Y always _must_ goes upward, X goes to the right
   2. All bitmaps including the render target assumed to be bottom-up
   3. All inputs to the rendere are in world coordinate "meters", _NOT_ in pixels.
      If something is in pixels values, it will be explicitly marked as such.
   4. Z is a special coordinate that represents discrete slices of world.
   
*/


struct bilinear_sample {
  
  uint32 A;
  uint32 B;
  uint32 C;
  uint32 D;
}; 

struct loaded_bitmap {

  real32 WidthOverHeight;
  real32 NativeHeight;
  v2 AlignPercentage;
  
  int32 Height;
  int32 Width;
  int32 Pitch;
  
  void* Memory;
};

struct environment_map {
  
  loaded_bitmap LOD[4];
  real32 Pz;
};

struct render_basis {
  
  v3 P;
};

struct render_entity_basis {
  
  render_basis *Basis;
  v3 Offset;
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
  
  v4 Color;
};

struct render_entry_bitmap {
  
  loaded_bitmap *Bitmap;
  v4 Color;
  render_entity_basis EntityBasis;
};

struct render_entry_rectangle {
  
  v4 Color;
  render_entity_basis EntityBasis;
  v2 Dim;
};


struct render_v2_basis {
  
  v2 Origin;
  v2 XAxis;
  v2 YAxis;
};


struct render_group {

  render_basis *DefaultBasis;
  real32 GlobalAlpha;
  real32 MtP;

  uint32 MaxPushBufferSize;
  uint32 PushBufferSize;
  uint8 *PushBufferBase;
};

// NOTE(Egor): for test only
struct render_entry_coordinate_system {
  
  render_entity_basis EntityBasis;
  v2 Origin;
  v2 XAxis;
  v2 YAxis;
  v4 Color;
  loaded_bitmap *Texture;
  loaded_bitmap *NormalMap;
  
  environment_map *Top;
  environment_map *Middle;
  environment_map *Bottom;
};


#define GAME_RENDER_GROUP_H
#endif
