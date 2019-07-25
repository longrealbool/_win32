#include "game.h"
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
             thread_context *Thread, char *FileName)
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
        
        real32 R = (real32)((C & RedMask) >> RedShiftDown);
        real32 G = (real32)((C & GreenMask) >> GreenShiftDown);
        real32 B = (real32)((C & BlueMask) >> BlueShiftDown);
        real32 A = (real32)((C & AlphaMask) >> AlphaShiftDown);
        real32 An = A / 255.0f;
        
#if 1
        R = R*An;
        G = G*An;
        B = B*An;
#endif
        
        *SourceDest = (((uint32)(A + 0.5f) << 24) |
                       ((uint32)(R + 0.5f) << 16) |
                       ((uint32)(G + 0.5f) << 8)  |
                       ((uint32)(B + 0.5f) << 0));
        
        SourceDest++;
      }
    }
    
    Result.Pitch = -Result.Width*LOADED_BITMAP_BYTES_PER_PIXEL;
    // NOTE(Egor): bitmap was loaded bottom to top, so we reverse pitch (make him negative),
    // and shift Memory ptr to top of the image (bottom of the array)
    Result.Memory = (uint8 *)Result.Memory - (Result.Height-1)*(Result.Pitch);
  }
  
  
  
  return Result;
}


internal void
DrawBitmap(loaded_bitmap *Buffer,
           loaded_bitmap *Bitmap,
           real32 RealX, real32 RealY, real32 CAlpha = 1.0f) {
  
  int32 MinX = RoundReal32ToInt32(RealX);
  int32 MinY = RoundReal32ToInt32(RealY);
  int32 MaxX = MinX + Bitmap->Width;
  int32 MaxY = MinY + Bitmap->Height;
  
  
  
  int32 SourceOffsetX = 0;
  // clipping value to actual size of the backbuffer
  if(MinX < 0) {
    SourceOffsetX = -MinX;
    MinX = 0;
  }
  int32 SourceOffsetY = 0;
  if(MinY < 0) { 
    SourceOffsetY = -MinY;
    MinY = 0;
  }
  // we will write up to, but not including to final row and column
  if(MaxX > Buffer->Width) MaxX = Buffer->Width;
  if(MaxY > Buffer->Height) MaxY = Buffer->Height;
  
  // NOTE(Egor): just example
  // uint8 *EndOfBuffer = (uint8 *)Buffer->Memory + Buffer->Pitch*Buffer->Height;
  

  
  uint8 *Test = (uint8 *)Bitmap->Memory;
  uint8 *SourceRow = (uint8 *)Bitmap->Memory + SourceOffsetY*Bitmap->Pitch
                      + LOADED_BITMAP_BYTES_PER_PIXEL*SourceOffsetX;
  
  // go to line to draw
  uint8 *DestRow = ((uint8 *)Buffer->Memory + MinY*Buffer->Pitch + MinX*LOADED_BITMAP_BYTES_PER_PIXEL);
  
  for(int Y = MinY; Y < MaxY; ++Y)
  {
    uint32 *Dest = (uint32 *)DestRow;
    uint32 *Source = (uint32 *)SourceRow;
    for(int X = MinX; X < MaxX; ++X)
    {

      real32 As = (real32)((*Source >> 24) & 0xFF);
      real32 RAs = (As/255.0f) * CAlpha;
      real32 Rs = CAlpha*(real32)((*Source >> 16) & 0xFF);
      real32 Gs = CAlpha*(real32)((*Source >> 8) & 0xFF);
      real32 Bs = CAlpha*(real32)((*Source >> 0) & 0xFF);
      
      real32 Ad = (real32)((*Dest >> 24) & 0xFF);
      real32 Rd = (real32)((*Dest >> 16) & 0xFF);
      real32 Gd = (real32)((*Dest >> 8) & 0xFF);
      real32 Bd = (real32)((*Dest >> 0) & 0xFF);
      
      real32 RAd = (Ad / 255.0f);
      
      real32 RAsComplement = (1 - RAs );
      
      // NOTE(Egor): alpha channel for compisited bitmaps in premultiplied alpha mode
      // for case when we create intermediate buffer with two or more bitmaps blend with 
      // each other
      real32 A = (RAs + RAd - RAs*RAd)*255.0f; 
      real32 R = RAsComplement*Rd + Rs;
      real32 G = RAsComplement*Gd + Gs;
      real32 B = RAsComplement*Bd + Bs;
      
      
      *Dest = (((uint32)(A + 0.5f) << 24) |
               ((uint32)(R + 0.5f) << 16) |
               ((uint32)(G + 0.5f) << 8)  |
               ((uint32)(B + 0.5f) << 0));
      
      Dest++;
      Source++;
    }
    DestRow += Buffer->Pitch;
    SourceRow += Bitmap->Pitch;
    
  }
  
}

internal void
DrawRectangle(loaded_bitmap *Buffer, v2 vMin, v2 vMax,
              real32 R, real32 G, real32 B)
{
  
  int32 MinX = RoundReal32ToInt32(vMin.X);
  int32 MinY = RoundReal32ToInt32(vMin.Y);
  int32 MaxX = RoundReal32ToInt32(vMax.X);
  int32 MaxY = RoundReal32ToInt32(vMax.Y);
  
  // clipping value to actual size of the backbuffer
  if(MinX < 0) MinX = 0;
  if(MinY < 0) MinY = 0;
  // we will write up to, but not including to final row and column
  if(MaxX > Buffer->Width) MaxX = Buffer->Width;
  if(MaxY > Buffer->Height) MaxY = Buffer->Height;
  
  uint32 Color = ((RoundReal32ToUInt32(R * 255.0f) << 16) |
                  (RoundReal32ToUInt32(G * 255.0f) << 8) |
                  (RoundReal32ToUInt32(B * 255.0f) << 0));
  
  
  uint8 *EndOfBuffer = (uint8 *)Buffer->Memory + Buffer->Pitch*Buffer->Height;
  
  // go to line to draw
  uint8 *Row = ((uint8 *)Buffer->Memory + MinY*Buffer->Pitch + MinX*LOADED_BITMAP_BYTES_PER_PIXEL);
  // TODO: color
  for(int Y = MinY; Y < MaxY; ++Y)
  {
    uint32 *Pixel = (uint32 *)Row;
    for(int X = MinX; X < MaxX; ++X)
    {
      *Pixel++ = Color;
    }
    Row += Buffer->Pitch;
  }
}


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
  Entity.Low->Sim.WalkableDim = Entity.Low->Sim.Collision->TotalVolume.Dim.XY;
  Entity.Low->Sim.WalkableHeight = GameState->World->TileDepthInMeters;
  
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


inline void
PushPiece(entity_visible_piece_group *Group, loaded_bitmap *Bitmap,
          v2 Offset, real32 OffsetZ, real32 OffsetZC, v2 Align, v2 Dim, v4 Color) {
  
  Assert(Group->Count < ArrayCount(Group->Pieces));
  entity_visible_piece *Piece = Group->Pieces + Group->Count++;
  Piece->Bitmap = Bitmap;
  Piece->Offset = Group->GameState->MetersToPixels*V2(Offset.X, -Offset.Y) - Align;
  Piece->OffsetZ = OffsetZ;
  Piece->OffsetZC = OffsetZC;
  
  Piece->Dim = Dim;
  
  Piece->R = Color.R;
  Piece->G = Color.G;
  Piece->B = Color.B;
  Piece->A = Color.A;
}

internal void 
PushBitmap(entity_visible_piece_group *Group, loaded_bitmap *Bitmap,
           v2 Offset, real32 OffsetZ, real32 OffsetZC, v2 Align, real32 A = 1.0f)
{
  
  PushPiece(Group, Bitmap, Offset, OffsetZ, OffsetZC,  Align, V2(0,0), V4(1.0f, 1.0f, 1.0f, A));
}

internal void 
PushRect(entity_visible_piece_group *Group,
         v2 Offset, real32 OffsetZ, real32 OffsetZC,
         v2 Dim, v4 Color)

{
  
  PushPiece(Group, 0, Offset, OffsetZ, OffsetZC, V2(0,0), Dim,
            Color);
}

internal void 
PushRectOutline(entity_visible_piece_group *Group,
                v2 Offset, real32 OffsetZ, real32 OffsetZC,
                v2 Dim, v4 Color){
  
  real32 Thickness = 0.1f;
  
  PushPiece(Group, 0, Offset - V2(0, Dim.Y)*0.5f, OffsetZ, OffsetZC, V2(0,0), V2(Dim.X, Thickness), Color);
  PushPiece(Group, 0, Offset + V2(0, Dim.Y)*0.5f, OffsetZ, OffsetZC, V2(0,0), V2(Dim.X, Thickness), Color);
  
  PushPiece(Group, 0, Offset - V2(Dim.X, 0)*0.5f, OffsetZ, OffsetZC, V2(0,0), V2(Thickness, Dim.Y), Color);
  PushPiece(Group, 0, Offset + V2(Dim.X, 0)*0.5f, OffsetZ, OffsetZC, V2(0,0), V2(Thickness, Dim.Y), Color);
}


internal void
DrawHitpoints(sim_entity *Entity, entity_visible_piece_group *PieceGroup) {
  
  if(Entity->HitPointMax) {
    
    v2 HitPointDim = {0.2f , 0.2f};
    real32 SpacingX = 2.0f * HitPointDim.X;
    real32 FirstX = (Entity->HitPointMax - 1)*0.5f*SpacingX;
    v2 HitP = {(Entity->HitPointMax - 1)*-0.5f*SpacingX, -0.4f};
    v2 dHitP = {SpacingX, 0};
    
    for(uint32 HPIndex = 0; HPIndex < Entity->HitPointMax; ++HPIndex) {
      
      hit_point *HitPoint = Entity->HitPoint + HPIndex;
      
      v4 Color = {1.0f , 0.0f, 0.0f, 1.0f};
      if(HitPoint->FilledAmount == 0) {
        
        Color = V4(0.2f, 0.2f, 0.2f, 1.0f);
      }
      
      PushRect(PieceGroup, HitP, 0.0f, 0.0f, HitPointDim, Color); 
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
DrawGroundChunk(game_state *GameState, loaded_bitmap *Buffer, world_position *Pos) {
  
  // TODO(Egor): this is nuts, make sane spatial hashing
  random_series Series = Seed(12*Pos->ChunkX + 34*Pos->ChunkY + 57*Pos->ChunkZ);
  
  real32 Width = (real32)Buffer->Width;
  real32 Height = (real32)Buffer->Height;
  
  loaded_bitmap *Stamp = 0;
  for(uint32 Index = 0; Index < 450; ++Index) {
    
    if(RandomChoice(&Series, 2)) {
      
      Stamp = GameState->Grass + RandomChoice(&Series,  ArrayCount(GameState->Grass));
    }
    else {
      
      Stamp = GameState->Stones + RandomChoice(&Series, ArrayCount(GameState->Stones));
    }
    
    v2 BitmapCenter = 0.5f*V2i(Stamp->Width, Stamp->Height);
    v2 Offset = V2(Width*RollTheDiceUnilateral(&Series),
                   Height*RollTheDiceUnilateral(&Series));
    v2 P = Offset - BitmapCenter;
    DrawBitmap(Buffer, Stamp, P.X, P.Y, 1.0f);
  }
  
  for(uint32 Index = 0; Index < 100; ++Index) {
    
    Stamp = GameState->Tuft + RandomChoice(&Series, ArrayCount(GameState->Tuft));
    
    v2 BitmapCenter = 0.5f*V2i(Stamp->Width, Stamp->Height);
    v2 Offset = V2(Width*RollTheDiceUnilateral(&Series),
                   Height*RollTheDiceUnilateral(&Series));
    v2 P = Offset - BitmapCenter;
    DrawBitmap(Buffer, Stamp, P.X, P.Y, 1.0f);
  }
}


internal loaded_bitmap
MakeEmptyBitmap(memory_arena *Arena, uint32 Width, uint32 Height) {
  
  loaded_bitmap Result = {};
  
  Result.Width = Width;
  Result.Height = Height;
  Result.Pitch = Result.Width * LOADED_BITMAP_BYTES_PER_PIXEL;
  uint32 TotalBitmapSize = Result.Width*Result.Height * sizeof(uint32);
  Result.Memory = PushSize_(Arena, TotalBitmapSize);
  ZeroSize(TotalBitmapSize, Result.Memory);
  
  return Result;
}


extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
  Assert((&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0]) ==
         (ArrayCount(Input->Controllers[0].Buttons)));
  Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
  
  game_state *GameState = (game_state *)Memory->PermanentStorage;
  if(!Memory->IsInitialized)
  {
    
    uint32 TilesPerHeight = 9;
    uint32 TilesPerWidth = 17;
    
    InitializeArena(&GameState->WorldArena, Memory->PermanentStorageSize - sizeof(game_state),
                    (uint8 *)Memory->PermanentStorage + sizeof(game_state));
    
    GameState->World = PushStruct(&GameState->WorldArena, world);
    world *World = GameState->World;
    InitializeWorld(World, 1.4f, 3.0f);
    
    AddLowEntity(GameState, EntityType_Null, 0);
    // TODO(Egor): This may be more appropriate to do in the platform layer
    
    
    GameState->NullCollision = MakeNullCollision(GameState);
    GameState->SwordCollision = MakeSimpleGroundedCollision(GameState, 1.0f, 0.5f, 0.1f);
    GameState->PlayerCollision = MakeSimpleGroundedCollision(GameState, 1.0f, 0.5f, 1.2f);
    GameState->MonsterCollision = MakeSimpleGroundedCollision(GameState,1.0f, 0.5f, 0.5f);
    GameState->FamiliarCollision = MakeSimpleGroundedCollision(GameState,1.0f, 0.5f, 0.5f);
    
    GameState->WallCollision = MakeSimpleGroundedCollision(GameState, 
                                                           GameState->World->TileSideInMeters,
                                                           GameState->World->TileSideInMeters,
                                                           GameState->World->TileDepthInMeters);
    
    GameState->StairCollision = MakeSimpleGroundedCollision(GameState,
                                                            GameState->World->TileSideInMeters,
                                                            2.0f * GameState->World->TileSideInMeters,
                                                            1.1f * GameState->World->TileDepthInMeters);
    
    GameState->StandardRoomCollision = MakeSimpleGroundedCollision(GameState, 
                                                                   TilesPerWidth * GameState->World->TileSideInMeters,
                                                                   TilesPerHeight * GameState->World->TileSideInMeters,
                                                                   0.9f * GameState->World->TileDepthInMeters);
    
    
    GameState->Backdrop = DEBUGLoadBMP(Memory->DEBUGPlatformReadEntireFile, Thread, "..//source//assets//background.bmp");
    GameState->Sword = DEBUGLoadBMP(Memory->DEBUGPlatformReadEntireFile, Thread, "..//source//assets//dagger.bmp");
    GameState->Tree = DEBUGLoadBMP(Memory->DEBUGPlatformReadEntireFile, Thread, "..//source//assets//tree.bmp");
    
    GameState->Stairwell = DEBUGLoadBMP(Memory->DEBUGPlatformReadEntireFile, Thread, "..//source//assets//arrow_up.bmp");
    
    hero_bitmaps *Bitmap = GameState->Hero;
    
    Bitmap->HeroHead = 
      DEBUGLoadBMP(Memory->DEBUGPlatformReadEntireFile, Thread, "..//source//assets//figurine.bmp");
    Bitmap->HeroCape = 
      DEBUGLoadBMP(Memory->DEBUGPlatformReadEntireFile, Thread, "..//source//assets//arrow_right.bmp");
    Bitmap->Align = V2(51, 112);
    
    
    Bitmap[1] = Bitmap[0];
    Bitmap[1].HeroCape = 
      DEBUGLoadBMP(Memory->DEBUGPlatformReadEntireFile, Thread, "..//source//assets//arrow_up.bmp");
    
    Bitmap[2] = Bitmap[0];
    Bitmap[2].HeroCape = 
      DEBUGLoadBMP(Memory->DEBUGPlatformReadEntireFile, Thread, "..//source//assets//arrow_left.bmp");
    
    Bitmap[3] = Bitmap[0];
    Bitmap[3].HeroCape = 
      DEBUGLoadBMP(Memory->DEBUGPlatformReadEntireFile, Thread, "..//source//assets//arrow_down.bmp");
    
    
    //    GameState->Tree = DEBUGLoadBMP(Memory->DEBUGPlatformReadEntireFile, Thread, "..//..//test//tree00.bmp");
    
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
    
#endif 
    
    
    
    int32 TileSideInPixels = 60;
    GameState->MetersToPixels = TileSideInPixels/World->TileSideInMeters;
    
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
      
      // TODO(Egor, TileMap): get again through logic of creating tilemap
      uint32 RandomChoice;
      
      bool32 NonValid = 1; // WARNING(Egor): TEST (!!!)
      //if(DoorUp || DoorDown) {
      if(1) {
        RandomChoice = RollTheDice() % 2;
      }
      else {
        RandomChoice = RollTheDice() % 3;
      }
      
      // NOTE(Egor): keep track of is there a need to create another stairwell
      bool32 CreatedZDoor = false;
      if (RandomChoice == 2) {
        
        CreatedZDoor = true;
        if(AbsTileZ == ScreenBaseZ) 
          DoorUp = true;
        else
          DoorDown = true;
        
      }
      else if(RandomChoice == 0) {
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
            if(TileY == 4 && TileX == 13) {
              
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
      
      if(RandomChoice == 2) {
        
        if(AbsTileZ == ScreenBaseZ) 
          AbsTileZ = ScreenBaseZ + 1;
        else
          AbsTileZ = ScreenBaseZ;
      }
      else if(RandomChoice == 0) {
        ScreenX++;
      }
      else if(RandomChoice == 1) {
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
    
    AddMonster(GameState, CameraTileX + 2, CameraTileY + 2, CameraTileZ);
    //AddFamiliar(GameState, CameraTileX, CameraTileY - 2, CameraTileZ);
    
    real32 ScreenWidth = (real32)Buffer->Width;
    real32 ScreenHeight = (real32)Buffer->Height;
    real32 MaximumZScale = 0.5f;
    real32 GroundOverscan = 1.5f;
    uint32 GroundBufferWidth = RoundReal32ToUInt32(GroundOverscan * ScreenWidth);
    uint32 GroundBufferHeight = RoundReal32ToUInt32(GroundOverscan * ScreenHeight);
    GameState->GroundBufferP = GameState->CameraP;
    
    
    GameState->GroundBuffer = MakeEmptyBitmap(&GameState->WorldArena,
                                              GroundBufferWidth, GroundBufferHeight);
    DrawGroundChunk(GameState, &GameState->GroundBuffer, &GameState->GroundBufferP);
    
    Memory->IsInitialized = true;
  }
  
  world *World = GameState->World;
  
  // TODO(Egor): put this into renderer in the future
  real32 MetersToPixels = GameState->MetersToPixels;
  
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
          Controlled->ddP.Y = 1.0f;
        }
        if(Controller->MoveRight.EndedDown) {
          Controlled->ddP.X = 1.0f;
        }
        if(Controller->MoveDown.EndedDown) {
          Controlled->ddP.Y = -1.0f;
        }
        if(Controller->MoveLeft.EndedDown) {
          Controlled->ddP.X = -1.0f;
        }
      }
      
      if(Controller->Start.EndedDown) {
        Controlled->dZ = 2.0f;
      }
      
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
    }
  }
  
  
  uint32 TileSpanX = 17*3;
  uint32 TileSpanY = 9*3;
  uint32 TileSpanZ = 1;
  
  rectangle3 CameraBounds = RectCenterDim(V3(0,0,0),
                                          World->TileSideInMeters*V3((real32)TileSpanX,
                                                                     (real32)TileSpanY,
                                                                     (real32)TileSpanZ));
  
  memory_arena SimArena;
  InitializeArena(&SimArena, Memory->TransientStorageSize, Memory->TransientStorage);
  
  sim_region *SimRegion = BeginSim(&SimArena, GameState, GameState->World, GameState->CameraP, CameraBounds, Input->dtForFrame);
  
  //
  // NOTE(Egor): rudimentary render starts below
  //
  
  loaded_bitmap DrawBuffer_ = {};
  loaded_bitmap *DrawBuffer = &DrawBuffer_;
  DrawBuffer->Width = Buffer->Width;
  DrawBuffer->Height = Buffer->Height;
  DrawBuffer->Memory = Buffer->Memory;
  DrawBuffer->Pitch = Buffer->Pitch;

  real32 CenterX = 0.5f*DrawBuffer->Width;
  real32 CenterY = 0.5f*DrawBuffer->Height;
  
  DrawRectangle(DrawBuffer, V2(0.0f, 0.0f),
                V2((real32)DrawBuffer->Width, (real32)DrawBuffer->Height),
                0.5f, 0.5f, 0.5f);
  
  v2 Ground = V2(CenterX - 0.5f * (real32)GameState->GroundBuffer.Width, 
                  CenterY - 0.5f * (real32)GameState->GroundBuffer.Height);
  
  // NOTE(Egor): groundbuffer scrolling
  v3 DeltaGround2Camera = Subtract(GameState->World, &GameState->CameraP , &GameState->GroundBufferP);
  DeltaGround2Camera.Y = -DeltaGround2Camera.Y;
  Ground -= (MetersToPixels*DeltaGround2Camera.XY);
  
  DrawBitmap(DrawBuffer, &GameState->GroundBuffer, Ground.X, Ground.Y);
  
  entity_visible_piece_group PieceGroup;
  PieceGroup.GameState = GameState;
  
  sim_entity *Entity = SimRegion->Entities;
  for(uint32 EntityIndex = 0; EntityIndex < SimRegion->EntityCount; ++EntityIndex, ++Entity) {
    
    if(Entity->Updatable) {
      
      move_spec MoveSpec = DefaultMoveSpec();
      v3 ddP = {};
      
      PieceGroup.Count = 0;
      real32 dt = Input->dtForFrame;
      
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
          
          PushBitmap(&PieceGroup, &GameState->Sword, V2(0, 0), 0, 1.0f, V2(26, 37));
          
        } break;
        case EntityType_Hero: {
          
          for(uint32 Index = 0; Index < ArrayCount(GameState->ControlledEntities); ++Index) {
            
            controlled_entity *Controlled = GameState->ControlledEntities + Index;
            
            if(Controlled->EntityIndex == Entity->StorageIndex) {
              
              if(Controlled->dZ) {
                
                Entity->dP.Z = Controlled->dZ;
              }
              
              MoveSpec.Speed = 50.0f;
              MoveSpec.Drag = 8.0f;
              MoveSpec.UnitMaxAccelVector = true;
              ddP = V3(Controlled->ddP, 0);
              
              if((Controlled->dSword.X != 0.0f) || (Controlled->dSword.Y != 0.0f)) {
                
                sim_entity *Sword = Entity->Sword.Ptr;
                if(Sword && IsSet(Sword, EntityFlag_NonSpatial)) {
                  
                  Sword->DistanceLimit = 5.0f;
                  MakeEntitySpatial(Sword, Entity->P, 5.0f*V3(Controlled->dSword, 0));
                  AddCollisionRule(GameState, Entity->StorageIndex, Sword->StorageIndex, false);
                }
              }
            }
          }
          
          PushBitmap(&PieceGroup, &Hero->HeroTorso, V2(0, 0), 0, 1.0f, Hero->Align);
          PushBitmap(&PieceGroup, &Hero->HeroHead, V2(0, 0), 0, 1.0f, Hero->Align);
          PushBitmap(&PieceGroup, &Hero->HeroCape, V2(0, 0), 0, 1.0f, Hero->Align);
          PushRect(&PieceGroup, V2(0,0), 0.0f, 0.0f, Entity->Collision->TotalVolume.Dim.XY, V4(1.0f, 0.0f, 0.0f, 1.0f)); 
          
          DrawHitpoints(Entity, &PieceGroup);
        } break;
        case EntityType_Space: {
          
#if 0
          for(uint32 VolumeIndex = 0;
              VolumeIndex < Entity->Collision->VolumeCount;
              ++VolumeIndex) {
            
            sim_entity_collision_volume *Volume = Entity->Collision->Volumes + VolumeIndex;
            PushRectOutline(&PieceGroup, Volume->OffsetP.XY, 0, 0, Volume->Dim.XY, V4(0.0f, 0.0f, 1.0f, 1.0f)); 
          }
#endif
          
        } break;
        case EntityType_Wall: {
          
          PushBitmap(&PieceGroup, &GameState->Tree, V2(0, 0), 0, 1.0f, V2(30, 30));
          //PushRect(&PieceGroup, V2(0,0), 0.0f, 0.0f, Entity->Collision->TotalVolume.Dim.XY, V4(1.0f, 0.0f, 0.0f, 1.0f)); 
        } break;
        case EntityType_Stairwell: {
          
          PushRect(&PieceGroup, V2(0,0), 0.0f * Entity->WalkableHeight/5, 0.0f, Entity->WalkableDim, V4(1.0f, 0.0f, 0.0f, 1.0f)); 
          PushRect(&PieceGroup, V2(0,0), 1.0f * Entity->WalkableHeight/5, 0.0f, Entity->WalkableDim, V4(1.0f, 0.2f, 0.0f, 1.0f)); 
          PushRect(&PieceGroup, V2(0,0), 2.0f * Entity->WalkableHeight/5, 0.0f, Entity->WalkableDim, V4(1.0f, 0.4f, 0.0f, 1.0f)); 
          PushRect(&PieceGroup, V2(0,0), 3.0f * Entity->WalkableHeight/5, 0.0f, Entity->WalkableDim, V4(1.0f, 0.6f, 0.0f, 1.0f)); 
          PushRect(&PieceGroup, V2(0,0), 4.0f * Entity->WalkableHeight/5, 0.0f, Entity->WalkableDim, V4(1.0f, 0.8f, 0.0f, 1.0f)); 
          PushRect(&PieceGroup, V2(0,0), 5.0f * Entity->WalkableHeight/5, 0.0f, Entity->WalkableDim, V4(1.0f, 1.0f, 0.0f, 1.0f)); 
        } break;
        case EntityType_Monster: {
          
          PushBitmap(&PieceGroup, &Hero->HeroHead, V2(0, 0), 0, 1.0f, Hero->Align);
          DrawHitpoints(Entity, &PieceGroup);
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
          
          PushBitmap(&PieceGroup, &Hero->HeroHead, V2(0, 0), 1.0f, 0, Hero->Align);
        } break;
        default: {
          InvalidCodePath;
        }
      }
      
      if(!IsSet(Entity, EntityFlag_NonSpatial) &&
         IsSet(Entity, EntityFlag_Moveable)) {
        
        MoveEntity(GameState, SimRegion, Entity, Input->dtForFrame, &MoveSpec, ddP);
      }
      
      
      for(uint32 PieceIndex = 0; PieceIndex < PieceGroup.Count; ++PieceIndex) {
        


        entity_visible_piece *Piece = PieceGroup.Pieces + PieceIndex;
        
        v3 EntityBaseP = GetEntityGroundPoint(Entity);
        real32 ZFudge = (1.0f + 0.05f*(EntityBaseP.Z + Piece->OffsetZ));
        
        if(Entity->Type == EntityType_Stairwell) {
          // DEBUG(Egor)
          int a = 3;
        }
        
        real32 EntityGroundPointX = CenterX + EntityBaseP.X*MetersToPixels*ZFudge;
        real32 EntityGroundPointY = CenterY - EntityBaseP.Y*MetersToPixels*ZFudge;
        real32 EntityZ = -EntityBaseP.Z*MetersToPixels;
        
        v2 Cen = {
          EntityGroundPointX + Piece->Offset.X,
          EntityGroundPointY + Piece->Offset.Y  + EntityZ*Piece->OffsetZC
        };
        
        if(Piece->Bitmap) {
          
          DrawBitmap(DrawBuffer, Piece->Bitmap, Cen.X, Cen.Y, 1.0f);
        }
        else {
          
          v2 HalfDim = Piece->Dim*MetersToPixels*0.5f;
          DrawRectangle(DrawBuffer, Cen - HalfDim, Cen + HalfDim,
                        Piece->R, Piece->G, Piece->B); 
        }
        
      }
    }
  }
  
  world_position WorldOrigin = {};
  v3 Diff = Subtract(SimRegion->World, &WorldOrigin, &SimRegion->Origin);
  DrawRectangle(DrawBuffer, Diff.XY, V2(10.0f, 10.0f), 1.0f, 1.0f, 0.0f);
  
  
  // NOTE(Egor): Ending the simulation
  EndSim(SimRegion, GameState);
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