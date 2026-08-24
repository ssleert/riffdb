#include "ThreadPool.h"
#include <stdlib.h>

int8_t
ThreadPoolStart(ThreadPool* Self, uint8_t WorkersAmount, int (*Worker)(void*))
{
  *Self = (ThreadPool){
    .MailBoxesSize = WorkersAmount,
    .WorkersSize = WorkersAmount,
  };

  Self->MailBoxes = calloc(Self->MailBoxesSize, sizeof(Channel));
  if (Self->MailBoxes == NULL) {
    return -1;
  }

  for (uint8_t i = 0; i < Self->MailBoxesSize; ++i) {
    ChannelInit(&Self->MailBoxes[i]);
  }

  Self->Workers = calloc(Self->WorkersSize, sizeof(thrd_t));
  if (Self->Workers == NULL) {
    return -1;
  }

  Self->Working = true;
  for (uint8_t i = 0; i < Self->WorkersSize; ++i) {
    ThreadPoolWorker* Arg = malloc(sizeof(ThreadPoolWorker));
    if (Arg == NULL) {
      return -1;
    }

    *Arg = (ThreadPoolWorker){
      .MailBox = &Self->MailBoxes[i],
      .Pool = Self,
    };

    thrd_create(&Self->Workers[i], Worker, Arg);
  }

  return 0;
}

void
ThreadPoolProcess(ThreadPool* Self, void* Message)
{
  uint8_t Index = Self->CurrentWorker;
  Self->CurrentWorker = (Index + 1) % Self->MailBoxesSize;
  ChannelSend(&Self->MailBoxes[Index], Message);
}

void
ThreadPoolStop(ThreadPool* Self)
{
  if (!Self->Working) {
    return;
  }

  Self->Working = false;
  for (uint8_t i = 0; i < Self->WorkersSize; ++i) {
    thrd_join(Self->Workers[i], NULL);
  }
  free(Self->Workers);

  for (uint8_t i = 0; i < Self->MailBoxesSize; ++i) {
    ChannelDestroy(&Self->MailBoxes[i]);
  }
  free(Self->MailBoxes);

  *Self = (ThreadPool){ 0 };
}
