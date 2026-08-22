#include "Worker.h"
#include "Channel.h"

int32_t WorkerHandler(ThreadPoolWorker* Self) {
  while (Self->Pool->Working) {
    Request* Req = ChannelRecv(Self->MailBox);

    (void)Req;
    // TODO: parse buffer with HttpParser and log info
  }

  return 0;
}
