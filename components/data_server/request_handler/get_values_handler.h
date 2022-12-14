/*
 * Copyright 2022 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef COMPONENTS_DATA_SERVER_REQUEST_HANDLER_GET_VALUES_HANDLER_H_
#define COMPONENTS_DATA_SERVER_REQUEST_HANDLER_GET_VALUES_HANDLER_H_

#include <grpcpp/grpcpp.h>

#include "components/data_server/cache/cache.h"
#include "public/query/get_values.grpc.pb.h"
#include "src/google/protobuf/struct.pb.h"

namespace fledge::kv_server {

using v1::GetValuesRequest;
using v1::GetValuesResponse;

// Handles GetValuesRequests.
class GetValuesHandler {
 public:
  explicit GetValuesHandler(const ShardedCache& sharded_cache, bool dsp_mode)
      : sharded_cache_(sharded_cache), dsp_mode_(dsp_mode) {}

  // TODO: Implement subkey, ad/render url lookups.
  grpc::Status GetValues(const GetValuesRequest& request,
                         GetValuesResponse* response) const;

 private:
  grpc::Status ValidateRequest(const GetValuesRequest& request) const;

  const ShardedCache& sharded_cache_;

  // Use DSP mode for request validation. If false, then automatically assumes
  // SSP mode.
  const bool dsp_mode_;
};

}  // namespace fledge::kv_server

#endif  // COMPONENTS_DATA_SERVER_REQUEST_HANDLER_GET_VALUES_HANDLER_H_
