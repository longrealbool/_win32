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
  Tag_FacingDirection,
  
  Tag_Count,
};

enum asset_type_id {
  
  AID_None,
  
  AID_Backdrop,
  AID_Tree,
  AID_Sword,
  AID_Rock,
  
  AID_Grass,
  AID_Stone,
  AID_Tuft,
  
  AID_Figurine,
  AID_FigurineArrow,
  
  AID_Count,
};

struct asset_tag {
  
  uint32 ID;
  real32 Value;
};


struct asset_vector {
  
  real32 E[Tag_Count];
};

struct asset {
  
  uint32 FirstTagIndex;
  uint32 OnePastLastTagIndex;
  uint32 SlotID;
};

struct asset_type {
  
  uint32 FirstAssetIndex;
  uint32 OnePastLastAssetIndex;
};

struct asset_bitmap_info {
  
  v2 AlignPercentage;
  char *FileName;
};


struct game_assets {
  
  struct transient_state *TranState;
  // NOTE(Egor): this is an asset arena
  memory_arena Arena;
  
  uint32 DEBUGUsedBitmapCount;
  uint32 DEBUGUsedAssetCount;
  asset_type *DEBUGAssetType;
  
  uint32 BitmapCount;
  asset_bitmap_info *BitmapInfos;
  asset_slot *Bitmaps;
  
  uint32 AssetCount;
  asset *Assets;
  
  uint32 TagCount;
  asset_tag *Tags;
  
  asset_type AssetTypes[AID_Count];
  
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