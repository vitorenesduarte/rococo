#pragma once

#include "all.h"
#include "rcc_coord.h"
#include "ro6_coord.h"
#include "none_coord.h"

namespace rococo {




struct ClientWorker {
  uint32_t coo_id;
  std::vector<std::string> *servers;
  int32_t benchmark;
  int32_t mode;
  bool batch_start;
  uint32_t id;
  uint32_t duration;
  ClientControlServiceImpl *ccsi;
  uint32_t concurrent_txn;
  rrr::Mutex finish_mutex;
  rrr::CondVar finish_cond;
  Coordinator *coo;
  std::atomic<uint32_t> num_txn, success, num_try;

  Coordinator* GetCoord() {
    if (coo) return coo;
    auto attr = this;
    switch (mode) {
      case MODE_2PL:
      case MODE_OCC:
      case MODE_RPC_NULL:
        coo = new Coordinator(coo_id, *(attr->servers),
                              attr->benchmark, attr->mode,
                              ccsi, id, attr->batch_start);
        break;
      case MODE_RCC:
        coo = new RCCCoord(coo_id, *(attr->servers),
                           attr->benchmark, attr->mode,
                           ccsi, id, attr->batch_start);
        break;
      case MODE_RO6:
        coo = new RO6Coord(coo_id, *(attr->servers),
                           attr->benchmark, attr->mode,
                           ccsi, id, attr->batch_start);
        break;
      case MODE_NONE:
        coo = new NoneCoord(coo_id, *(attr->servers),
                            attr->benchmark, attr->mode,
                            ccsi, id, attr->batch_start);
        break;
      default:
        verify(0);
    }
    return coo;
  }

  void callback2(TxnReply &txn_reply);

  void work();
};
} // namespace rococo

