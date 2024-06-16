//  Copyright © 2022 Apple Inc.

#pragma once

#include <Foundation/Foundation.h>
#include <Metal/Metal.h>
#include <initializer_list>
#include <MetalPerformanceShaders/MetalPerformanceShaders.h>
#include <MetalPerformanceShadersGraph/MetalPerformanceShadersGraph.h>
#include <unordered_map>
#include "MPSDevice.h"
typedef id<MTLDevice> MTLDevice_t;

// Fwd declarations
namespace torchao::mps {
enum class SyncType {
  NONE, // no commit to command buffer
  COMMIT, // commit and flush the command buffer
  COMMIT_AND_WAIT, // flush and wait for command buffer execution to finish
  COMMIT_AND_CONTINUE, // commit and continue with a new underlying command
                       // buffer
  COMMIT_ADAPTIVE, // commit adaptively based on available memory
};

// Helper structure to copy data between CPU <-> GPU
struct CPUBufferWrapper {
  void* srcBuffer;
  void* dstBuffer;
  size_t length;
  size_t srcOffset;
  size_t dstOffset;
  union {
    struct {
      unsigned int srcCpu : 1;
      unsigned int dstCpu : 1;
    };
    uint16_t flags;
  };
};

class MPSStream {
 public:
  MPSStream();

  ~MPSStream();
  id<MTLCommandQueue> commandQueue() const {
    return _commandQueue;
  };
  dispatch_queue_t queue() const {
    return _serialQueue;
  }
  MTLDevice_t device() const { return [_commandQueue device];}

  bool hasLiveCommandBuffer();
  MPSCommandBuffer* commandBuffer();
  id<MTLComputeCommandEncoder> commandEncoder();
  void endKernelCoalescing();
  void synchronize(SyncType syncType);
  bool commitAndContinueEnabled();
  void copy(
      id<MTLBuffer> srcBuffer,
      id<MTLBuffer> dstBuffer,
      size_t length,
      size_t srcOffset,
      size_t dstOffset,
      SyncType syncType = SyncType::NONE);
  void copy(
      std::vector<CPUBufferWrapper>& dataBuffers,
      SyncType syncType = SyncType::NONE);
  void copy_and_sync(
      id<MTLBuffer> srcBuffer,
      id<MTLBuffer> dstBuffer,
      size_t length,
      size_t srcOffset,
      size_t dstOffset,
      bool non_blocking);
  void copy_and_sync(
      std::vector<CPUBufferWrapper>& dataBuffers,
      bool non_blocking);

 private:
  id<MTLCommandQueue> _commandQueue = nil;
  MPSCommandBuffer* _commandBuffer = nil;
  MPSCommandBuffer* _prevCommandBuffer = nil;
  id<MTLComputeCommandEncoder> _commandEncoder = nil;
  dispatch_queue_t _serialQueue = nullptr;
  // CommitAndContinue is disabled by default
  bool _enableCommitAndContinue = false;
  // accumulated sizes of resources encoded on command buffer
  size_t _commandBufferResourceSize = 0;
  // unfortunately, there's no way to get the underlying buffer from
  // an MPSGraphTensorData. so we need to keep a mapping of them here
  std::unordered_map<MPSGraphTensorData*, void*> _activeResources{};

  // use synchronize() to access any of these commit functions outside MPSStream
  void commit();
  void commitAndWait();
  void commitAndContinue();
  void flush();
};

/**
 * Get the current MPS stream
 */
MPSStream* getCurrentMPSStream();

/**
 * Get the default MPS stream
 */
MPSStream* getDefaultMPSStream();

//-----------------------------------------------------------------
//  MPSStreamImpl
//-----------------------------------------------------------------

class MPSStreamImpl {
 public:
  /**
   * Gets single instance of the MPSStream.
   */
  static MPSStream* getInstance();

 private:
  static MPSStream* _stream;
  MPSStreamImpl();
};

} // namespace torchao::mps
