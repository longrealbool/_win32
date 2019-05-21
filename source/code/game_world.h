#if !defined(GAME_WORLD_H)


struct tile_map_difference {
  
  v2 dXY;
  real32 dZ;
};

struct tile_map_position {
  
  int32 AbsTileX;
  int32 AbsTileY;
  int32 AbsTileZ;
  
  // NOTE(Egor): origin is at the center of a tile
  v2 Offset_;
};

struct tile_chunk_position {
  
  int32 TileChunkX;
  int32 TileChunkY;
  int32 TileChunkZ;
  
  // NOTE(Egor): tile coord's in on tilechunk
  int32 TileX;
  int32 TileY;
};

struct tile_entity_block {
  
  uint32 EntityCount;
  uint32 LowEntityIndex[16];
  tile_entity_block *Next;
};

struct tile_chunk {
  
  int32 ChunkX;
  int32 ChunkY;
  int32 ChunkZ;
  
  tile_entity_block FirstBlock;
  
  tile_chunk *NextInHash;
};

struct world {
  
  real32 TileSideInMeters;
  
  uint32 WorldChunkShift;
  uint32 WorldChunkMask;
  uint32 WorldChunkDim;
  world_chunk WorldChunkHash[4096];
};




#define GAME_WORLD_H
#endif