#include "game.h"



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
    // TODO(): Draw this out for people
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


internal void
DrawRectangle(game_offscreen_buffer *Buffer,
              real32 RealMinX, real32 RealMinY,
              real32 RealMaxX, real32 RealMaxY,
              real32 R, real32 G, real32 B)
{
  
  int32 MinX = RoundReal32ToInt32(RealMinX);
  int32 MinY = RoundReal32ToInt32(RealMinY);
  int32 MaxX = RoundReal32ToInt32(RealMaxX);
  int32 MaxY = RoundReal32ToInt32(RealMaxY);
  
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
  uint8 *Row = ((uint8 *)Buffer->Memory + MinY*Buffer->Pitch + MinX*Buffer->BytesPerPixel);
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

internal void 
InitializeArena(memory_arena *Arena, size_t Size, uint8 *Base) {
  
  Arena->Size = Size;
  Arena->Base = Base;
  Arena->Used = 0;
}


extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
  Assert((&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0]) ==
         (ArrayCount(Input->Controllers[0].Buttons)));
  Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
  
  
  
  game_state *GameState = (game_state *)Memory->PermanentStorage;
  if(!Memory->IsInitialized)
  {
    // TODO(): This may be more appropriate to do in the platform layer
    Memory->IsInitialized = true;
    
    GameState->PlayerP.AbsTileX = 2;
    GameState->PlayerP.AbsTileY = 4;
    GameState->PlayerP.X = 0.0f;
    GameState->PlayerP.Y = 0.0f;
    InitializeArena(&GameState->WorldArena, Memory->PermanentStorageSize - sizeof(game_state),
                    (uint8 *)Memory->PermanentStorage + sizeof(game_state));
    
    uint32 TilesPerHeight = 9;
    uint32 TilesPerWidth = 17;
    
    GameState->World = PushStruct(&GameState->WorldArena, world);
    world *World = GameState->World;
    World->TileMap = PushStruct(&GameState->WorldArena, tile_map);
    
    
    tile_map *TileMap = World->TileMap;
    
    TileMap->ChunkShift = 4;
    TileMap->ChunkDim = (1 << TileMap->ChunkShift);
    TileMap->ChunkMask = TileMap->ChunkDim - 1;
    
    TileMap->TileChunkCountX = 128;
    TileMap->TileChunkCountY = 128;
    TileMap->TileChunkCountZ = 2;
    
    TileMap->TileSideInMeters = 1.4f;

    
    TileMap->TileChunks = PushArray(&GameState->WorldArena,
                                    TileMap->TileChunkCountZ*
                                    TileMap->TileChunkCountX*
                                    TileMap->TileChunkCountY,
                                    tile_chunk);
    
    uint32 ScreenY = 0;
    uint32 ScreenX = 0;
    uint32 AbsTileZ = 0;
    
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
      
      if(DoorUp || DoorDown) {
        RandomChoice = RollTheDice() % 2;
      }
      else {
        RandomChoice = RollTheDice() % 3;
      }
      
      // NOTE(Egor): keep track of is there a need to create another stairwell
      bool32 CreatedZDoor = false;
      if (RandomChoice == 2) {
        
        CreatedZDoor = true;
        if(AbsTileZ == 0) 
          DoorUp = true;
        else if(AbsTileZ == 1) 
          DoorDown = true;
        
      }
      else if(RandomChoice == 0) {
        DoorToRight = true;
      }
      else {
        DoorToTop = true;
      }
      
      
      for(uint32 TileY = 0; TileY < TilesPerHeight; ++TileY) {
        for(uint32 TileX = 0; TileX < TilesPerWidth; ++TileX) {
          
          int32 AbsTileY = ScreenY*TilesPerHeight + TileY;
          int32 AbsTileX = ScreenX*TilesPerWidth + TileX;
          
          uint32 TileValue = 1;
          
          if(TileX == 0 || TileX == (TilesPerWidth - 1)) {
            TileValue = 2;
            
            if(DoorToRight && TileX == TilesPerWidth - 1) {
              if(TileY == TilesPerHeight/2)
                TileValue = 1;
            }
            
            if(DoorToLeft && TileX == 0 ) {
              if(TileY == TilesPerHeight/2)
                TileValue = 1;
            }
            
          }
          if(TileY == 0 || TileY == (TilesPerHeight - 1)) {
            TileValue = 2;
            
            if(DoorToTop && TileY == TilesPerHeight - 1) {
              if(TileX == TilesPerWidth/2)
                TileValue = 1;
            }
            
            if(DoorToBottom && TileY == 0) {
              if(TileX == TilesPerWidth/2)
                TileValue = 1;
            }
            
          }
          
          if(TileY == 4 && TileX == 8) {
            if(DoorUp) 
              TileValue = 4;
            if(DoorDown)
              TileValue = 5;
          }
          
          SetTileValue(&GameState->WorldArena, TileMap, AbsTileX, AbsTileY, AbsTileZ, TileValue);
          int a = 0;
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
        
        if(AbsTileZ == 1) 
          AbsTileZ = 0;
        else
          AbsTileZ = 1;
      }
      else if(RandomChoice == 0) {
        ScreenX++;
      }
      else if(RandomChoice == 1) {
        ScreenY++;
      }
      
    }
  }

  world *World = GameState->World;
  tile_map *TileMap = World->TileMap;
  
  real32 PlayerHeight = (real32)TileMap->TileSideInMeters;
  real32 PlayerWidth = PlayerHeight*0.75f;
  tile_chunk_position FakePos;
  FakePos.TileChunkX = 0;
  FakePos.TileChunkY = 0;
  
  tile_chunk *TileChunk = GetTileChunk(TileMap, &FakePos);
  Assert(TileChunk);
  
  // TODO(Egor): put this into renderer in the future
  int32 TileSideInPixels = 60;
  real32 MetersToPixels = TileSideInPixels/TileMap->TileSideInMeters;
  
  for(int ControllerIndex = 0;
      ControllerIndex < ArrayCount(Input->Controllers);
      ++ControllerIndex)
  {
    game_controller_input *Controller = GetController(Input, ControllerIndex);
    if(Controller->IsAnalog)
    {
      // analog movement
    }
    else
    {
      real32 dPlayerX = 0.0f;
      real32 dPlayerY = 0.0f;
      
      if(Controller->MoveUp.EndedDown) {
        dPlayerY = 1.0f;
      }
      if(Controller->MoveRight.EndedDown) {
        dPlayerX = +1.0f;
      }
      if(Controller->MoveDown.EndedDown) {
        dPlayerY = -1.0f;
      }
      if(Controller->MoveLeft.EndedDown) {
        dPlayerX = -1.0f;
      }
      
      real32 PlayerSpeed = 5.0f;
      
      if(Controller->Start.EndedDown) {
        if(GameState->PlayerP.AbsTileZ == 1)
          GameState->PlayerP.AbsTileZ = 0;
        else if(GameState->PlayerP.AbsTileZ == 0)
          GameState->PlayerP.AbsTileZ = 1;
      }
      
      if(Controller->Back.EndedDown) {
        PlayerSpeed = 100.0f;
      }
      
      dPlayerX *= PlayerSpeed;
      dPlayerY *= PlayerSpeed;
      
      real32 NewPlayerX = GameState->PlayerP.X + Input->dtForFrame*dPlayerX;
      real32 NewPlayerY = GameState->PlayerP.Y + Input->dtForFrame*dPlayerY;
      
      tile_map_position NewPlayerPos = GameState->PlayerP;
      NewPlayerPos.X = NewPlayerX;
      NewPlayerPos.Y = NewPlayerY;
      
      tile_map_position NewPlayerPosLeft = NewPlayerPos;
      NewPlayerPosLeft.X -= 0.5f*PlayerWidth;
      tile_map_position NewPlayerPosRight = NewPlayerPos;
      NewPlayerPosRight.X += 0.5f*PlayerWidth;
      
      CanonicalizePosition(TileMap, &NewPlayerPos);
      CanonicalizePosition(TileMap, &NewPlayerPosLeft);
      CanonicalizePosition(TileMap, &NewPlayerPosRight);
      
      
      
      bool32 IsValid = (IsTileMapPointEmpty(TileMap, &NewPlayerPos) &&
                        IsTileMapPointEmpty(TileMap, &NewPlayerPosLeft) &&
                        IsTileMapPointEmpty(TileMap, &NewPlayerPosRight));
      
      if(!AreOnTheSameTile(&GameState->PlayerP, &NewPlayerPos)) {
        
        uint32 TileValue = GetTileValue(TileMap, &NewPlayerPos);
        
        if(TileValue == 4) {
          NewPlayerPos.AbsTileZ++;
        }
        if(TileValue == 5) {
          NewPlayerPos.AbsTileZ--;
        }
        
        
      }
      
      if(IsValid) {
        
        GameState->PlayerP = NewPlayerPos;
      }
      
      // digital movement
    }
  }
  
  
  
  
  
  DrawRectangle(Buffer, 0.0f, 0.0f, (real32)Buffer->Width, (real32)Buffer->Height,
                1.0f, 0.0f, 1.0f);
  
  real32 CenterX = 0.5f*Buffer->Width;
  real32 CenterY = 0.5f*Buffer->Height;
  
  for(int32 ViewRow = -6; ViewRow < 6; ++ViewRow) {
    for(int32 ViewColumn = -10; ViewColumn < 10; ++ViewColumn) {
      
      uint32 Column = GameState->PlayerP.AbsTileX + ViewColumn;
      uint32 Row = GameState->PlayerP.AbsTileY + ViewRow;
      
      //    uint32 TestColumn = Column & 255;
      //    uint32 TestRow = Row & 255;
      
      uint32 TileID = GetTileValue(TileMap, Column, Row, GameState->PlayerP.AbsTileZ);
      real32 Gray = 0.5f;
      
      if(TileID > 0) {
        
        if(TileID == 2 || TileID == 3) {
          
          Gray = 1.0f;
        }
        
        if(TileID == 4 || TileID == 5) {
          Gray = 0.25f;
        }
        
        if(Row == GameState->PlayerP.AbsTileY && Column == GameState->PlayerP.AbsTileX) {
          Gray = 0.0f;
        }
        
        real32 CenX = CenterX  + ((real32)ViewColumn * TileSideInPixels) - MetersToPixels*GameState->PlayerP.X;
        real32 CenY = CenterY  - ((real32)ViewRow * TileSideInPixels) + MetersToPixels*GameState->PlayerP.Y;  
        
        real32 MinX = CenX - 0.5f*TileSideInPixels;
        real32 MinY = CenY - 0.5f*TileSideInPixels;
        real32 MaxX = CenX + 0.5f*TileSideInPixels;
        real32 MaxY = CenY + 0.5f*TileSideInPixels;
        
        // NOTE: (Egor) MinY and MaxY is inverted because in windows
        // bitmap Y coord is growing downwards
        DrawRectangle(Buffer, MinX, MinY, MaxX, MaxY, Gray, Gray, Gray);
        if(TileID == 3) {
          SetTileValue(&GameState->WorldArena, TileMap, Column, Row, GameState->PlayerP.AbsTileZ, 2);
          DrawRectangle(Buffer, CenX - 6, CenY - 6, CenX + 6, CenY + 6, 1.0f, 0.0f, 0.0f);
          Gray = 1.0f;
        }
      }
    }
  }
  real32 PlayerR = 1.0f;
  real32 PlayerG = 0.0f;
  real32 PlayerB = 0.0f;
  
  real32 PlayerMinX =  CenterX - MetersToPixels*PlayerWidth/2;
   
  real32 PlayerMinY = CenterY; 
  
  real32 PlayerMaxX = PlayerMinX + PlayerWidth*MetersToPixels;
  real32 PlayerMaxY = PlayerMinY - PlayerHeight*MetersToPixels;
  
  DrawRectangle(Buffer, PlayerMinX, PlayerMaxY,
                PlayerMaxX, PlayerMinY,
                PlayerR, PlayerG, PlayerB);
  
/*  DrawRectangle(Buffer, GameState->PlayerX - 5, GameState->PlayerY, 
                GameState->PlayerX + 5, GameState->PlayerY + 1,
                0.0f, 0.0f, 0.0f);
  */
    
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