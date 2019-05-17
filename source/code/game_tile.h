#if !defined(GAME_TILE_H)

struct tile_map_difference {
  
  v2 dXY;
  real32 dZ;
};

struct tile_map_position {
  
  uint32 AbsTileX;
  uint32 AbsTileY;
  uint32 AbsTileZ;
  
  // NOTE(Egor): origin is at the center of a tile
  v2 Offset_;
};

struct tile_chunk_position {
  
  uint32 TileChunkX;
  uint32 TileChunkY;
  uint32 TileChunkZ;
  
  // NOTE(Egor): tile coord's in on tilechunk
  uint32 TileX;
  uint32 TileY;
};

struct tile_chunk {
  
  uint32 TileChunkX;
  uint32 TileChunkY;
  uint32 TileChunkZ;
  
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