#if !defined(GAME_RANDOM_H)

// NOTE(Egor): sort of global "more-honest" random generator

#define MINE_RAND_MAX 32767

internal uint32
RollTheDice(void) {
  
  static uint32 next = 17;
  next = next * 1103515245 + 12345;
  return (unsigned int)(next/65536) % (MINE_RAND_MAX + 1);
}

internal void
PopulateRandomNumbersTable(uint32 *Ptr, uint32 Count) {
  
  for(uint32 Index = 0; Index < Count; ++Index, ++Ptr) {
    
    *Ptr = RollTheDice();
  }
}

internal real32
RollTheDiceUnilateral(void) {
  
  return (real32)RollTheDice()/(real32)MINE_RAND_MAX;
}

internal real32
RollTheDiceBilateral(void) {
  
  real32 Result = 2.0f*RollTheDiceUnilateral() - 1;
  return Result;
}

// NOTE(Egor): predictable and replicable series generator

struct random_series {
  
  uint32 Next;
  uint32 Index;
};

internal random_series
Seed(uint32 Seed) {
  
  
  random_series Result;
  Result.Next = Seed;
  Result.Index = 0;
  return Result;
}

internal uint32
RollTheDice(random_series *Series) {
  
  Series->Next = Series->Next * 1103515245 + 12345;
  return (unsigned int)(Series->Next/65536) % (MINE_RAND_MAX + 1);
}

internal uint32
RandomChoice(random_series *Series, uint32 ChoiceCount) {
  
  uint32 Result = RollTheDice(Series) % ChoiceCount;
  return Result;
}

internal real32
RollTheDiceUnilateral(random_series *Series) {
  
  // NOTE(Egor): cpu usually good at multiplying and fucking horrible at dividing
  real32 Divisor = 1.0f / (real32)MINE_RAND_MAX;
  return Divisor*(real32)RollTheDice(Series);
}

internal real32
RollTheDiceBilateral(random_series *Series) {
  
  real32 Result = 2.0f*RollTheDiceUnilateral(Series) - 1.0f;
  return Result;
}

inline real32
RandomBetween(random_series *Series, real32 Min, real32 Max) {
 
  real32 Result = (Max - Min) * RollTheDiceUnilateral(Series) + Min;
  return Result;
}

#define GAME_RANDOM_H
#endif