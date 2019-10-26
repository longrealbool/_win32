#include "game.h"
#include "game_render_group.cpp"
#include "game_world.cpp"
#include "game_sim_region.cpp"
#include "game_entity.cpp"



internal void
GameOutputSound(game_state *GameState, game_sound_output_buffer *SoundBuffer, int ToneHz)
{
  int16 ToneVolume = 3000;
  int WavePeriod = SoundBuffer->SamplesPerSecond/ToneHz;
  
  int16 *SampleOut = SoundBuffer->Samples;
  for(int SampleIndex = 0;
      SampleIndex < SoundBuffer->SampleCount;
      ++SampleIndex)
  {
    // TODO(Egor): Draw this out for people
#if 0
    real32 SineValue = sinf(GameState->tSine);
    int16 SampleValue = (int16)(SineValue * ToneVolume);
#else
    int16 SampleValue = 0;
#endif
    *SampleOut++ = SampleValue;
    *SampleOut++ = SampleValue;
    
#if 0
    GameState->tSine += 2.0f*Pi32*1.0f/(real32)WavePeriod;
    if(GameState->tSine > 2.0f*Pi32)
    {
      GameState->tSine -= 2.0f*Pi32;
    }
#endif
  }
}

#pragma pack(push, 1)
struct bitmap_header {
  uint16 FileType;     /* File type, always 4D42h ("BM") */
  uint32 FileSize;     /* Size of the file in bytes */
  uint16 Reserved1;    /* Always 0 */
  uint16 Reserved2;    /* Always 0 */
  uint32 BitmapOffset; /* Starting position of image data in bytes */
  uint32 Size;            /* Size of this header in bytes */
  int32 Width;           /* Image width in pixels */
  int32 Height;          /* Image height in pixels */
  uint16 Planes;          /* Number of color planes */
  uint16 BitsPerPixel;    /* Number of bits per pixel */
  /* Fields added for Windows 3.x follow this line */
  uint32 Compression;
  uint32 SizeOfBitmap;
  int32 HorzResolution;
  int32 VertResolution;
  uint32 ColorUser;
  uint32 ColorsImportant;
  /* bitmap winNT color masks */
  uint32 RedMask;
  uint32 GreenMask;
  uint32 BlueMask;
};
#pragma pack(pop)



internal loaded_bitmap
DEBUGLoadBMP(debug_platform_read_entire_file *ReadEntireFile,
             thread_context *Thread, char *FileName, int32 AlignX, int32 AlignY)
{
  
  loaded_bitmap Result = {};
  debug_read_file_result ReadResult = ReadEntireFile(Thread, FileName);
  
  if(ReadResult.ContentsSize != 0) {
    
    bitmap_header *Header = (bitmap_header *)ReadResult.Contents;
    
    // NOTE(Egor, bitmap_format): be aware that bitmap can have negative height for
    // top-down pictures, and there can be compression
    uint32 *Pixel = (uint32 *)((uint8 *)ReadResult.Contents + Header->BitmapOffset);
    Result.Memory = Pixel;
    Result.Height = Header->Height;
    Result.Width = Header->Width;

    
    Assert(Result.Height > 0);
    
    // NOTE(Egor): compute the ratios for eradicating meters to pixels from pipeline
    Result.WidthOverHeight = SafeRatio0((real32)Result.Width, (real32)Result.Height);
    // NOTE(Egor): this is constant, and it should stay this way
    real32 PtM = 1.0f / 42.0f;
    Result.NativeHeight = Result.Height * PtM;
    

    // NOTE(Egor): alignment
    v2 Align = V2i(AlignX, (Result.Height - 1) - AlignY);
    v2 AlignPercentage;
    AlignPercentage.x = SafeRatio0(Align.x, (real32)Result.Width); 
    AlignPercentage.y = SafeRatio0(Align.y, (real32)Result.Height);
    Result.AlignPercentage = AlignPercentage;
    
    // NOTE(Egor, bitmap_loading): we have to account all color masks in header,
    // in order to correctly load bitmaps saved with different software
    uint32 RedMask = Header->RedMask;
    uint32 GreenMask = Header->GreenMask;
    uint32 BlueMask = Header->BlueMask;
    uint32 AlphaMask = ~(RedMask | GreenMask | BlueMask);
    
    bit_scan_result AlphaScan = FindLeastSignificantSetBit(AlphaMask);
    bit_scan_result RedScan = FindLeastSignificantSetBit(RedMask);
    bit_scan_result GreenScan = FindLeastSignificantSetBit(GreenMask);
    bit_scan_result BlueScan = FindLeastSignificantSetBit(BlueMask);
    
    int32 AlphaShiftUp = 24;
    int32 AlphaShiftDown = (int32)AlphaScan.Index;
    int32 RedShiftUp = 16;
    int32 RedShiftDown = (int32)RedScan.Index;
    int32 GreenShiftUp = 8;
    int32 GreenShiftDown = (int32)GreenScan.Index;
    int32 BlueShiftUp = 0;
    int32 BlueShiftDown = (int32)BlueScan.Index;
    
    Assert(AlphaScan.Found);    
    Assert(RedScan.Found);
    Assert(GreenScan.Found);
    Assert(BlueScan.Found);
    
    uint32 *SourceDest = Pixel;
    
    for(int32 Y = 0; Y < Header->Height; ++Y) {
      for(int32 X = 0; X < Header->Width; ++X) {
        
        uint32 C = *SourceDest;
        
        v4 Texel = V4((real32)((C & RedMask) >> RedShiftDown),
                      (real32)((C & GreenMask) >> GreenShiftDown),
                      (real32)((C & BlueMask) >> BlueShiftDown),
                      (real32)((C & AlphaMask) >> AlphaShiftDown));
        
        // NOTE(Egor): when we will grab this data we want to get our color's in 
        // premultiplied format, so we store them in:
        
        // sRGB (non-premultiplied) --to_linear--> LinearRGB * LinearAlpha
        // --to_sRGB--> sRGB (premultiplied)
        
        Texel = SRGB255ToLinear1(Texel);
        
        Texel.rgb *= Texel.a;
        
        Texel = Linear1ToSRGB255(Texel);
        

        

        

        
        // when we will take it back from sRGB space to linear
        // they were already in premultiplied format
        
        
        *SourceDest = (((uint32)(Texel.a + 0.5f) << 24) |
                       ((uint32)(Texel.r + 0.5f) << 16) |
                       ((uint32)(Texel.g + 0.5f) << 8)  |
                       ((uint32)(Texel.b + 0.5f) << 0));
        
        SourceDest++;
      }
    }
    
    
    Result.Pitch = Result.Width*BITMAP_BYTES_PER_PIXEL;
#if 0
    
    // NOTE(Egor): bitmap was loaded bottom to top, so we reverse pitch (make him negative),
    // and shift Memory ptr to top of the image (bottom of the array)
    Result.Memory = (uint8 *)Result.Memory + (Result.Height-1)*(Result.Pitch);
    Result.Pitch = -Result.Width*BITMAP_BYTES_PER_PIXEL;
#endif
  }
  
  return Result;
}

internal loaded_bitmap
DEBUGLoadBMP(debug_platform_read_entire_file *ReadEntireFile,
             thread_context *Thread, char *FileName) {
  
  loaded_bitmap Result = DEBUGLoadBMP(ReadEntireFile, Thread, FileName, 0, 0);
  Result.AlignPercentage = V2(0.5f, 0.5f);
  return Result;
  
}

#if 0
internal void
DrawRectangleOutline(loaded_bitmap *Buffer, v2 vMin, v2 vMax,
                     v4 Color) {
  
  real32 r = 1.0f;
  
  DrawRectangle(Buffer, V2(vMin.x - r, vMin.y - r), V2(vMax.x + r, vMin.y + r), Color);
  DrawRectangle(Buffer, V2(vMin.x - r, vMin.y - r), V2(vMin.x + r, vMax.y + r), Color);
  
  DrawRectangle(Buffer, V2(vMin.x - r, vMax.y - r), V2(vMax.x + r, vMax.y + r), Color);
  DrawRectangle(Buffer, V2(vMax.x - r, vMax.y - r), V2(vMax.x + r, vMin.y + r), Color);
} 
#endif


struct add_low_entity_result {
  
  low_entity *Low;
  uint32 Index;
};



internal add_low_entity_result
AddLowEntity(game_state *GameState, entity_type EntityType, world_position *P) {
  
  Assert(GameState->LowEntityCount < ArrayCount(GameState->LowEntity));
  
  uint32 LowEntityIndex = GameState->LowEntityCount++;
  
  low_entity *LowEntity = GameState->LowEntity + LowEntityIndex;
  *LowEntity = {};
  LowEntity->Sim.Type = EntityType;
  LowEntity->Sim.Collision = GameState->NullCollision;
  
  // TODO(Egor): this is awfull
  if(P) {
    LowEntity->P = *P;
    // NOTE(Egor): maybe I should refactor this and not use Raw version
    ChangeEntityLocationRaw(&GameState->WorldArena, GameState->World, LowEntityIndex, 0, P);
  }
  else {
    
    LowEntity->P = NullPosition();
  }
  
  add_low_entity_result Result;
  Result.Low = LowEntity;
  Result.Index = LowEntityIndex;
  return Result;
}

inline void
InitHitPoints(low_entity *LowEntity, uint32 HitPointCount) {
  
  Assert(HitPointCount <= ArrayCount(LowEntity->Sim.HitPoint));
  
  LowEntity->Sim.HitPointMax = HitPointCount;
  
  for(uint32 i = 0; i < HitPointCount; ++i) {
    
    hit_point *HitPoint = LowEntity->Sim.HitPoint + i;
    HitPoint->Flags = 0;
    HitPoint->FilledAmount = SUB_HIT_POINT;
  }
  
}


internal add_low_entity_result
AddGroundedEntity(game_state *GameState, entity_type EntityType, world_position *P,
                  sim_entity_collision_volume_group *Collision) {
  
  add_low_entity_result Entity = AddLowEntity(GameState, EntityType, P);
  Entity.Low->Sim.Collision = Collision;
  
  return Entity;
}

inline world_position
ChunkPositionFromTilePosition(world *World, int32 AbsTileX, int32 AbsTileY,
                              int32 AbsTileZ, v3 AdditionalOffset = V3(0,0,0)) {
  
  real32 TileSideInMeters = 1.4f;
  real32 TileDepthInMeters = 3.0f;
  
  v3 TileDim = V3(TileSideInMeters, TileSideInMeters, TileDepthInMeters);
  
  v3 Offset = Hadamard(TileDim, V3((real32)AbsTileX, (real32)AbsTileY, (real32)AbsTileZ));
  
  world_position BasePos = {};
  // TODO(Egor): could produce floating point precision problems in future
  world_position Result = MapIntoChunkSpace(World, BasePos, Offset + AdditionalOffset);
  
  Assert(IsCanonical(World, Result.Offset_));
  return Result;
}


internal add_low_entity_result
AddWall(game_state *GameState, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ) {
  
  
  
  world_position P = ChunkPositionFromTilePosition(GameState->World, AbsTileX,
                                                   AbsTileY, AbsTileZ);
  add_low_entity_result Entity = AddGroundedEntity(GameState, EntityType_Wall, &P,
                                                   GameState->WallCollision);
  AddFlag(&Entity.Low->Sim, EntityFlag_Collides);
  
  return Entity;
}


internal add_low_entity_result
AddStair(game_state *GameState, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ) {
  
  world_position P = ChunkPositionFromTilePosition(GameState->World, AbsTileX, AbsTileY, AbsTileZ);
  add_low_entity_result Entity = AddGroundedEntity(GameState, EntityType_Stairwell, &P,
                                                   GameState->StairCollision);
  AddFlag(&Entity.Low->Sim, EntityFlag_Collides);
  // TODO(Egor): maybe I should reconsider this approach, and tie up to entity dim 
  Entity.Low->Sim.WalkableDim = Entity.Low->Sim.Collision->TotalVolume.Dim.xy;
  Entity.Low->Sim.WalkableHeight = GameState->FloorHeight;
  
  return Entity;
}


internal add_low_entity_result
AddSword(game_state *GameState) {
  
  add_low_entity_result Entity = AddLowEntity(GameState, EntityType_Sword, 0);
  Entity.Low->Sim.Collision = GameState->SwordCollision;
  AddFlag(&Entity.Low->Sim, EntityFlag_NonSpatial | EntityFlag_Moveable | EntityFlag_Collides);
  
  return Entity;
}


internal add_low_entity_result
AddPlayer(game_state *GameState) {
  
  world_position P = GameState->CameraP;
  add_low_entity_result Entity = AddGroundedEntity(GameState, EntityType_Hero, &P,
                                                   GameState->PlayerCollision);
  AddFlag(&Entity.Low->Sim, EntityFlag_Collides | EntityFlag_Moveable);
  InitHitPoints(Entity.Low, 3);
  add_low_entity_result Sword = AddSword(GameState);
  Entity.Low->Sim.Sword.Index = Sword.Index;
  
  // NOTE(Egor): if camera doesn't follows any entity, make it follow this
  if(GameState->CameraFollowingEntityIndex == 0) {
    
    GameState->CameraFollowingEntityIndex = Entity.Index;
  }
  
  return Entity;
}

internal add_low_entity_result
AddSpace(game_state *GameState, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ) {
  
  world_position P = ChunkPositionFromTilePosition(GameState->World, AbsTileX, AbsTileY, AbsTileZ);
  add_low_entity_result Entity = AddGroundedEntity(GameState, EntityType_Space, &P,
                                                   GameState->StandardRoomCollision);
  AddFlag(&Entity.Low->Sim, EntityFlag_Traversable);
  
  return Entity;
}


internal add_low_entity_result
AddMonster(game_state *GameState, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ) {
  
  world_position P = ChunkPositionFromTilePosition(GameState->World, AbsTileX, AbsTileY, AbsTileZ);
  add_low_entity_result Entity = AddGroundedEntity(GameState, EntityType_Monster, &P,
                                                   GameState->MonsterCollision);
  AddFlag(&Entity.Low->Sim, EntityFlag_Collides);
  InitHitPoints(Entity.Low, 3);
  
  return Entity;
}


internal add_low_entity_result
AddFamiliar(game_state *GameState, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ) {
  
  v3 Dim = V3(1.0f, 0.5f, 0.5f);
  world_position P = ChunkPositionFromTilePosition(GameState->World, AbsTileX,
                                                   AbsTileY, AbsTileZ);
  add_low_entity_result Entity = AddGroundedEntity(GameState, EntityType_Familiar, &P,
                                                   GameState->FamiliarCollision);
  
  AddFlag(&Entity.Low->Sim, EntityFlag_Moveable);
  AddFlag(&Entity.Low->Sim, EntityFlag_Collides);
  
  return Entity;
}

#if 0
internal void 
SimCameraRegion(game_state *GameState) {
  
  world *World = GameState->World;
  
  uint32 TileSpanX = 17*3;
  uint32 TileSpanY = 9*3;
  
  rectangle2 CameraBounds = RectCenterDim(V2(0,0),
                                          World->TileSideInMeters*V2((real32)TileSpanX,
                                                                     (real32)TileSpanY));
  sim_region *SimRegion = BeginSim(SimArena, GameState->World, GameState->CameraP, CameraBounds);
  EndSim(SimRegion, GameState);
  
}
#endif



internal void
DrawHitpoints(sim_entity *Entity, render_group *PieceGroup) {
  
  if(Entity->HitPointMax) {
    
    v2 HitPointDim = {0.2f , 0.2f};
    real32 SpacingX = 2.0f * HitPointDim.x;
    real32 FirstX = (Entity->HitPointMax - 1)*0.5f*SpacingX;
    v2 HitP = {(Entity->HitPointMax - 1)*-0.5f*SpacingX, -0.4f};
    v2 dHitP = {SpacingX, 0};
    
    for(uint32 HPIndex = 0; HPIndex < Entity->HitPointMax; ++HPIndex) {
      
      hit_point *HitPoint = Entity->HitPoint + HPIndex;
      
      v4 Color = {1.0f , 0.0f, 0.0f, 1.0f};
      if(HitPoint->FilledAmount == 0) {
        
        Color = V4(0.2f, 0.2f, 0.2f, 1.0f);
      }
      
      PushRect(PieceGroup, ToV3(HitP, 0.0f), HitPointDim, Color); 
      HitP += dHitP;
    }
  }
}


internal sim_entity_collision_volume_group*
MakeNullCollision(game_state *GameState) {
  
  // TODO(Egor): ONLY TEMPORARY, should use another arena
  sim_entity_collision_volume_group *Group = PushStruct(&GameState->WorldArena,
                                                        sim_entity_collision_volume_group);
  Group->VolumeCount = 0;
  Group->Volumes = 0;
  Group->TotalVolume.Dim = V3(0, 0, 0);
  Group->TotalVolume.OffsetP = V3(0, 0, 0);
  
  return Group;
}


internal sim_entity_collision_volume_group*
MakeSimpleGroundedCollision(game_state *GameState, real32 DimX, real32 DimY, real32 DimZ) {
  
  // TODO(Egor): ONLY TEMPORARY, should use another arena
  sim_entity_collision_volume_group *Group = PushStruct(&GameState->WorldArena,
                                                        sim_entity_collision_volume_group);
  
  Group->VolumeCount = 1;
  Group->Volumes = PushArray(&GameState->WorldArena,
                             Group->VolumeCount, sim_entity_collision_volume);
  
  Group->TotalVolume.Dim = V3(DimX, DimY, DimZ);
  Group->TotalVolume.OffsetP = V3(0, 0, 0.5f * DimZ);
  Group->Volumes[0] = Group->TotalVolume;
  
  return Group;
}


internal void
FillGroundChunk(transient_state *TranState, game_state *GameState,
                ground_buffer *GroundBuffer, world_position *Pos) {
  
  temporary_memory GroundMemory = BeginTemporaryMemory(&TranState->TranArena);
  // TODO(Egor): ground chunk resolution
  loaded_bitmap *Buffer =  &GroundBuffer->Bitmap;
  
  Buffer->AlignPercentage = V2(0.5f, 0.5f);
  Buffer->WidthOverHeight = 1.0f;
  
  render_group *GroundGroup = AllocateRenderGroup(&TranState->TranArena, Megabytes(4), V2i(Buffer->Width, Buffer->Height));
  GroundBuffer->P = *Pos;
  
  Clear(GroundGroup, V4(1.0f, 0.4f, 0.0f, 1.0f));
  
#if 0
  if(Pos->ChunkZ == 0) {
    // NOTE(Egor): the idea is: 
    // we take block that we want (Pos), generate 3x3 block centered at (Pos)
    // with rigid order, allowing to bitmaps blits over the top of our block generating
    // seemless texture and cut the center block - effectively our (Pos)
    
    real32 Width = GameState->World->ChunkDimInMeters.x;
    real32 Height = GameState->World->ChunkDimInMeters.y;
    
    v2 HalfDim = 0.5f*V2(Width, Height);
    HalfDim *= 2.0f;
    
    for(int32 ChunkOffsetY = -1; ChunkOffsetY <= 1; ++ChunkOffsetY) {
      for(int32 ChunkOffsetX = -1; ChunkOffsetX <= 1; ++ChunkOffsetX) {
        
        int32 ChunkX = Pos->ChunkX + ChunkOffsetX;
        int32 ChunkY = Pos->ChunkY + ChunkOffsetY;
        int32 ChunkZ = Pos->ChunkZ;
        
        // TODO(Egor): this is nuts, make sane spatial hashing
        random_series Series = Seed(12*ChunkX + 34*ChunkY + 57*ChunkZ);
        
        v2 Center = V2(ChunkOffsetX*Width, ChunkOffsetY*Height);
        
        loaded_bitmap *Stamp = 0;
        for(uint32 Index = 0; Index < 50; ++Index) {
          
          if(RandomChoice(&Series, 2)) {
            
            Stamp = GameState->Grass + RandomChoice(&Series,  ArrayCount(GameState->Grass));
          }
          else {
            
            Stamp = GameState->Stones + RandomChoice(&Series, ArrayCount(GameState->Stones));
          }
          
          v2 P = Center + Hadamard(HalfDim, V2(RandomBilateral(&Series),
                                               RandomBilateral(&Series)));
          PushBitmap(GroundGroup, Stamp, ToV3(P, 0.0f), 5.0f);
        }
        PushRect(GroundGroup, V3(0, 0, 0), V2(2,2), V4(1.0f, 0, 0, 1));
      }
      
      for(int32 ChunkOffsetY = -1; ChunkOffsetY <= 1; ++ChunkOffsetY) {
        for(int32 ChunkOffsetX = -1; ChunkOffsetX <= 1; ++ChunkOffsetX) {
          
          int32 ChunkX = Pos->ChunkX + ChunkOffsetX;
          int32 ChunkY = Pos->ChunkY + ChunkOffsetY;
          int32 ChunkZ = Pos->ChunkZ;
          
          // TODO(Egor): this is nuts, make sane spatial hashing
          random_series Series = Seed(12*ChunkX + 34*ChunkY + 57*ChunkZ);
          v2 Center = V2(ChunkOffsetX*Width, ChunkOffsetY*Height);
          
          loaded_bitmap *Stamp = 0;
          
          for(uint32 Index = 0; Index < 5; ++Index) {
            
            Stamp = GameState->Tuft + RandomChoice(&Series, ArrayCount(GameState->Tuft));
            
            v2 P = Center + Hadamard(HalfDim, V2(RandomBilateral(&Series),
                                                 RandomBilateral(&Series)));
            PushBitmap(GroundGroup, Stamp, ToV3(P, 0.0f), 1.0f);
          }
        }
      }
    }
  }
  #endif

  
  TiledRenderPushBuffer(TranState->RenderQueue, GroundGroup, Buffer);
  EndTemporaryMemory(GroundMemory);
}

internal void
ClearBitmap(loaded_bitmap *Bitmap) {
  
  if(Bitmap->Memory) {
    uint32 TotalBitmapSize = Bitmap->Width*Bitmap->Height*BITMAP_BYTES_PER_PIXEL;
    ZeroSize(TotalBitmapSize, Bitmap->Memory);
  }
}


internal loaded_bitmap
MakeEmptyBitmap(memory_arena *Arena, uint32 Width, uint32 Height, bool32 ClearToZero = true) {
  
  loaded_bitmap Result = {};
  
  Result.Width = Width;
  Result.Height = Height;
  Result.Pitch = Result.Width * BITMAP_BYTES_PER_PIXEL;
  uint32 TotalBitmapSize = Result.Width*Result.Height * BITMAP_BYTES_PER_PIXEL;
  Result.Memory = PushSize(Arena, TotalBitmapSize);
  if(ClearToZero) {
    ClearBitmap(&Result);
  }
  
  return Result;
}

internal void
MakeSphereNormalMap(loaded_bitmap *Bitmap, real32 Roughness, real32 NxC, real32 NyC) {
  
  real32 InvWidth = 1.0f/(Bitmap->Width - 1.0f);
  real32 InvHeight = 1.0f/(Bitmap->Height - 1.0f);
  
  uint8 *Row = (uint8 *)Bitmap->Memory;
  
  for(int32 Y = 0; Y < Bitmap->Height; ++Y) {
    
    uint32 *Pixel = (uint32 *)Row;
    for(int32 X = 0; X < Bitmap->Width; ++X) {
      
      v2 BitmapUV = V2(InvWidth*(real32)X, InvHeight*(real32)Y);
      
      real32 Nx = 2.0f*BitmapUV.x - 1.0f;
      real32 Ny = 2.0f*BitmapUV.y - 1.0f;
      
      Nx *= NxC;
      Ny *= NyC;
      
      real32 RootTerm = 1.0f - Square(Nx) - Square(Ny);
      
      
      v3 Normal = V3(0, 0.70710678f, 0.70710678f);
      //      v3 Normal = V3(0, 0, 1);
      if(RootTerm >= 0.0f) {
        
        real32 Nz = SquareRoot(RootTerm); 
        Normal = V3(Nx, Ny, Nz); 
      }
      
      // TODO(Egor): maybe this is wrong
      v4 Color = V4(255.0f*(0.5f*(Normal.x + 1.0f)),
                    255.0f*(0.5f*(Normal.y + 1.0f)),
                    255.0f*(0.5f*(Normal.z + 1.0f)),
                    255.0f*Roughness);
      
      uint32 PixNormal = (((uint32)(Color.a + 0.5f) << 24) |
                          ((uint32)(Color.r + 0.5f) << 16) |
                          ((uint32)(Color.g + 0.5f) << 8)  |
                          ((uint32)(Color.b + 0.5f) << 0));
      
      
      *Pixel++ = PixNormal;
    }
    
    Row += Bitmap->Pitch;
  }
}

internal void
MakeSphereDiffuseMap(loaded_bitmap *Bitmap, real32 NxC = 1.0f, real32 NyC = 1.0f) {
  
  real32 InvWidth = 1.0f/(Bitmap->Width - 1.0f);
  real32 InvHeight = 1.0f/(Bitmap->Height - 1.0f);
  
  v3 BaseColor = V3(0, 0, 0);
  
  uint8 *Row = (uint8 *)Bitmap->Memory;
  
  for(int32 Y = 0; Y < Bitmap->Height; ++Y) {
    
    uint32 *Pixel = (uint32 *)Row;
    for(int32 X = 0; X < Bitmap->Width; ++X) {
      
      v2 BitmapUV = V2(InvWidth*(real32)X, InvHeight*(real32)Y);
      
      real32 Nx = 2.0f*BitmapUV.x - 1.0f;
      real32 Ny = 2.0f*BitmapUV.y - 1.0f;
      
      Nx *= NxC;
      Ny *= NyC;
      
      real32 Alpha = 0.0f;
      real32 RootTerm = 1.0f - Square(Nx) - Square(Ny);
      if(RootTerm >= 0.0f) {
        
        Alpha = 1.0f;
      }
      
      Alpha *= 255.0f;
      // TODO(Egor): maybe this is wrong
      v4 Color = V4((Alpha*BaseColor.r),
                    (Alpha*BaseColor.g),
                    (Alpha*BaseColor.b),
                    (Alpha));
      
      uint32 PixNormal = (((uint32)(Color.a + 0.5f) << 24) |
                          ((uint32)(Color.r + 0.5f) << 16) |
                          ((uint32)(Color.g + 0.5f) << 8)  |
                          ((uint32)(Color.b + 0.5f) << 0));
      
      
      *Pixel++ = PixNormal;
    }
    
    Row += Bitmap->Pitch;
  }
}


internal void
MakePyramidNormalMap(loaded_bitmap *Bitmap, real32 Roughness) {
  
  real32 InvWidth = 1.0f/(Bitmap->Width - 1.0f);
  real32 InvHeight = 1.0f/(Bitmap->Height - 1.0f);
  
  
  
  uint8 *Row = (uint8 *)Bitmap->Memory;
  
  for(int32 Y = 0; Y < Bitmap->Height; ++Y) {
    
    uint32 *Pixel = (uint32 *)Row;
    for(int32 X = 0; X < Bitmap->Width; ++X) {
      
      real32 Seven = 0.70710678f;
      v3 Normal = V3(0, 0, Seven);
      int32 InvX = (Bitmap->Width - 1) - X;
      if(X < Y) {
        
        if(InvX < Y) {
          
          Normal.y = Seven;
          
        }
        else {
          
          Normal.x = -Seven;
          
        }
      }
      else {
        
        if(InvX < Y) {
          
          Normal.x = Seven;
          
        }
        else {
          
          Normal.y = -Seven;
          
        }
      }
      
      // TODO(Egor): maybe this is wrong
      v4 Color = V4(255.0f*(0.5f*(Normal.x + 1.0f)),
                    255.0f*(0.5f*(Normal.y + 1.0f)),
                    255.0f*(0.5f*(Normal.z + 1.0f)),
                    255.0f*Roughness);
      
      uint32 PixNormal = (((uint32)(Color.a + 0.5f) << 24) |
                          ((uint32)(Color.r + 0.5f) << 16) |
                          ((uint32)(Color.g + 0.5f) << 8)  |
                          ((uint32)(Color.b + 0.5f) << 0));
      
      
      *Pixel++ = PixNormal;
    }
    
    Row += Bitmap->Pitch;
  }
}


internal void
MakeSphereNormalMapDebug(loaded_bitmap *Bitmap, real32 Roughness) {
  
  static int32 StaticX = 0;
  static int32 StaticY = 0;
  
  for(uint32 Counter = 0; Counter < 81*4; ++Counter) {
    int32 Y = StaticY;
    int32 X = StaticX;
    
    real32 InvWidth = 1.0f/(Bitmap->Width-1);
    real32 InvHeight = 1.0f/(Bitmap->Height-1);
    
    uint8 *Row = (uint8 *)Bitmap->Memory + Bitmap->Pitch*Y;
    
    uint32 *Pixel = (uint32 *)Row + X;
    
    v2 BitmapUV = V2(InvWidth*(real32)(X + 1), InvHeight*(real32)(Y + 1));
    
#if 1
    real32 Nx = 2.0f*BitmapUV.x - 1.0f;
    real32 Ny = 2.0f*BitmapUV.y - 1.0f;
    real32 RootTerm = 1.0f - Square(Nx) - Square(Ny);
    v3 Normal = V3(0, 0, 1);
    if(RootTerm >= 0.0f) {
      
      real32 Nz = SquareRoot(RootTerm); 
      Normal = V3(Nx, Ny, Nz); 
    }
    
#else
    v3 Normal = V3(2.0f*BitmapUV.x - 1.0f, 2.0f*BitmapUV.y - 1.0f, 0.0f); 
    // TODO(Egor): this is ugly hack
    Normal.z = SquareRoot(1.0f - Min(1.0f, Square(Normal.x) + Square(Normal.y)));
#endif
    
    // TODO(Egor): maybe this is wrong
    v4 Color = V4(255.0f*(0.5f*(Normal.x + 1.0f)),
                  255.0f*(0.5f*(Normal.y + 1.0f)),
                  255.0f*(0.5f*(Normal.z + 1.0f)),
                  255.0f*Roughness);
    
    uint32 PixNormal = (((uint32)(Color.a + 0.5f) << 24) |
                        ((uint32)(Color.r + 0.5f) << 16) |
                        ((uint32)(Color.g + 0.5f) << 8)  |
                        ((uint32)(Color.b + 0.5f) << 0));
    
    
    
    
    
    *Pixel = PixNormal;
    
    
    StaticX++;
    if(StaticX == Bitmap->Width) {
      StaticY++;
      StaticX = 0;
    }
    if(StaticY == Bitmap->Height) {
      StaticY = 0;
      StaticX = 0;
    }
    
  }
}


#if 0
internal void 
RequestGroundBuffers(world_position CenterP, rectangle3 Bounds) {
  
  Bounds = Offset(Bounds, CenterP.Offset_);
  CenterP.Offset_ = V3(0, 0, 0);
  
  for(uint32 ) {
    
    
  }
  
  FillGroundChunk(TranState, GameState, TranState->GroundBuffers,
                  &GameState->CameraP);
}
#endif


internal v2
ConvertToBottomUpAlign(loaded_bitmap *Bitmap, v2 Align) {
  
  Align.y = (Bitmap->Height - 1) - Align.y;
  return Align;
}


#if GAME_INTERNAL
game_memory *DebugGlobalMemory; 
#endif

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
  
  PlatformAddEntry = Memory->PlatformAddEntry;
  PlatformCompleteAllWork = Memory->PlatformCompleteAllWork;
#if GAME_INTERNAL
  DebugGlobalMemory = Memory; 
#endif
  
  BEGIN_TIMED_BLOCK(GameUpdateAndRender);
  
  Assert((&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0]) ==
         (ArrayCount(Input->Controllers[0].Buttons)));
  
  Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
  game_state *GameState = (game_state *)Memory->PermanentStorage;
  
  uint32 GroundBufferWidth = 256;
  uint32 GroundBufferHeight = 256;
  real32 PtM = 1.0f/ 42.0f;
  
  if(!Memory->IsInitialized)
  {
    
    uint32 TilesPerHeight = 9;
    uint32 TilesPerWidth = 17;
    
    InitializeArena(&GameState->WorldArena, Memory->PermanentStorageSize - sizeof(game_state),
                    (uint8 *)Memory->PermanentStorage + sizeof(game_state));
    
    
    GameState->World = PushStruct(&GameState->WorldArena, world);
    world *World = GameState->World;

    
    GameState->FloorHeight = 3.0f;
    
    real32 TileSideInMeters = 1.4f;
    

    v3 WorldChunkDimInMeters = V3((real32)GroundBufferWidth*PtM,
                                  (real32)GroundBufferHeight*PtM,
                                  (real32)GameState->FloorHeight);
    
    InitializeWorld(World, WorldChunkDimInMeters);
    
    AddLowEntity(GameState, EntityType_Null, 0);
    // TODO(Egor): This may be more appropriate to do in the platform layer
    
    
    GameState->NullCollision = MakeNullCollision(GameState);
    GameState->SwordCollision = MakeSimpleGroundedCollision(GameState, 1.0f, 0.5f, 0.1f);
    GameState->PlayerCollision = MakeSimpleGroundedCollision(GameState, 1.0f, 0.5f, 1.2f);
    GameState->MonsterCollision = MakeSimpleGroundedCollision(GameState,1.0f, 0.5f, 0.5f);
    GameState->FamiliarCollision = MakeSimpleGroundedCollision(GameState,1.0f, 0.5f, 0.5f);
    
    GameState->WallCollision = MakeSimpleGroundedCollision(GameState, 
                                                           TileSideInMeters,
                                                           TileSideInMeters,
                                                           GameState->FloorHeight);
    
    GameState->StairCollision = MakeSimpleGroundedCollision(GameState,
                                                            TileSideInMeters,
                                                            2.0f * TileSideInMeters,
                                                            1.1f * GameState->FloorHeight);
    
    GameState->StandardRoomCollision = MakeSimpleGroundedCollision(GameState, 
                                                                   TilesPerWidth * TileSideInMeters,
                                                                   TilesPerHeight * TileSideInMeters,
                                                                   0.9f * GameState->FloorHeight);
    
    
    GameState->Backdrop = DEBUGLoadBMP(Memory->DEBUGPlatformReadEntireFile, Thread, "..//source//assets//background.bmp");
    GameState->Sword = DEBUGLoadBMP(Memory->DEBUGPlatformReadEntireFile, Thread, "..//source//assets//dagger.bmp", 30, 30);
    GameState->Tree1 = DEBUGLoadBMP(Memory->DEBUGPlatformReadEntireFile, Thread, "..//..//test//tree00.bmp");
    
    GameState->Tree = DEBUGLoadBMP(Memory->DEBUGPlatformReadEntireFile, Thread, "..//source//assets//tree1.bmp", 44, 70);
//    GameState->Tree = GameState->Tree1;
    
    GameState->Stairwell = DEBUGLoadBMP(Memory->DEBUGPlatformReadEntireFile, Thread, "..//source//assets//arrow_up.bmp");
    
    hero_bitmaps *Bitmap = GameState->Hero;
    
    Bitmap->HeroHead = 
      DEBUGLoadBMP(Memory->DEBUGPlatformReadEntireFile, Thread, "..//source//assets//figurine.bmp", 51, 112);
    Bitmap->HeroCape = 
      DEBUGLoadBMP(Memory->DEBUGPlatformReadEntireFile, Thread, "..//source//assets//arrow_right.bmp", 51, 112);
    Bitmap->Align = ConvertToBottomUpAlign(&Bitmap->HeroHead,V2(51, 112));
    
    
    Bitmap[1] = Bitmap[0];
    Bitmap[1].HeroCape = 
      DEBUGLoadBMP(Memory->DEBUGPlatformReadEntireFile, Thread, "..//source//assets//arrow_up.bmp", 51, 112);
    
    Bitmap[2] = Bitmap[0];
    Bitmap[2].HeroCape = 
      DEBUGLoadBMP(Memory->DEBUGPlatformReadEntireFile, Thread, "..//source//assets//arrow_left.bmp", 51, 112);
    
    Bitmap[3] = Bitmap[0];
    Bitmap[3].HeroCape = 
      DEBUGLoadBMP(Memory->DEBUGPlatformReadEntireFile, Thread, "..//source//assets//arrow_down.bmp", 51, 112);
    
    
    //GameState->Tree = DEBUGLoadBMP(Memory->DEBUGPlatformReadEntireFile, Thread, "..//..//test//tree00.bmp");
    
#if 1
    
    loaded_bitmap *Stones = GameState->Stones;
    loaded_bitmap *Grass = GameState->Grass;
    loaded_bitmap *Tuft = GameState->Tuft;
    
    Grass[0] = DEBUGLoadBMP(Memory->DEBUGPlatformReadEntireFile, Thread, "..//..//test//grass00.bmp");
    Grass[1] = DEBUGLoadBMP(Memory->DEBUGPlatformReadEntireFile, Thread, "..//..//test//grass01.bmp");
    
    Stones[0] = DEBUGLoadBMP(Memory->DEBUGPlatformReadEntireFile, Thread, "..//..//test//ground00.bmp");    
    Stones[1] = DEBUGLoadBMP(Memory->DEBUGPlatformReadEntireFile, Thread, "..//..//test//ground01.bmp");    
    Stones[2] = DEBUGLoadBMP(Memory->DEBUGPlatformReadEntireFile, Thread, "..//..//test//ground02.bmp");    
    Stones[3] = DEBUGLoadBMP(Memory->DEBUGPlatformReadEntireFile, Thread, "..//..//test//ground03.bmp");    
    
    
    Tuft[0] = DEBUGLoadBMP(Memory->DEBUGPlatformReadEntireFile, Thread, "..//..//test//tuft00.bmp");    
    Tuft[1] = DEBUGLoadBMP(Memory->DEBUGPlatformReadEntireFile, Thread, "..//..//test//tuft00.bmp");    
    Tuft[2] = DEBUGLoadBMP(Memory->DEBUGPlatformReadEntireFile, Thread, "..//..//test//tuft00.bmp");    
    
#else
    
    loaded_bitmap *Slump = GameState->Slumps;
    Slump[0] = DEBUGLoadBMP(Memory->DEBUGPlatformReadEntireFile, Thread, "..//source//assets//slump00.bmp");    
    Slump[1] = DEBUGLoadBMP(Memory->DEBUGPlatformReadEntireFile, Thread, "..//source//assets//slump01.bmp");    
    Slump[2] = DEBUGLoadBMP(Memory->DEBUGPlatformReadEntireFile, Thread, "..//source//assets//slump02.bmp");    
    Slump[3] = DEBUGLoadBMP(Memory->DEBUGPlatformReadEntireFile, Thread, "..//source//assets//slump03.bmp");    
    
    
#endif 
    
    random_series Series = Seed(1113);
    
    int32 ScreenBaseX = 0;
    int32 ScreenBaseY = 0;
    int32 ScreenBaseZ = 0;
    int32 ScreenY = ScreenBaseY;
    int32 ScreenX = ScreenBaseX;
    int32 AbsTileZ = ScreenBaseZ;
    
    bool32 DoorToRight = false;
    bool32 DoorToLeft = false;
    bool32 DoorToTop = false;
    bool32 DoorToBottom = false;    
    bool32 DoorUp = false;
    bool32 DoorDown = false;
    
    uint32 CounterOfTwo = 0;
    
    for(uint32 ScreenIndex = 0; ScreenIndex < 100; ++ScreenIndex) {
      
#if 0
      // TODO(Egor, TileMap): get again through logic of creating tilemap
      uint32 RandomChoice;
      
      if(DoorUp || DoorDown) {
        Choice = RollTheDice() % 2;
      }
      else {
        Choice = RollTheDice() % 3;
      }
#else
      
      uint32 Choice = RandomChoice(&Series, ((DoorUp || DoorDown)? 2 : 4));
      Choice = 1;
#endif
      //Choice = 3;
      
      // NOTE(Egor): keep track of is there a need to create another stairwell
      bool32 CreatedZDoor = false;
      if(Choice == 3) {
        
        CreatedZDoor = true;
        DoorDown = true;
      }
      else if(Choice == 2) {
        
        CreatedZDoor = true;
        DoorUp = true;
        
      }
      else if(Choice == 0) {
        
        DoorToRight = true;
      }
      else {
        
        DoorToTop = true;
      }
      
      AddSpace(GameState,
               ScreenX*TilesPerWidth + TilesPerWidth/2,
               ScreenY*TilesPerHeight + TilesPerHeight/2,
               AbsTileZ);
      
      for(uint32 TileY = 0; TileY < TilesPerHeight; ++TileY) {
        for(uint32 TileX = 0; TileX < TilesPerWidth; ++TileX) {
          
          uint32 AbsTileY = ScreenY*TilesPerHeight + TileY;
          uint32 AbsTileX = ScreenX*TilesPerWidth + TileX;
          
          bool32 ShouldBeWall = false;
          
          if(TileX == 0 || TileX == (TilesPerWidth - 1)) {
            
            ShouldBeWall = true;
            if(DoorToRight && TileX == TilesPerWidth - 1) {
              if(TileY == TilesPerHeight/2)
                ShouldBeWall = false;
            }
            
            if(DoorToLeft && TileX == 0 ) {
              if(TileY == TilesPerHeight/2)
                ShouldBeWall = false;
            }
          }
          
          if(TileY == 0 || TileY == (TilesPerHeight - 1)) {
            
            ShouldBeWall = true;
            if(DoorToTop && TileY == TilesPerHeight - 1) {
              if(TileX == TilesPerWidth/2)
                ShouldBeWall = false;
            }
            
            if(DoorToBottom && TileY == 0) {
              if(TileX == TilesPerWidth/2)
                ShouldBeWall = false;
            }
          }
          
          if(ShouldBeWall) {
            AddWall(GameState, AbsTileX, AbsTileY, AbsTileZ);
          }
          else if(CreatedZDoor) {
            if((((AbsTileZ % 2) && TileY == 4 && TileX == 13)) ||
               (!(AbsTileZ % 2) && TileY == 4 && TileX == 5)) {
              
              AddStair(GameState, AbsTileX, AbsTileY, DoorDown ? AbsTileZ - 1 : AbsTileZ);
            }
          }
        }
      }
      
      DoorToLeft = DoorToRight;
      DoorToBottom = DoorToTop;    
      DoorToRight = false;
      DoorToTop = false;
      
      if(CreatedZDoor) {
        
        DoorDown = !DoorDown;
        DoorUp = !DoorUp;
      }
      else {
        
        DoorUp = false;
        DoorDown = false;
      }
      
      if(Choice == 3) {
        
        AbsTileZ -= 1;
      }
      if(Choice == 2) {
        
        AbsTileZ += 1;
      }
      else if(Choice == 0) {
        
        ScreenX++;
      }
      else if(Choice == 1) {
        
        ScreenY++;
      }
      
    }
    
    world_position NewCameraP = {};
    uint32 CameraTileX = ScreenBaseX*TilesPerWidth + 17/2;
    uint32 CameraTileY = ScreenBaseY*TilesPerHeight + 9/2;
    uint32 CameraTileZ = ScreenBaseZ;
    NewCameraP = ChunkPositionFromTilePosition(GameState->World, CameraTileX, CameraTileY,
                                               CameraTileZ);
    GameState->CameraP = NewCameraP;
    
    AddMonster(GameState, CameraTileX + 20, CameraTileY + 2, CameraTileZ);
    AddWall(GameState, CameraTileX, CameraTileY - 2, CameraTileZ);
    //AddFamiliar(GameState, CameraTileX, CameraTileY - 2, CameraTileZ);
    
    
    Memory->IsInitialized = true;
  }
  
  ///////////////////////////////////////////////////////////////////////////
  // NOTE(Egor): transient state initialization   
  ///////////////////////////////////////////////////////////////////////////
  Assert(sizeof(transient_state) <= Memory->TransientStorageSize);
  transient_state *TranState = (transient_state *)Memory->TransientStorage;
  
  if(!TranState->Initialized){
    
    InitializeArena(&TranState->TranArena, Memory->TransientStorageSize - sizeof(transient_state),
                    (uint8 *)Memory->TransientStorage + sizeof(transient_state));
    
    
    TranState->RenderQueue = Memory->RenderQueue;
    
    TranState->GroundBufferCount = 16;
    TranState->GroundBuffers = PushArray(&TranState->TranArena,
                                         TranState->GroundBufferCount, ground_buffer); 
    
    for(uint32 GroundBufferIndex = 0;
        GroundBufferIndex < TranState->GroundBufferCount;
        ++GroundBufferIndex) {
      
      ground_buffer *GroundBuffer = TranState->GroundBuffers + GroundBufferIndex;
      GroundBuffer->Bitmap = MakeEmptyBitmap(&TranState->TranArena,
                                             GroundBufferWidth,
                                             GroundBufferHeight, false);
      GroundBuffer->P = NullPosition();
      
      TranState->EnvMapWidth = 512;
      TranState->EnvMapHeight = 256;
      for(uint32 MapIndex = 0; MapIndex < ArrayCount(TranState->EnvMaps); ++MapIndex) {
        
        environment_map *Map = TranState->EnvMaps + MapIndex;
        uint32 Width = TranState->EnvMapWidth;
        uint32 Height = TranState->EnvMapHeight;
        
        for(uint32 LODIndex = 0; LODIndex < ArrayCount(Map->LOD); ++LODIndex) {
          
          Map->LOD[LODIndex] = MakeEmptyBitmap(&TranState->TranArena, Width, Height, false);
          Width >>= 1;
          Height >>= 1;
        }
      }
    }
    
    GameState->TestDiffuse = MakeEmptyBitmap(&TranState->TranArena, 256, 256, false);
#if 0
    DrawRectangle(&GameState->TestDiffuse, V2(0,0),
                  V2i(GameState->TestDiffuse.Width, GameState->TestDiffuse.Height), V4(0.5f, 0.5f, 0.5f, 1.0f));
#endif
    
    GameState->TestNormal = MakeEmptyBitmap(&TranState->TranArena,
                                            GameState->TestDiffuse.Width,
                                            GameState->TestDiffuse.Height);
    MakeSphereNormalMap(&GameState->TestNormal, 0.0f, 1.0f, 1.0f);
    MakeSphereDiffuseMap(&GameState->TestDiffuse, 1.0f, 1.0f);
    //    MakePyramidNormalMap(&GameState->TestNormal, 0.0f);
    
    TranState->Initialized = true;
  }
  
  // NOTE(Egor): debug code
  if(Input->ExecutableReloaded) {
    
    for(uint32 GroundBufferIndex = 0;
        GroundBufferIndex < TranState->GroundBufferCount;
        ++GroundBufferIndex) {
      
      ground_buffer *GroundBuffer = TranState->GroundBuffers + GroundBufferIndex;
      GroundBuffer->P = NullPosition();
    }
  }
  
  ///////////////////////////////////////////////////////////////////////////
  // NOTE(Egor): end of initialization   
  ///////////////////////////////////////////////////////////////////////////
  
  
  world *World = GameState->World;
  
  // TODO(Egor): put this into renderer in the future
  
  for(int ControllerIndex = 0;
      ControllerIndex < ArrayCount(Input->Controllers);
      ++ControllerIndex)
  {
    
    game_controller_input *Controller = GetController(Input, ControllerIndex);
    controlled_entity *Controlled = GameState->ControlledEntities + ControllerIndex;
    
    if(Controlled->EntityIndex == 0) {
      
      if(Controller->Back.EndedDown) {
        
        *Controlled = {};
        Controlled->EntityIndex = AddPlayer(GameState).Index;
      }
    }
    else {
      
      Controlled->dZ = 0;
      Controlled->ddP = {};
      Controlled->dSword = {};
      if(Controller->IsAnalog)
      {
        // analog movement
        Controlled->ddP = v2{Controller->StickAverageX, Controller->StickAverageY};
      }
      else
      {
        
        if(Controller->MoveUp.EndedDown) {
          Controlled->ddP.y = 1.0f;
        }
        if(Controller->MoveRight.EndedDown) {
          Controlled->ddP.x = 1.0f;
        }
        if(Controller->MoveDown.EndedDown) {
          Controlled->ddP.y = -1.0f;
        }
        if(Controller->MoveLeft.EndedDown) {
          Controlled->ddP.x = -1.0f;
        }
      }
      
      if(Controller->Start.EndedDown) {
        Controlled->dZ = 3.0f;
      }
      
#if 0
      Controlled->dSword = {};
      if(Controller->ActionUp.EndedDown) {
        
        Controlled->dSword = V2(0.0f, 1.0f);
      }
      if(Controller->ActionDown.EndedDown) {
        
        Controlled->dSword = V2(0.0f, -1.0f);
      }
      if(Controller->ActionLeft.EndedDown) {
        
        Controlled->dSword = V2(-1.0f, 0.0f);
      }
      if(Controller->ActionRight.EndedDown) {
        
        Controlled->dSword = V2(1.0f, 0.0f);
      }
#else
      real32 ZoomRate = 0.0f;
      if(Controller->ActionUp.EndedDown) {
        
        ZoomRate = 1.0f;
      }
      if(Controller->ActionDown.EndedDown) {
        
        ZoomRate = -1.0f;
      }
      
      GameState->OffsetZ += ZoomRate*Input->dtForFrame;
      
#endif
    }
  }
  

  loaded_bitmap DrawBuffer_ = {};
  loaded_bitmap *DrawBuffer = &DrawBuffer_;
  DrawBuffer->Width = Buffer->Width;
  DrawBuffer->Height = Buffer->Height;
  DrawBuffer->Memory = Buffer->Memory;
  DrawBuffer->Pitch = Buffer->Pitch;
  
  temporary_memory RenderMem = BeginTemporaryMemory(&TranState->TranArena);
  render_group *RenderGroup = AllocateRenderGroup(&TranState->TranArena, Megabytes(4),
                                                  V2i(DrawBuffer->Width, DrawBuffer->Height));
  
  

  
  v2 ScreenCenter = {0.5f*DrawBuffer->Width, 0.5f*DrawBuffer->Height};
  
  
  
#if 1
  
  Clear(RenderGroup, V4(0.25f, 0.00f, 0.25f, 0.0f));
  
#else
  
  Clear(RenderGroup, V4(0.0f, 0.0f, 0.0f, 0.0f));
  
#endif
  
  uint32 TileSpanX = 17*3;
  uint32 TileSpanY = 9*3;
  uint32 TileSpanZ = 1;
  
  real32 ScreenWidthInMeters = Buffer->Width * PtM;
  real32 ScreenHeightInMeters = Buffer->Height * PtM;
  
  
  rectangle2 ScreenBounds = GetCameraRectangleAtTarget(RenderGroup);
  
  rectangle3 CameraBoundsInMeters = RectMinMax(ToV3(ScreenBounds.Min, 0.0f),
                                               ToV3(ScreenBounds.Max, 0.0f));
  
  CameraBoundsInMeters.Min.z = -3.0f*GameState->FloorHeight;
  CameraBoundsInMeters.Max.z = 1.0f*GameState->FloorHeight;
  

  #if 1
  
  // NOTE(Egor): groundbuffer scrolling
  for(uint32 Index = 0; Index < TranState->GroundBufferCount; ++Index) {
    
    ground_buffer *GroundBuffer = TranState->GroundBuffers + Index;
    
    if(IsValid(GroundBuffer->P) && GroundBuffer->P.ChunkZ == 0) {
      
      // NOTE(Egor): recreate bitmap from template
      loaded_bitmap *Bitmap = &GroundBuffer->Bitmap;
      v3 Delta = Subtract(GameState->World, &GroundBuffer->P, &GameState->CameraP);
      
      render_basis *Basis = PushStruct(&TranState->TranArena, render_basis);
      RenderGroup->DefaultBasis = Basis;
      Basis->P = Delta;

//      PushRectOutline(RenderGroup, V3(0, 0, 0), World->ChunkDimInMeters.xy, V4(1.0f, 1.0f, 0.0f, 1.0f));
      PushBitmap(RenderGroup, Bitmap, V3(0, 0, 0), World->ChunkDimInMeters.x);

    }
  }
  
  #endif
  
  RenderGroup->DefaultBasis = PushStruct(&TranState->TranArena, render_basis);
  RenderGroup->DefaultBasis->P = V3(0,0,0);
  
  {
    world_position MinChunk = MapIntoChunkSpace(World, GameState->CameraP, GetMinCorner(CameraBoundsInMeters));
    world_position MaxChunk = MapIntoChunkSpace(World, GameState->CameraP, GetMaxCorner(CameraBoundsInMeters));
    
    for(int32 ChunkZ = MinChunk.ChunkZ; ChunkZ <= MaxChunk.ChunkZ; ++ChunkZ) {
      for(int32 ChunkY = MinChunk.ChunkY; ChunkY <= MaxChunk.ChunkY; ++ChunkY) {
        for(int32 ChunkX = MinChunk.ChunkX; ChunkX <= MaxChunk.ChunkX; ++ChunkX) {
          
          world_position ChunkCenterP = CenteredChunkPoint(ChunkX, ChunkY, ChunkZ);
          v3 RelP = Subtract(GameState->World, &ChunkCenterP, &GameState->CameraP);
          ground_buffer *FurthestBuffer = 0;
          real32 FurthestBufferLenSq = 0.0f;
          
          for(uint32 GroundBufferIndex = 0;
              GroundBufferIndex < TranState->GroundBufferCount;
              ++GroundBufferIndex) {
            
            ground_buffer *GroundBuffer = TranState->GroundBuffers + GroundBufferIndex;
            
            if(AreInTheSameChunk(World, &GroundBuffer->P ,
                                 &ChunkCenterP)) {
              FurthestBuffer = 0;
              break;
            }
            else if (IsValid(GroundBuffer->P)) {
              
              v3 Rel = Subtract(GameState->World, &GroundBuffer->P, &GameState->CameraP);
              real32 DistanceLenSq = LengthSq(Rel.xy);
              if(DistanceLenSq > FurthestBufferLenSq) {
                
                FurthestBufferLenSq = DistanceLenSq;
                FurthestBuffer = GroundBuffer;
              }
            }
            else {
              
              FurthestBufferLenSq = real32Maximum;
              FurthestBuffer = GroundBuffer;              
            }
          }
          
          if(FurthestBuffer) {
            
            if(ChunkZ == GameState->CameraP.ChunkZ) 
              FillGroundChunk(TranState, GameState, FurthestBuffer, &ChunkCenterP);
          }
          
//          PushRectOutline(RenderGroup, ToV3(RelP.xy, 0), World->ChunkDimInMeters.xy, V4(1.0f, 1.0f, 0.0f, 1.0f));
        }
      }
    }
  }
  
  
  
  v3 SimBoundExpansion = V3(15.0f , 15.0f, 0.0f);
  rectangle3 SimBounds = AddRadiusTo(CameraBoundsInMeters, SimBoundExpansion);
  
  memory_arena SimArena;
  InitializeArena(&SimArena, Memory->TransientStorageSize, Memory->TransientStorage);
  temporary_memory SimMem = BeginTemporaryMemory(&TranState->TranArena);
  
  
  world_position SimCenterP = GameState->CameraP;
  sim_region *SimRegion = BeginSim(&TranState->TranArena, GameState, GameState->World,
                                   SimCenterP, SimBounds, Input->dtForFrame);
  
  PushRectOutline(RenderGroup, V3(0,0,0), GetDim(ScreenBounds), V4(1.0f, 1.0f, 1.0f, 1.0f)); 
//  PushRectOutline(RenderGroup, V3(0,0,0), GetDim(SimBounds).xy, V4(1.0f, 0.0f, 0.0f, 1.0f));
  PushRectOutline(RenderGroup, V3(0,0,0), GetDim(SimRegion->UpdateBounds).xy, V4(1.0f, 0.0f, 1.0f, 1.0f));
  PushRectOutline(RenderGroup, V3(0,0,0), GetDim(SimRegion->Bounds).xy, V4(0.0f, 0.0f, 1.0f, 1.0f));
  
  
  ////////////////////////////////////////////////////////////////////////////////
  // NOTE(Egor): entity processing
  ////////////////////////////////////////////////////////////////////////////////
  
  real32 dt = Input->dtForFrame;
  
  // NOTE(Egor): Relative coordinate of camera to sim_region Origin
  v3 CameraP = Subtract(World, &GameState->CameraP, &SimCenterP);
  
  for(uint32 EntityIndex = 0; EntityIndex < SimRegion->EntityCount; ++EntityIndex) {
    
    sim_entity *Entity = SimRegion->Entities + EntityIndex;
    
    if(Entity->Updatable) {
      
      render_basis *Basis = PushStruct(&TranState->TranArena, render_basis);
      RenderGroup->DefaultBasis = Basis;
      
      move_spec MoveSpec = DefaultMoveSpec();
      v3 ddP = {};
      
      v3 CameraRelativeGroundP = GetEntityGroundPoint(Entity) - CameraP;
      
      real32 FadeTopEnd = 0.75f*GameState->FloorHeight;
      real32 FadeTopStart = 0.5f*GameState->FloorHeight;
      real32 FadeBottomStart = -2.0f*GameState->FloorHeight;
      real32 FadeBottomEnd = -2.25f*GameState->FloorHeight;
      
      RenderGroup->GlobalAlpha = 1.0f;
      
      if(CameraRelativeGroundP.z > FadeTopStart) {
        
        RenderGroup->GlobalAlpha = Clamp01MapToRange(FadeTopEnd, CameraRelativeGroundP.z, FadeTopStart);
      }
      else if(CameraRelativeGroundP.z < FadeBottomStart) {
        
        RenderGroup->GlobalAlpha = Clamp01MapToRange(FadeBottomEnd, CameraRelativeGroundP.z, FadeBottomStart);
      }
      
      // NOTE(Egor): SIMULATING ENTITIES
      hero_bitmaps *Hero = &GameState->Hero[Entity->FacingDirection];    
      switch(Entity->Type) {
        
        case EntityType_Sword: {
          
          MoveSpec.Speed = 0.0f;
          MoveSpec.Drag = 0.0f;
          MoveSpec.UnitMaxAccelVector = true;
          
          if(Entity->DistanceLimit == 0.0f) {
            
            ClearCollisionRulesFor(GameState, Entity->StorageIndex);
            MakeEntityNonSpatial(Entity);
          }
          
          v2 Alignment = ConvertToBottomUpAlign(&GameState->Sword, V2(26, 37));
          
        } break;
        case EntityType_Hero: {
          
          for(uint32 Index = 0; Index < ArrayCount(GameState->ControlledEntities); ++Index) {
            
            controlled_entity *Controlled = GameState->ControlledEntities + Index;
            
            if(Controlled->EntityIndex == Entity->StorageIndex) {
              
              if(Controlled->dZ) {
                
                Entity->dP.z = Controlled->dZ;
              }
              
              MoveSpec.Speed = 50.0f;
              MoveSpec.Drag = 8.0f;
              MoveSpec.UnitMaxAccelVector = true;
              ddP = ToV3(Controlled->ddP, 0);
              
              if((Controlled->dSword.x != 0.0f) || (Controlled->dSword.y != 0.0f)) {
                
                sim_entity *Sword = Entity->Sword.Ptr;
                if(Sword && IsSet(Sword, EntityFlag_NonSpatial)) {
                  
                  Sword->DistanceLimit = 5.0f;
                  MakeEntitySpatial(Sword, Entity->P, 5.0f*ToV3(Controlled->dSword, 0));
                  AddCollisionRule(GameState, Entity->StorageIndex, Sword->StorageIndex, false);
                }
              }
            }
          }
          
        } break;
        case EntityType_Familiar: {
          
          sim_entity *ClosestHero = 0;
          real32 ClosestHeroDSq = Square(10.0f);
          for(uint32 Index = 1; Index < SimRegion->EntityCount; ++Index) {
            
            sim_entity *TestEntity = SimRegion->Entities + Index;
            if(TestEntity->Type == EntityType_Hero) {
              
              real32 TestDSq = LengthSq(TestEntity->P - Entity->P);
              
              if(ClosestHeroDSq > TestDSq && TestDSq > Square(3.0f)) {
                
                ClosestHero = TestEntity;
                ClosestHeroDSq = TestDSq;
              }
            }
          }
          if(ClosestHero) {
            
            real32 Acceleration = 0.5f;
            real32 OneOverLength = Acceleration / SquareRoot(ClosestHeroDSq);
            ddP = (ClosestHero->P - Entity->P) * OneOverLength;
          }
          
          MoveSpec.Speed = 50.0f;
          MoveSpec.Drag = 8.0f;
          MoveSpec.UnitMaxAccelVector = true;
          
        } break;
        default: {
          InvalidCodePath;
        }
      }
      
      if(!IsSet(Entity, EntityFlag_NonSpatial) &&
         IsSet(Entity, EntityFlag_Moveable)) {
        
        MoveEntity(GameState, SimRegion, Entity, Input->dtForFrame, &MoveSpec, ddP);
      }
      
      Basis->P = GetEntityGroundPoint(Entity) + V3(0, 0, (real32)GameState->OffsetZ);
      
      // NOTE(Egor): RENDERING ENTITIES
      
      switch(Entity->Type) {
        
        case EntityType_Sword: {
          
          MoveSpec.Speed = 0.0f;
          MoveSpec.Drag = 0.0f;
          MoveSpec.UnitMaxAccelVector = true;
          
          if(Entity->DistanceLimit == 0.0f) {
            
            ClearCollisionRulesFor(GameState, Entity->StorageIndex);
            MakeEntityNonSpatial(Entity);
          }
          
          v2 Alignment = ConvertToBottomUpAlign(&GameState->Sword, V2(26, 37));
          PushBitmap(RenderGroup, &GameState->Sword, V3(0, 0, 0), 0.3f);
          
        } break;
        case EntityType_Hero: {
          
          for(uint32 Index = 0; Index < ArrayCount(GameState->ControlledEntities); ++Index) {
            
            controlled_entity *Controlled = GameState->ControlledEntities + Index;
            
            if(Controlled->EntityIndex == Entity->StorageIndex) {
              
              if(Controlled->dZ) {
                
                Entity->dP.z = Controlled->dZ;
              }
              
              MoveSpec.Speed = 50.0f;
              MoveSpec.Drag = 8.0f;
              MoveSpec.UnitMaxAccelVector = true;
              ddP = ToV3(Controlled->ddP, 0);
              
              if((Controlled->dSword.x != 0.0f) || (Controlled->dSword.y != 0.0f)) {
                
                sim_entity *Sword = Entity->Sword.Ptr;
                if(Sword && IsSet(Sword, EntityFlag_NonSpatial)) {
                  
                  Sword->DistanceLimit = 5.0f;
                  MakeEntitySpatial(Sword, Entity->P, 5.0f*ToV3(Controlled->dSword, 0));
                  AddCollisionRule(GameState, Entity->StorageIndex, Sword->StorageIndex, false);
                }
              }
            }
          }
          
          real32 HeroSizeC = 2.0f;
          PushBitmap(RenderGroup, &Hero->HeroHead, V3(0, 0, 0), HeroSizeC*1.4f);
          PushBitmap(RenderGroup, &Hero->HeroCape, V3(0, 0, 0), HeroSizeC*1.4f);
          //PushRect(RenderGroup, V3(0, 0, 0), Entity->Collision->TotalVolume.Dim.xy, V4(1.0f, 0.0f, 0.0f, 1.0f)); 
          
          DrawHitpoints(Entity, RenderGroup);
        } break;
        case EntityType_Space: {
          
#if 1
          for(uint32 VolumeIndex = 0;
              VolumeIndex < Entity->Collision->VolumeCount;
              ++VolumeIndex) {
            
            sim_entity_collision_volume *Volume = Entity->Collision->Volumes + VolumeIndex;
            PushRectOutline(RenderGroup, Volume->OffsetP - V3(0,0, 0.5f*Volume->Dim.z), Volume->Dim.xy, V4(0.0f, 0.0f, 1.0f, 1.0f)); 
          }
#endif
          
        } break;
        case EntityType_Wall: {
          
          //PushRect(&RenderGroup, V2(0,0), 0.0f, 0.0f, Entity->Collision->TotalVolume.Dim.XY, V4(1.0f, 0.0f, 0.0f, 1.0f)); 
          PushBitmap(RenderGroup, &GameState->Tree, V3(0, 0, 0), 2.5f);
          
        } break;
        case EntityType_Stairwell: {
          
          PushRect(RenderGroup, V3(0.0f, 0.0f * Entity->WalkableHeight/5, 0), Entity->WalkableDim, V4(1.0f, 0.0f, 0.0f, 1.0f)); 
          PushRect(RenderGroup, V3(0.1f, 1.0f * Entity->WalkableHeight/5, 0), Entity->WalkableDim, V4(1.0f, 0.2f, 0.0f, 1.0f)); 
          PushRect(RenderGroup, V3(0.2f, 2.0f * Entity->WalkableHeight/5, 0), Entity->WalkableDim, V4(1.0f, 0.4f, 0.0f, 1.0f)); 
          PushRect(RenderGroup, V3(0.3f, 3.0f * Entity->WalkableHeight/5, 0), Entity->WalkableDim, V4(1.0f, 0.6f, 0.0f, 1.0f)); 
          PushRect(RenderGroup, V3(0.4f, 4.0f * Entity->WalkableHeight/5, 0), Entity->WalkableDim, V4(1.0f, 0.8f, 0.0f, 1.0f)); 
          PushRect(RenderGroup, V3(0.5f, 5.0f * Entity->WalkableHeight/5, 0), Entity->WalkableDim, V4(1.0f, 1.0f, 0.0f, 1.0f)); 
        } break;
        case EntityType_Monster: {
          
          PushBitmap(RenderGroup, &Hero->HeroHead, V3(0, 0, 0), 1.4f);
          DrawHitpoints(Entity, RenderGroup);
        } break;
        case EntityType_Familiar: {
          
          sim_entity *ClosestHero = 0;
          real32 ClosestHeroDSq = Square(10.0f);
          for(uint32 Index = 1; Index < SimRegion->EntityCount; ++Index) {
            
            sim_entity *TestEntity = SimRegion->Entities + Index;
            if(TestEntity->Type == EntityType_Hero) {
              
              real32 TestDSq = LengthSq(TestEntity->P - Entity->P);
              
              if(ClosestHeroDSq > TestDSq && TestDSq > Square(3.0f)) {
                
                ClosestHero = TestEntity;
                ClosestHeroDSq = TestDSq;
              }
              
            }
            
          }
          if(ClosestHero) {
            
            real32 Acceleration = 0.5f;
            real32 OneOverLength = Acceleration / SquareRoot(ClosestHeroDSq);
            ddP = (ClosestHero->P - Entity->P) * OneOverLength;
          }
          
          MoveSpec.Speed = 50.0f;
          MoveSpec.Drag = 8.0f;
          MoveSpec.UnitMaxAccelVector = true;
          
          PushBitmap(RenderGroup, &Hero->HeroHead, V3(0, 0, 0), 1.4f);
        } break;
        default: {
          InvalidCodePath;
        }
      }
      
      
    }
  }
  
  
#if 0
  GameState->Time += Input->dtForFrame;
  real32 Angle = 0.1f*GameState->Time;
  v2 Disp = 100.0f*V2(0.0f, Sin(3.0f*Angle));
  
  v2 Origin = ScreenCenter + Disp;
  v2 XAxis = 100.0f*V2(Cos(Angle), Sin(Angle));
  //  v2 XAxis = 100.0f*V2(1.0f, 0.0f);
  v2 YAxis = Perp(XAxis);
  
#if 0
  
  real32 CAngle = 5.0f*Angle;
  v4 Color = V4(0.5f+0.5f*Sin(CAngle),
                0.5f+0.5f*Sin(2.9f*CAngle),
                0.5f+0.5f*Cos(9.9f*CAngle),
                0.5f + 0.5f*Sin(40.0f*CAngle));
#else
  
  v4 Color = V4(1.0f, 1.0f, 1.0f, 1.0f);
#endif
  
  
  uint32 CheckerHeight = 16;
  uint32 CheckerWidth = 16;
  uint32 Toggle = 0;
  
  v4 ColorMatrix[] = 
  { {1,0,0,1},
    {0,1,0,1},
    {0,0,1,1}
  };
  
  
  for(uint32 Index = 0;Index < ArrayCount(TranState->EnvMaps); ++Index) {
    
    environment_map *Map = TranState->EnvMaps + Index;
    loaded_bitmap *LOD = &Map->LOD[0];
    
    for(uint32 Y = 0; Y < TranState->EnvMapHeight; Y += CheckerHeight, ++Toggle %= 2) {
      for(uint32 X = 0; X < TranState->EnvMapWidth;
          X += CheckerWidth, ++Toggle %= 2) {
        
        v4 CheckerColor = Toggle ? ColorMatrix[Index] : V4(0, 0, 0, 1);
        
        DrawRectangle(LOD,
                      V2i(X, Y), V2i(X + CheckerWidth, Y + CheckerHeight),
                      CheckerColor);
      }
    }
  }
  TranState->EnvMaps[0].Pz = -8.0f;
  TranState->EnvMaps[1].Pz = 0.0f;
  TranState->EnvMaps[2].Pz = 8.0f;
  
  PushCoordinateSystem(RenderGroup, Origin - 0.5f*XAxis - 0.5f*YAxis,
                       XAxis, YAxis, Color, 
                       &GameState->TestDiffuse, &GameState->TestNormal,
                       TranState->EnvMaps + 2,
                       TranState->EnvMaps + 1,
                       TranState->EnvMaps + 0);
  
  v2 MapP = V2(0,0);
  for(uint32 Index = 0;
      Index < ArrayCount(TranState->EnvMaps);
      ++Index) {
    
    environment_map *Map = TranState->EnvMaps + Index;
    loaded_bitmap *LOD = &Map->LOD[0];
    
    XAxis = 0.5f*V2i(LOD->Width, 0);
    YAxis = 0.5f*V2i(0, LOD->Height);
    
    PushCoordinateSystem(RenderGroup, MapP, XAxis, YAxis, V4(1.0f, 1.0f, 1.0f, 1.0f),
                         &TranState->EnvMaps[Index].LOD[0], 0,0,0,0);
    MapP += YAxis + V2(0.0f, 6.0f);
  }
  
#endif
  TiledRenderPushBuffer(TranState->RenderQueue, RenderGroup, DrawBuffer);
 
  #if 0
  world_position WorldOrigin = {};
  v3 Diff = Subtract(SimRegion->World, &WorldOrigin, &SimRegion->Origin);
  DrawRectangle(DrawBuffer, Diff.XY, V2(10.0f, 10.0f), V4(1.0f, 1.0f, 0.0f, 1.0f));
  #endif
 
  
  // NOTE(Egor): Ending the simulation
  EndSim(SimRegion, GameState);
  EndTemporaryMemory(SimMem);
  EndTemporaryMemory(RenderMem);
  CheckArena(&GameState->WorldArena);
  CheckArena(&TranState->TranArena);
  
  END_TIMED_BLOCK(GameUpdateAndRender);
}
  
extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
  game_state *GameState = (game_state *)Memory->PermanentStorage;
  GameOutputSound(GameState, SoundBuffer, 400);
}



#if 0
internal void
RenderWeirdGradient(game_offscreen_buffer *Buffer, int BlueOffset, int GreenOffset)
{
  // TODO(Egor): Let's see what the optimizer does
  
  uint8 *Row = (uint8 *)Buffer->Memory;    
  for(int Y = 0;
      Y < Buffer->Height;
      ++Y)
  {
    uint32 *Pixel = (uint32 *)Row;
    for(int X = 0;
        X < Buffer->Width;
        ++X)
    {
      uint8 Blue = (uint8)(X + BlueOffset);
      uint8 Green = (uint8)(Y + GreenOffset);
      
      *Pixel++ = ((Green << 16) | Blue);
    }
    
    Row += Buffer->Pitch;
  }
}
#endif