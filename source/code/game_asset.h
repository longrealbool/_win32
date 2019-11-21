#if !defined(GAME_ASSET_H)


struct hero_bitmaps {
  
  loaded_bitmap HeroHead;
  loaded_bitmap HeroCape;
  v2 Align;
};

enum asset_state {
  
  GAS_Unloaded,
  GAS_Queued,
  GAS_Loaded,
  GAS_Locked,
};

struct asset_slot {
  
  asset_state State;
  loaded_bitmap *Bitmap;
};

enum asset_tag_id {
  
  Tag_Smootheness,
  Tag_Flatness,
  
  TAG_Count,
};

enum asset_type_id {
  
  AID_Backdrop,
  AID_Tree,
  AID_Sword,
  AID_Rock,
  
  AID_Count,
};

struct asset_tag {
  
  uint32 Tag;
  real32 Value;
};

struct asset {
  
  uint32 FirstTagIndex;
  uint32 OnePastLastTagIndex;
  uint32 SlotID;
};

struct asset_type {
  
  uint32 Count;
  uint32 FirstAssetIndex;
  uint32 OnePastLastAssetIndex;
};

struct asset_bitmap_info {
  
  v2 AlignPercentage;
  real32 WidthOverHeight;
  int32 Height;
  int32 Width;
};

struct asset_group {
  
  uint32 FirstTagIndex;
  uint32 One;
};

struct game_assets {
  
  struct transient_state *TranState;
  
  // NOTE(Egor): this is an asset arena
  memory_arena Arena;

  uint32 BitmapCount;
  asset_slot *Bitmaps;
  
  asset_type AssetTypes[AID_Count];
  
  // NOTE(Egor): array of assets
  loaded_bitmap Grass[2];
  loaded_bitmap Stones[4];
  loaded_bitmap Tuft[3];
  loaded_bitmap Slumps[4];
  
  // NOTE(Egor): structured assets
  hero_bitmaps Hero[4];
  
  // NOTE(Egor): test assets
  loaded_bitmap TestDiffuse;
  loaded_bitmap TestNormal;
};

struct bitmap_id {
  
  uint32 Value;
};



#define GAME_ASSET_H
#endif