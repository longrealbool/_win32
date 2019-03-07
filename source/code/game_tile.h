#if !defined(GAME_TILE_H)

struct tile_map_position {
  
  uint32 AbsTileX;
  uint32 AbsTileY;
  uint32 AbsTileZ;
  // coordinates inside one tile
  real32 X;
  real32 Y;
};

struct tile_chunk_position {
  uint32 TileChunkX;
  uint32 TileChunkY;
  uint32 TileChunkZ;
  
  uint32 TileX;
  uint32 TileY;
};

struct tile_chunk {
  
  uint32 *Tiles;
};

struct tile_map {
  
  uint32 ChunkShift;
  uint32 ChunkMask;
  uint32 ChunkDim;
  
  uint32 TileChunkCountX;
  uint32 TileChunkCountY;
  uint32 TileChunkCountZ;
  
  real32 TileSideInMeters;

  
  tile_chunk *TileChunks;
};



#define GAME_TILE_H
#endif