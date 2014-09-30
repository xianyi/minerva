#pragma once
#include <vector>
#include <string>
#include <utility>
#include <unordered_map>
#include <mutex>
#include "device/data_store.h"
#include "op/physical.h"
#include "op/physical_fn.h"
#include "procedures/device_listener.h"
#include "common/common.h"
#include "common/concurrent_blocking_queue.h"
#include "common/thread_pool.h"
#ifdef HAS_CUDA
#include <cuda.h>
#include <cublas_v2.h>
#endif

namespace minerva {

class Device {
 public:
  enum class MemType {
    kCpu,
    kGpu
  };
  Device(uint64_t, DeviceListener*);
  virtual ~Device();
  virtual void PushTask(uint64_t) = 0;
  virtual std::pair<MemType, float*> GetPtr(uint64_t) = 0;
  virtual std::string Name() const = 0;
  virtual void FreeDataIfExist(uint64_t);

 protected:
  std::unordered_set<uint64_t> local_data_;
  std::unordered_set<uint64_t> remote_data_;
  uint64_t device_id_;
  DataStore* data_store_;
  DeviceListener* listener_;

 private:
  Device();
  DISALLOW_COPY_AND_ASSIGN(Device);
};

#ifdef HAS_CUDA
class GpuDevice : public Device {
 public:
  GpuDevice(uint64_t, DeviceListener*, int);
  ~GpuDevice();
  void PushTask(uint64_t);
  std::pair<MemType, float*> GetPtr(uint64_t);
  std::string Name() const;

 private:
  static const size_t kParallelism = 16;
  const int device_;
  void Execute(uint64_t, int);
  std::unordered_map<uint64_t, std::mutex> copy_locks_;
  cudaStream_t stream_[kParallelism];
  cublasHandle_t handle_[kParallelism];
  ThreadPool pool_;
  DISALLOW_COPY_AND_ASSIGN(GpuDevice);
};
#endif

class CpuDevice : public Device {
 public:
  CpuDevice(uint64_t, DeviceListener*);
  ~CpuDevice();
  void PushTask(uint64_t);
  std::pair<MemType, float*> GetPtr(uint64_t);
  std::string Name() const;
  void FreeDataIfExist(uint64_t);

 private:
  std::unordered_map<uint64_t, std::mutex> copy_locks_;
  static const size_t kDefaultThreadNum = 8;
  void Execute(uint64_t, int);
  ThreadPool pool_;
  DISALLOW_COPY_AND_ASSIGN(CpuDevice);
};

}  // namespace minerva

