// TODO(Egor): I need to think about what SAFE_MARGIN is
#define CHUNK_SAFE_MARGIN (INT32_MAX / 16) * 4
#define CHUNK_UNITIALIZED INT32_MAX


inline bool32 
IsCanonical(world *World, real32 TileRel) {
  
  bool32 Result = (TileRel >= -World->ChunkSideInMeters*0.5f &&
                   TileRel <= World->ChunkSideInMeters*0.5f);
  return Result;
}

inline bool32 
IsCanonical(world *World, v2 Offset) {
  
  bool32 Result = IsCanonical(World, Offset.X) && IsCanonical(World, Offset.Y);
  return Result;
}


inline world_chunk *
GetChunk(world *World, int32 ChunkX, int32 ChunkY, int32 ChunkZ,
         memory_arena *Arena = 0) {
  
  Assert(ChunkX > -CHUNK_SAFE_MARGIN);
  Assert(ChunkY > -CHUNK_SAFE_MARGIN);
  Assert(ChunkZ > -CHUNK_SAFE_MARGIN);
  
  Assert(ChunkX < CHUNK_SAFE_MARGIN);
  Assert(ChunkY < CHUNK_SAFE_MARGIN);
  Assert(ChunkZ < CHUNK_SAFE_MARGIN);
  
  // TODO(Egor): make a better hash function, lol
  uint32 HashValue = 19*ChunkX + 7*ChunkY + 3*ChunkZ;
  uint32 HashSlot = HashValue & (ArrayCount(World->ChunkHash) - 1);
  Assert(HashSlot < ArrayCount(World->ChunkHash));
  
  world_chunk *Chunk = World->ChunkHash + HashSlot;
  do {
    
    if(Chunk->ChunkX == ChunkX &&
       Chunk->ChunkY == ChunkY &&
       Chunk->ChunkZ == ChunkZ) {
      
      break;
    } 
    
    // NOTE(Egor): if our initial slot is initialized, and there isn't chained chunk
    if(Arena && (Chunk->ChunkX != CHUNK_UNITIALIZED && (!Chunk->NextInHash))) {
      
      Chunk->NextInHash = PushStruct(Arena, world_chunk);
      Chunk = Chunk->NextInHash;
      Chunk->ChunkX = CHUNK_UNITIALIZED; 
    }
    
    // TODO(Egor): check if I garanteed that newly allocated world_chunk will be zeroed
    // NOTE(Egor): initialize initial or newly created chained chunk
    if(Arena && Chunk->ChunkX == CHUNK_UNITIALIZED) {
      
      Chunk->ChunkX = ChunkX;
      Chunk->ChunkY = ChunkY;
      Chunk->ChunkZ = ChunkZ;
      
      Chunk->NextInHash = 0;
      
      break;
    }
    
    Chunk = Chunk->NextInHash;
  }while(Chunk);
  
  return Chunk;
}

#if 0
inline world_chunk_position
GetChunkPos(world *World, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ) {
  
  world_chunk_position Result;
  
  Result.ChunkX = AbsTileX >> TileMap->ChunkShift;
  Result.ChunkY = AbsTileY >> TileMap->ChunkShift;
  Result.ChunkZ = AbsTileZ;
  
  Result.TileX = AbsTileX & TileMap->ChunkMask;
  Result.TileY = AbsTileY & TileMap->ChunkMask;
  
  return Result;
  
}
#endif


////
// TODO(Egor): maybe I should transfer this into another file



inline void
RecanonicalizeCoord(world *World, int32 *Tile, real32 *TileRel) {
  
  real32 RelCoord = *TileRel; // DEBUG
  
  int32 Offset = RoundReal32ToInt32(*TileRel /World->ChunkSideInMeters);
  *Tile += Offset;
  *TileRel -= Offset*World->ChunkSideInMeters;
  
  Assert(IsCanonical(World, *TileRel));
}



// TODO (Egor): we cannot move faster than 1 tile map in on gameLoop
inline world_position
MapIntoChunkSpace(world *World, world_position BasePos, v2 Offset) {
  
  world_position Result = BasePos;
  
  Result.Offset_ += Offset;
  RecanonicalizeCoord(World, &Result.ChunkX, &Result.Offset_.X);
  RecanonicalizeCoord(World, &Result.ChunkY, &Result.Offset_.Y);
  
  return Result;
}



inline world_difference 
Subtract(world *World, world_position *A, world_position *B) {
  
  world_difference Result;
  
  v2 dTileXY = V2((real32)A->ChunkX - (real32)B->ChunkX,
                  (real32)A->ChunkY - (real32)B->ChunkY);
  
  real32 dTileZ = (real32)A->ChunkZ - (real32)B->ChunkZ;
  
  Result.dXY = World->ChunkSideInMeters*dTileXY + (A->Offset_ - B->Offset_);
  // NOTE(Egor): Z is not a real coordinate right now
  Result.dZ = World->ChunkSideInMeters*dTileZ;
  
  return Result;
}

#if 0
inline bool32
AreOnTheSameTile(world_position *A, world_position *B) {
  
  bool32 Result = (A->AbsTileX == B->AbsTileX &&
                   A->AbsTileY == B->AbsTileY &&
                   A->AbsTileZ == B->AbsTileZ);
  
  return Result;
}
#endif

inline bool32
AreInTheSameChunk(world *World, world_position *A, world_position *B) {
  
  Assert(IsCanonical(World, A->Offset_));
  Assert(IsCanonical(World, B->Offset_));
  
  
  bool32 Result = (A->ChunkX == B->ChunkX &&
                   A->ChunkY == B->ChunkY &&
                   A->ChunkZ == B->ChunkZ);
  
  return Result;
}

#define TILES_PER_CHUNK 16
internal void
InitializeWorld(world *World, real32 TileSideInMeters) {
  
  World->TileSideInMeters = TileSideInMeters;
  World->ChunkSideInMeters = (real32)TILES_PER_CHUNK*TileSideInMeters;
  World->FirstFree = 0;
  
  for(uint32 ChunkIndex = 0;
      ChunkIndex < ArrayCount(World->ChunkHash); ++ChunkIndex) {
    
    World->ChunkHash[ChunkIndex].ChunkX = CHUNK_UNITIALIZED;
    World->ChunkHash[ChunkIndex].FirstBlock.EntityCount = 0;
    
  }
}

inline world_position
CenteredChunkPoint(uint32 ChunkX, uint32 ChunkY, uint32 ChunkZ) {
  
  world_position Result = {};
  
  Result.ChunkX = ChunkX;
  Result.ChunkY = ChunkY;
  Result.ChunkZ = ChunkZ;
  
  return Result;
  
  
}


inline void
ChangeEntityLocation(memory_arena *Arena, world *World, uint32 LowEntityIndex, 
                     world_position *OldP, world_position *NewP) {
  
  if(OldP && AreInTheSameChunk(World, OldP, NewP)) {
    
  }
  else {
    
    if(OldP) {
      
      world_chunk *Chunk = GetChunk(World, OldP->ChunkX, OldP->ChunkY, OldP->ChunkZ);
      Assert(Chunk);
      world_entity_block *FirstBlock = &Chunk->FirstBlock;
      // TODO(Egor): I'm not really sure if I want this IF case be there
      if(Chunk) {
        
        bool32 NotFound = true;
        for(world_entity_block *Block = &Chunk->FirstBlock; Block && NotFound; Block = Block->Next) {
          
          for(uint32 Index = 0; Index < Block->EntityCount && NotFound; ++Index) {
            
            if(Block->LowEntityIndex[Index] == LowEntityIndex) {
              
              Block->LowEntityIndex[Index] =
                FirstBlock->LowEntityIndex[--FirstBlock->EntityCount];
              if(FirstBlock->EntityCount == 0) {
                
                if(FirstBlock->Next) {
                  
                  world_entity_block *NextBlock = FirstBlock->Next;
                  *FirstBlock = *NextBlock;
                  // NOTE(Egor): freeing the memory
                  NextBlock->Next = World->FirstFree;
                  World->FirstFree = NextBlock;
                }
              }
              
              // NOTE(Egor): nasty double break
              NotFound = false;
            }
          }
        }
      }
    }
    
    world_chunk *Chunk = GetChunk(World, NewP->ChunkX, NewP->ChunkY, NewP->ChunkZ, Arena);
    world_entity_block *Block = &Chunk->FirstBlock;
    if(Block->EntityCount == ArrayCount(Block->LowEntityIndex)) {
      
      // NOTE(Egor): we are out of space in entity block, need a new one
      world_entity_block *OldBlock = World->FirstFree;
      if(OldBlock) {
        
        // NOTE(Egor): we pop the frist one, rise the next one to top
        World->FirstFree = World->FirstFree->Next;
      }
      else {
        
        // NOTE(Egor): there are no free blocks, use memory arena to create new one
        OldBlock = PushStruct(Arena, world_entity_block);
      }
      // NOTE(Egor): we just push filled up block deeper into a list
      *OldBlock = *Block;
      Block->Next = OldBlock;
      Block->EntityCount = 0;
    }
    
    Assert(Block->EntityCount < ArrayCount(Block->LowEntityIndex));
    Block->LowEntityIndex[Block->EntityCount++] = LowEntityIndex;
    
    int a = 3;
  }
}

inline world_position
ChunkPositionFromTilePosition(world *World, int32 AbsTileX, int32 AbsTileY, int32 AbsTileZ) {
  
  world_position Result = {};
  
  Result.ChunkX = AbsTileX / TILES_PER_CHUNK;
  Result.ChunkY = AbsTileY / TILES_PER_CHUNK;
  Result.ChunkZ = AbsTileZ / TILES_PER_CHUNK;
  
  Result.Offset_.X = (real32)(AbsTileX - (Result.ChunkX * TILES_PER_CHUNK)) * World->TileSideInMeters;
  Result.Offset_.Y = (real32)(AbsTileY - (Result.ChunkY * TILES_PER_CHUNK)) * World->TileSideInMeters;
  
  
  
  return Result;
}

