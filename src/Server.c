#include "Server.h"
#include "Log.h"
#include <stdlib.h>

typedef struct
{
  Server* Self;
  Channel* MailBox;
} WorkerArgument;

static int
Worker(WorkerArgument* Arg)
{
  while (Arg->Self->Working) {
    WorkerMessage* Msg = ChannelRecv(Arg->MailBox);

    LogInfo("received SocketFD = %d", Msg->SocketFD);

    free(Msg);
  }

  free(Arg);
  return 0;
}

int8_t
ServerStart(Server* Self, uint8_t WorkersAmount)
{
  *Self = (Server){
    .MailBoxesSize = WorkersAmount,
    .WorkersSize = WorkersAmount,
  };

  Self->MailBoxes = calloc(Self->MailBoxesSize, sizeof(Channel));
  if (Self->MailBoxes) {
    return -1;
  }

  for (uint8_t i = 0; i < Self->MailBoxesSize; ++i) {
    ChannelInit(&Self->MailBoxes[i]);
  }

  Self->Workers = calloc(Self->WorkersSize, sizeof(thrd_t));
  if (Self->Workers) {
    return -1;
  }

  for (uint8_t i = 0; i < Self->WorkersSize; ++i) {
    WorkerArgument* Arg = malloc(sizeof(WorkerArgument));
    if (Arg == NULL) {
      return -1;
    }

    *Arg = (WorkerArgument){
      .MailBox = &Self->MailBoxes[i],
      .Self = Self,
    };

    thrd_create(&Self->Workers[i], (int (*)(void*))Worker, Arg);
  }

  Self->Working = true;
  return 0;
}

void
ServerProcess(Server* Self, const WorkerMessage* Message)
{
  return;
}

void
ServerStop(Server* Self)
{
  Self->Working = false;
  for (uint8_t i = 0; i < Self->WorkersSize; ++i) {
    thrd_join(Self->Workers[i], NULL);
  }
}
