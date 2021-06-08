// #ifndef FACE_MEMPOOL_HPP_
// #define FACE_MEMPOLL_HPP_

// #include "face_queue.hpp"
// #include "cnrt.h"

// namespace facealign {
//   class CpuBuffer {
//     public:
//       explicit CpuBuffer(size_t size) {
//         ptr = malloc(size);
//       }
//       ~CpuBuffer() { if(ptr) free(ptr); }
//     public:
//       void* ptr = nullptr;
//   };

//   class CpuQueueb {
//     public:
//       explicit CpuQueueb(size_t capicity);
//       bool Push(CpuBuffer* buffer);
//       CpuBuffer* Pop();
//     public:
//       ThreadSafeQueue<CpuBuffer*> q_; 
//   }; 

//   class MluBuffer {
//    public:
//     explicit MluBuffer(size_t size, size_t batch_size) {
//       ptr = (void**)malloc(batch_size * sizeof(void*));
//       for (int i = 0; i < batch_size; ++i) {
//         cnrtMalloc(ptr[i], size);
//       }
//     }
    
//     bool CopyFromCpu(void* cpu_ptr) {

//     }

//     bool CopyToCpu(void* cpu_ptr) {

//     }

//     ~CpuBuffer() {
//       if (ptr) {
//         for (int i = 0; i < batch_size; ++i) {
//           cnrtFree(ptr[i]);
//         }
//         free(ptr);
//       }
//     }
//     public:
//       void** ptr = nullptr;
//   }

//   class MluQueueb {
//     public:
//       explicit MluQueueb(size_t capicity);
//       bool Push(MluBuffer* buffer);
//       MluBuffer* Pop();
//     public:
//       ThreadSafeQueue<MluBuffer*> q_; 
//   }; 

// }

// #endif // !FACE_MEMPOOL_HPP_
