/* Copyright 2020 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#include "tensorflow/core/profiler/convert/xplane_to_memory_profile.h"

#include "absl/strings/string_view.h"
#include "tensorflow/core/platform/test.h"
#include "tensorflow/core/platform/types.h"
#include "tensorflow/core/profiler/protobuf/memory_profile.pb.h"
#include "tensorflow/core/profiler/protobuf/xplane.pb.h"
#include "tensorflow/core/profiler/utils/xplane_builder.h"
#include "tensorflow/core/profiler/utils/xplane_schema.h"
#include "tensorflow/core/profiler/utils/xplane_test_utils.h"

namespace tensorflow {
namespace profiler {
namespace {

// Tests with a sample profile with multiple memory allocation and deallocation
// activities within one memory allocator captured in host trace.
TEST(ConvertXPlaneToMemoryProfile, OneAllocatorMultiActivitiesTest) {
  XSpace space;
  XPlane* host_plane = GetOrCreateHostXPlane(&space);
  XPlaneBuilder host_plane_builder(host_plane);
  host_plane_builder.ReserveLines(1);

  auto tf_executor_thread = host_plane_builder.GetOrCreateLine(0);
  CreateXEvent(&host_plane_builder, &tf_executor_thread, "MemoryAllocation",
               40000, 1000,
               {{StatType::kBytesReserved, 2000LL},
                {StatType::kBytesAllocated, 3000LL},
                {StatType::kBytesAvailable, 5000LL},
                {StatType::kPeakBytesInUse, 8500LL},
                {StatType::kRequestedBytes, 200LL},
                {StatType::kAllocationBytes, 256LL},
                {StatType::kAddress, 222333LL},
                {StatType::kStepId, -93746LL},
                {StatType::kDataType, 1LL},
                {StatType::kAllocatorName, "GPU_0_bfc"},
                {StatType::kTfOp, "foo/bar"},
                {StatType::kRegionType, "output"},
                {StatType::kTensorShapes, "[3, 3, 512, 512]"}});

  CreateXEvent(&host_plane_builder, &tf_executor_thread, "MemoryDeallocation",
               50000, 1000,
               {{StatType::kBytesReserved, 2000LL},
                {StatType::kBytesAllocated, 2744LL},
                {StatType::kBytesAvailable, 5256LL},
                {StatType::kPeakBytesInUse, 8500LL},
                {StatType::kRequestedBytes, 200LL},
                {StatType::kAllocationBytes, 256LL},
                {StatType::kAddress, 222333LL},
                {StatType::kStepId, 0LL},
                {StatType::kDataType, 0LL},
                {StatType::kAllocatorName, "GPU_0_bfc"},
                {StatType::kRegionType, ""},
                {StatType::kTensorShapes, ""}});

  CreateXEvent(&host_plane_builder, &tf_executor_thread, "MemoryAllocation",
               70000, 1000,
               {{StatType::kBytesReserved, 2000LL},
                {StatType::kBytesAllocated, 5000LL},
                {StatType::kBytesAvailable, 3000LL},
                {StatType::kPeakBytesInUse, 9500LL},
                {StatType::kRequestedBytes, 300LL},
                {StatType::kAllocationBytes, 300LL},
                {StatType::kAddress, 345678LL},
                {StatType::kStepId, -93746LL},
                {StatType::kDataType, 9LL},
                {StatType::kAllocatorName, "GPU_0_bfc"},
                {StatType::kTfOp, "mul_grad/Sum"},
                {StatType::kRegionType, "temp"},
                {StatType::kTensorShapes, "[1, 2]"}});

  MemoryProfile memory_profile = ConvertXPlaneToMemoryProfile(*host_plane);
  EXPECT_EQ(memory_profile.memory_profile_per_allocator().size(), 1);
  EXPECT_EQ(memory_profile.num_hosts(), 1);
  EXPECT_EQ(memory_profile.memory_ids_size(), 1);
  EXPECT_EQ(memory_profile.step_count().size(), 1);
  EXPECT_EQ(memory_profile.memory_profile_per_allocator().begin()->first,
            "GPU_0_bfc");
  const auto& allocator_memory_profile =
      memory_profile.memory_profile_per_allocator().begin()->second;
  EXPECT_EQ(
      allocator_memory_profile.profile_summary().peak_bytes_usage_lifetime(),
      9500);
  EXPECT_EQ(allocator_memory_profile.profile_summary()
                .peak_stats()
                .peak_bytes_in_use(),
            7000);
  EXPECT_EQ(allocator_memory_profile.profile_summary().peak_stats_time_ps(),
            70000);
  EXPECT_EQ(allocator_memory_profile.memory_profile_snapshots_size(), 3);
  EXPECT_EQ(allocator_memory_profile.active_allocations_size(), 3);
  EXPECT_EQ(
      allocator_memory_profile.active_allocations().at(2).snapshot_index(), 2);
  EXPECT_EQ(allocator_memory_profile.special_allocations_size(), 2);
  EXPECT_EQ(allocator_memory_profile.special_allocations().at(1).tf_op_name(),
            "stack");
  EXPECT_EQ(
      allocator_memory_profile.special_allocations().at(1).allocation_bytes(),
      2000);
}

}  // namespace
}  // namespace profiler
}  // namespace tensorflow
