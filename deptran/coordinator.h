#pragma once

#include "__dep__.h"
#include "constants.h"
#include "txn_chopper.h"
//#include "all.h"
#include "msg.h"
#include "commo.h"
#include "client_worker.h"


namespace rococo {
class ClientControlServiceImpl;

enum CoordinatorStage { START, PREPARE, FINISH };

class CoordinatorBase {
public:
  std::mutex mtx_;
  uint32_t n_start_ = 0;
  virtual ~CoordinatorBase() = default;
  // TODO do_one should be replaced with Submit.
  virtual void do_one(TxnRequest &) = 0;
  virtual void Reset() = 0;
  virtual void restart(TxnCommand *ch) = 0;
};

class Coordinator : public CoordinatorBase {
 public:
  uint32_t coo_id_;
  int benchmark_;
  ClientControlServiceImpl *ccsi_;
  uint32_t thread_id_;
  bool batch_optimal_;
  bool retry_wait_;

  uint32_t n_start_ = 0;
  uint32_t n_start_ack_ = 0;
  uint32_t n_prepare_req_ = 0;
  uint32_t n_prepare_ack_ = 0;
  uint32_t n_finish_req_ = 0;
  uint32_t n_finish_ack_ = 0;

  std::atomic<uint64_t> next_pie_id_;
  std::atomic<uint64_t> next_txn_id_;

  std::function<void()> callback_;

  std::mutex mtx_;
  std::mutex start_mtx_;
  Recorder *recorder_;
  Command *cmd_; 
//  cmdid_t cmd_id_;
  CoordinatorStage stage_ = START;
  phase_t phase_ = 0;
  map<innid_t, bool> start_ack_map_;
  Sharding* sharding_ = nullptr;
  TxnRegistry *txn_reg_ = nullptr;
  Communicator* commo_ = nullptr;
  Frame* frame_ = nullptr;

  std::vector<int> site_prepare_;
  std::vector<int> site_commit_;
  std::vector<int> site_abort_;
  std::vector<int> site_piece_;
#ifdef TXN_STAT
  typedef struct txn_stat_t {
    uint64_t                             n_serv_tch;
    uint64_t                             n_txn;
    std::unordered_map<int32_t, uint64_t>piece_cnt;
    txn_stat_t() : n_serv_tch(0), n_txn(0) {}
    void one(uint64_t _n_serv_tch, const std::vector<int32_t>& pie) {
      n_serv_tch += _n_serv_tch;
      n_txn++;

      for (int i = 0; i < pie.size(); i++) {
        if (pie[i] != 0) {
          auto it = piece_cnt.find(pie[i]);

          if (it == piece_cnt.end()) {
            piece_cnt[pie[i]] = 1;
          } else {
            piece_cnt[pie[i]]++;
          }
        }
      }
    }

    void output() {
      Log::info("SERV_TCH: %lu, TXN_CNT: %lu, MEAN_SERV_TCH_PER_TXN: %lf",
                n_serv_tch, n_txn, ((double)n_serv_tch) / n_txn);

      for (auto& it : piece_cnt) {
        Log::info("\tPIECE: %d, PIECE_CNT: %lu, MEAN_PIECE_PER_TXN: %lf",
                  it.first, it.second, ((double)it.second) / n_txn);
      }
    }
  } txn_stat_t;
  std::unordered_map<int32_t, txn_stat_t> txn_stats_;
#endif /* ifdef TXN_STAT */
  Coordinator(uint32_t coo_id,
              int benchmark,
              ClientControlServiceImpl *ccsi = NULL,
              uint32_t thread_id = 0,
              bool batch_optimal = false);

  virtual ~Coordinator();

  /** thread unsafe */
  uint64_t next_pie_id() {
    return this->next_pie_id_++;
  }

  /** thread unsafe */
  uint64_t next_txn_id() {
    return this->next_txn_id_++;
  }

  virtual void do_one(TxnRequest &) = 0;
  virtual void Submit(SimpleCommand &, std::function<void()>) {
    verify(0);
  }
  virtual void Reset() {
    n_start_ = 0;
    n_start_ack_ = 0;
    n_prepare_req_ = 0;
    n_prepare_ack_ = 0;
    n_finish_req_ = 0;
    n_finish_ack_ = 0;
  }
  virtual void restart(TxnCommand *ch) = 0;
};
}
