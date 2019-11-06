


struct platform_work_queue_entry {
  
  platform_work_queue_callback *Callback;
  void *Data;
};


struct platform_work_queue {
  
  uint32 volatile EntryCompleted;
  uint32 volatile CompletionTarget;
  
  uint32 volatile NextEntryToRead;
  uint32 volatile NextEntryToWrite;
  
  
  uint32 MaxEntryCount;
  HANDLE Semaphore;
  
  // NOTE(Egor): has to be the power of 2
  platform_work_queue_entry Entries[256];
};


// TODO(Egor): move code below to .cpp file


// NOTE(Egor): this is how we add entry to the list
void Win32AddEntry(platform_work_queue *Queue, platform_work_queue_callback *Callback, void *Data) {
  
  // NOTE(Egor): length of Entries should be the power of 2
  uint32 NewNextEntryToWrite = (Queue->NextEntryToWrite + 1) & (ArrayCount(Queue->Entries) - 1);
  Assert(NewNextEntryToWrite != Queue->NextEntryToRead);
  platform_work_queue_entry *Entry = Queue->Entries + Queue->NextEntryToWrite;
  Entry->Data = Data;
  Entry->Callback = Callback;
  // NOTE(Egor): is has to be written before entry creation signalled
  ++Queue->CompletionTarget;
  
  WRITE_BARRIER;
  
  Queue->NextEntryToWrite = NewNextEntryToWrite;
  ReleaseSemaphore(Queue->Semaphore, 1, 0);
}


internal bool32
Win32DoNextQueueEntry(platform_work_queue *Queue) {
  
  bool32 GoToSleep = false;
  
  // NOTE(Egor): length of Entries should be the power of 2
  uint32 OriginalNextEntryToRead = Queue->NextEntryToRead;
  uint32 NewNextEntryToRead = (OriginalNextEntryToRead + 1) & (ArrayCount(Queue->Entries) - 1);
  
  if(OriginalNextEntryToRead != Queue->NextEntryToWrite) {
    
    // NOTE(Egor): if another thread beat this thread for increment, just do nothing
    uint32 Index = InterlockedCompareExchange((LONG volatile *)&Queue->NextEntryToRead,
                                              NewNextEntryToRead,
                                              OriginalNextEntryToRead);
    
    if(Index == OriginalNextEntryToRead) {

      platform_work_queue_entry *Entry = Queue->Entries + Index;
      Entry->Callback(Queue, Entry->Data);
      // NOTE(Egor): this is should make the fence needed
      InterlockedIncrement((LONG volatile *)&Queue->EntryCompleted);
    }
  }
  else {
    
    GoToSleep = true;
  }
  
  return GoToSleep;
}

internal void Win32CompleteAllWork(platform_work_queue *Queue) {
  
  while(Queue->EntryCompleted != Queue->CompletionTarget) {
    
    Win32DoNextQueueEntry(Queue);
  }
  
  // NOTE(Egor): resets the queue, hack
  Queue->CompletionTarget = 0;
  Queue->EntryCompleted = 0;
}


struct win32_thread_info {
  
  platform_work_queue *Queue;
  uint32 LogicalThreadIndex;
};


DWORD WINAPI 
ThreadProc(LPVOID lpParamer) {
  
  platform_work_queue *Queue = (platform_work_queue *)lpParamer;
  
  for(;;) {
    
    if(Win32DoNextQueueEntry(Queue))
      WaitForSingleObjectEx(Queue->Semaphore, INFINITE, false);
    
  }
}


// NOTE(Egor): test code
PLATFORM_WORK_QUEUE_CALLBACK(DoPrintingWork) {
 
  char Buffer[256];
  wsprintf(Buffer, "Thread %u %s\n", GetCurrentThreadId(), (char *)Data);
  OutputDebugStringA(Buffer);
}


