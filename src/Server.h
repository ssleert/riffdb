#ifndef SERVER_H
#define SERVER_H

#include "Channel.h"

#include <threads.h>
#include <stdbool.h>

typedef struct {
  int32_t SocketFD;
} WorkerMessage; 

typedef struct {
  Channel* MailBoxes; 
  thrd_t*  Workers;

  uint8_t  MailBoxesSize;
  uint8_t  WorkersSize;

  uint8_t  CurrentWorker;

  _Atomic(bool) Working;
} Server;

int8_t ServerStart(Server* Self, uint8_t WorkersAmount);

void ServerProcess(Server* Self, const WorkerMessage* Message);

void ServerStop(Server* Self);

#endif
