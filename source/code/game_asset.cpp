internal v2
ConvertToBottomUpAlign(loaded_bitmap *Bitmap, v2 Align) {
  
  Align.y = (Bitmap->Height - 1) - Align.y;
  return Align;
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
DEBUGLoadBMP(char *FileName, v2 AlignPercentage = V2(0.5f , 0.5f))
{
  
  loaded_bitmap Result = {};
  debug_read_file_result ReadResult = DEBUGReadEntireFile(FileName);
  
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
//    Result.NativeHeight = Result.Height * PtM;
    
    
    // NOTE(Egor): alignment
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

internal bitmap_id
DEBUGAddBitmapInfo(game_assets *Assets, char *FileName, v2 AlignPercentage) {
 
  Assert(Assets->DEBUGUsedBitmapCount < Assets->BitmapCount);
  bitmap_id ID = {Assets->DEBUGUsedBitmapCount++};
  
  asset_bitmap_info *Info = Assets->BitmapInfos + ID.Value;
  Info->AlignPercentage = AlignPercentage;
  Info->FileName = FileName;
  
  return ID;
}

internal void
BeginAssetType(game_assets *Assets, asset_type_id ID) {
  
  Assert(Assets->DEBUGAssetType == 0);
  
  Assets->DEBUGAssetType = Assets->AssetTypes + ID;
  Assets->DEBUGAssetType->FirstAssetIndex = Assets->DEBUGUsedAssetCount;
  Assets->DEBUGAssetType->OnePastLastAssetIndex = Assets->DEBUGAssetType->FirstAssetIndex;
}

internal void
AddBitmapAsset(game_assets *Assets, char *FileName, v2 AlignPercentage = V2(0.5f, 0.5f)) {
  
  Assert(Assets->DEBUGAssetType);
  
  asset *Asset = Assets->Assets + Assets->DEBUGAssetType->OnePastLastAssetIndex++;
  Asset->FirstTagIndex = Assets->DEBUGUsedTagCount;
  Asset->OnePastLastTagIndex = Asset->FirstTagIndex;
  Asset->SlotID = DEBUGAddBitmapInfo(Assets, FileName, AlignPercentage).Value;
  
  Assets->DEBUGAsset = Asset;
}

internal void
AddTag(game_assets *Assets, asset_tag_id TagID, real32 Value) {
  
  Assert(Assets->DEBUGAssetType);
  
  asset *Asset = Assets->DEBUGAsset;
  Asset->OnePastLastTagIndex++;
  asset_tag *Tag = Assets->Tags + Assets->DEBUGUsedTagCount++;
  Tag->TagID = TagID;
  Tag->Value = Value;
};

internal void
EndAssetType(game_assets *Assets) {
  
  Assert(Assets->DEBUGAssetType);
  
  Assets->DEBUGUsedAssetCount = Assets->DEBUGAssetType->OnePastLastAssetIndex;
  Assets->DEBUGAssetType = 0;
}


internal game_assets *
AllocateGameAssets(memory_arena *Arena, uint32 Size,
                   transient_state *TranState) {
  
  game_assets *Assets = PushStruct(Arena, game_assets);
  SubArena(&Assets->Arena, Arena, Size); 
  Assets->TranState = TranState;
  
  for(uint32 TagType = 0;
      TagType < ArrayCount(Assets->TagRange);
      ++TagType) {
    
    Assets->TagRange[TagType] = 1000000.0f;
  }
  
  Assets->TagRange[Tag_FacingDirection] = 2.0f*Pi32;
  
  Assets->BitmapCount = 256*AID_Count;
  Assets->AssetCount = Assets->BitmapCount;
  Assets->TagCount = Tag_Count*Assets->AssetCount;
  
  Assets->Assets = PushArray(Arena, Assets->AssetCount, asset);
  Assets->Tags = PushArray(Arena, Assets->TagCount, asset_tag);
  Assets->Bitmaps = PushArray(Arena, Assets->BitmapCount, asset_slot);
  Assets->BitmapInfos = PushArray(Arena, Assets->BitmapCount, asset_bitmap_info);
  
  Assets->DEBUGUsedBitmapCount = 1;
  Assets->DEBUGUsedAssetCount = 1;
  
  BeginAssetType(Assets, AID_Tree);
  AddBitmapAsset(Assets,"..//source//assets//tree.bmp", V2(0.493872f, 0.29565f));
  EndAssetType(Assets);
  
  BeginAssetType(Assets, AID_Sword);
  AddBitmapAsset(Assets, "..//source//assets//dagger.bmp", V2(0.5f, 0.5f));      
  EndAssetType(Assets);
  
  BeginAssetType(Assets, AID_Grass);
  AddBitmapAsset(Assets, "..//..//test//grass00.bmp", V2(0.5f, 0.5f));
  AddBitmapAsset(Assets, "..//..//test//grass01.bmp", V2(0.5f, 0.5f));
  EndAssetType(Assets);
  
  BeginAssetType(Assets, AID_Stone);
  AddBitmapAsset(Assets, "..//..//test//ground00.bmp", V2(0.5f, 0.5f));
  AddBitmapAsset(Assets, "..//..//test//ground01.bmp", V2(0.5f, 0.5f));
  AddBitmapAsset(Assets, "..//..//test//ground02.bmp", V2(0.5f, 0.5f));
  AddBitmapAsset(Assets, "..//..//test//ground03.bmp", V2(0.5f, 0.5f));
  EndAssetType(Assets);
  
  BeginAssetType(Assets, AID_Rock);
  AddBitmapAsset(Assets, "..//..//test//rock00.bmp", V2(0.5f, 0.5f));
  EndAssetType(Assets);
  
  BeginAssetType(Assets, AID_Tuft);
  AddBitmapAsset(Assets, "..//..//test//tuft00.bmp", V2(0.5f, 0.5f));
  AddBitmapAsset(Assets, "..//..//test//tuft01.bmp", V2(0.5f, 0.5f));
  AddBitmapAsset(Assets, "..//..//test//tuft02.bmp", V2(0.5f, 0.5f));
  EndAssetType(Assets);
  
  BeginAssetType(Assets, AID_Figurine);
  AddBitmapAsset(Assets, "..//source//assets//figurine.bmp");
  EndAssetType(Assets);

  #if 1  
  BeginAssetType(Assets, AID_FigurineArrow);
  AddBitmapAsset(Assets, "..//source//assets//arrow_right.bmp");
  AddTag(Assets, Tag_FacingDirection, 0.0f);
  AddBitmapAsset(Assets, "..//source//assets//arrow_up.bmp");
  AddTag(Assets, Tag_FacingDirection, Pi32/2.0f);
  AddBitmapAsset(Assets, "..//source//assets//arrow_left.bmp");
  AddTag(Assets, Tag_FacingDirection, Pi32);
  AddBitmapAsset(Assets, "..//source//assets//arrow_down.bmp");
  AddTag(Assets, Tag_FacingDirection, -Pi32/2.0f);
  EndAssetType(Assets);
  #endif

  #if 0
  hero_bitmaps *Bitmap = Assets->Hero;
  
  Bitmap->HeroHead = DEBUGLoadBMP("..//source//assets//figurine.bmp");
  Bitmap->HeroCape = DEBUGLoadBMP("..//source//assets//arrow_right.bmp");
  Bitmap->Align = ConvertToBottomUpAlign(&Bitmap->HeroHead,V2(51, 112));
  
  Bitmap[1] = Bitmap[0];
  Bitmap[1].HeroCape = DEBUGLoadBMP("..//source//assets//arrow_up.bmp");
  
  Bitmap[2] = Bitmap[0];
  Bitmap[2].HeroCape = DEBUGLoadBMP("..//source//assets//arrow_left.bmp");
  
  Bitmap[3] = Bitmap[0];
  Bitmap[3].HeroCape = DEBUGLoadBMP("..//source//assets//arrow_down.bmp");
  #endif
  
  return Assets;
};






struct load_bitmap_work {
  
  game_assets *Assets;
  loaded_bitmap *Bitmap;
  bitmap_id ID;
  asset_state FinalState;
  char *FileName;
  v2 AlignPercentage;
  
  task_with_memory *Task;
};

internal PLATFORM_WORK_QUEUE_CALLBACK(LoadBitmapWork) {
  
  
  load_bitmap_work *Work = (load_bitmap_work *)Data;  
  Assert(Work);
  
  asset_bitmap_info *Info = Work->Assets->BitmapInfos + Work->ID.Value;
  
  *Work->Bitmap = DEBUGLoadBMP(Info->FileName, Info->AlignPercentage);
  
  WRITE_BARRIER;
  
  Work->Assets->Bitmaps[Work->ID.Value].Bitmap = Work->Bitmap;
  Work->Assets->Bitmaps[Work->ID.Value].State = Work->FinalState;
  
  EndTask(Work->Task);
}


internal void
LoadBitmap(game_assets *Assets, bitmap_id ID) {
  
  if(ID.Value && (CompareExchangeUInt32((uint32 *)&Assets->Bitmaps[ID.Value].State, GAS_Unloaded, GAS_Queued) ==
                  GAS_Unloaded)) {
    
    task_with_memory *Task = BeginTask(Assets->TranState);
    
    if(Task) {
      
      load_bitmap_work *Work = PushStruct(&Task->Arena, load_bitmap_work);
      
      Work->Assets = Assets;
      Work->ID = ID;
      Work->Task = Task;
      Work->Bitmap = PushStruct(&Assets->Arena, loaded_bitmap);
      Work->FinalState = GAS_Loaded;
      
      PlatformAddEntry(Assets->TranState->LowPriorityQueue, LoadBitmapWork, Work);
    }
  }
}

internal loaded_bitmap *
GetBitmap(game_assets *Assets, bitmap_id ID) {
  
  loaded_bitmap *Result = Assets->Bitmaps[ID.Value].Bitmap;
  return Result;
};

internal bitmap_id
GetFirstBitmapID(game_assets *Assets, asset_type_id ID) {
  
  bitmap_id Result = {};
  asset_type *Type = Assets->AssetTypes + ID;
  asset *Asset = Assets->Assets + Type->FirstAssetIndex;
  
  // NOTE(Egor): range is not empty set
  if(Type->FirstAssetIndex != Type->OnePastLastAssetIndex) {
    
    Result.Value = Asset->SlotID; 
  }
  
  return Result;
};

internal bitmap_id
RandomAssetFrom(game_assets *Assets, random_series *Series, asset_type_id TypeID) {
  
  bitmap_id Result = {};
  asset_type *Type = Assets->AssetTypes + TypeID;
  
  if(Type->FirstAssetIndex != Type->OnePastLastAssetIndex) {
    
    uint32 ChoiceCount = Type->OnePastLastAssetIndex - Type->FirstAssetIndex;
    uint32 Choice = RandomChoice(Series, ChoiceCount);
    uint32 AssetIndex = (Type->FirstAssetIndex + Choice);
    asset *Asset = Assets->Assets + AssetIndex;
    Result.Value = Asset->SlotID;
  }
  
  return Result;
};

internal bitmap_id
BestMatchAsset(game_assets *Assets, asset_type_id TypeID,
               asset_vector *MatchVector, asset_vector *WeightVector) {
  
  bitmap_id Result = {};
  
  real32 BestDiff = real32Maximum;
  
  asset_type *Type = Assets->AssetTypes + TypeID;
  
  for(uint32 AssetIndex = Type->FirstAssetIndex;
      AssetIndex < Type->OnePastLastAssetIndex;
      ++AssetIndex) {
    
    asset *Asset = Assets->Assets + AssetIndex;
    real32 TotalWeightedDiff = 0.0f;
    
    for(uint32 TagIndex = Asset->FirstTagIndex;
        TagIndex < Asset->OnePastLastTagIndex;
        ++TagIndex) {
      
      asset_tag *Tag = Assets->Tags + TagIndex;
      
      real32 A = MatchVector->E[Tag->TagID];
      real32 B =  Tag->Value;
      real32 TagRange = Assets->TagRange[Tag->TagID];
      
      real32 D0 = A - B;
      real32 D1 = (A - TagRange*SignOf(A)) - B;
      real32 Difference = Min(AbsoluteValue(D0), AbsoluteValue(D1));
      
      real32 Weighted = WeightVector->E[Tag->TagID]*AbsoluteValue(Difference);
      TotalWeightedDiff += Weighted;
    }
    
    if(BestDiff > TotalWeightedDiff) {
      
      BestDiff = TotalWeightedDiff;
      Result.Value = Asset->SlotID;
    }
  }
  
  return Result;
}



  
  
  
  