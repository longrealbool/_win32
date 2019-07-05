#if !defined(GAME_WORLD_H)


struct world_position {
  
  int32 ChunkX;
  int32 ChunkY;
  int32 ChunkZ;
  
  // NOTE(Egor): origin is at the center of a tile
  v3 Offset_;
};


struct world_entity_block {
  
  uint32 EntityCount;
  uint32 LowEntityIndex[16];
  world_entity_block *Next;
};

struct world_chunk {
  
  int32 ChunkX, ChunkY, ChunkZ;
  
  world_entity_block FirstBlock;
  
  world_chunk *NextInHash;
};

struct world {
  
  real32 TileSideInMeters;
  real32 TileDepthInMeters;
//  real32 ChunkSideInMeters;
  v3 ChunkDimInMeters;
  
  world_entity_block *FirstFree;
  
  world_chunk ChunkHash[4096];
};

#define GAME_WORLD_H
#endif