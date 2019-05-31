#if !defined(GAME_WORLD_H)


struct world_difference {
  
  v2 dXY;
  real32 dZ;
};

struct world_position {
  
  int32 ChunkX;
  int32 ChunkY;
  int32 ChunkZ;
  
  // NOTE(Egor): origin is at the center of a tile
  v2 Offset_;
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
  real32 ChunkSideInMeters;
  
  world_entity_block *FirstFree;
  
  world_chunk ChunkHash[4096];
};

#define GAME_WORLD_H
#endif