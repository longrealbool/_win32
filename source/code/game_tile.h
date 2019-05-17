#if !defined(GAME_TILE_H)

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

struct tile_chunk {
  
  int32 TileChunkX;
  int32 TileChunkY;
  int32 TileChunkZ;
  
  uint32 *Tiles;
  
  tile_chunk *NextInHash;
};

struct tile_map {
  
  uint32 ChunkShift;
  uint32 ChunkMask;
  uint32 ChunkDim;
  
  real32 TileSideInMeters;

  tile_chunk TileChunkHash[4096];
};





#define GAME_TILE_H
#endif