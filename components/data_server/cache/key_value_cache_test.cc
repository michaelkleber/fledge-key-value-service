// Copyright 2022 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "components/data_server/cache/cache.h"
#include "components/data_server/cache/mocks.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "public/base_types.pb.h"

namespace fledge::kv_server {
namespace {

using testing::UnorderedElementsAre;

TEST(CacheTest, RetrievesMatchingEntry) {
  std::unique_ptr<ShardedCache> sharded_cache = ShardedCache::Create();
  Cache& cache = sharded_cache->GetMutableCacheShard(KeyNamespace::KEYS);
  cache.UpdateKeyValue({"my_key", "my_subkey"}, "my_value");

  Cache::Key cache_key;
  cache_key.key = "my_key";
  cache_key.subkey = "my_subkey";
  std::vector<Cache::Key> cache_keys = {cache_key};

  std::vector<std::pair<Cache::Key, std::string>> kv_pairs =
      cache.GetKeyValuePairs(cache_keys);
  EXPECT_TRUE(
      sharded_cache->GetCacheShard(KeyNamespace::AD_COMPONENT_RENDER_URLS)
          .GetKeyValuePairs(cache_keys)
          .empty());
  EXPECT_THAT(kv_pairs, UnorderedElementsAre(KVPairEq(cache_key, "my_value")));
}

TEST(CacheTest, GetWithMultipleKeysReturnsMatchingValues) {
  std::unique_ptr<Cache> cache = Cache::Create();
  cache->UpdateKeyValue({"key1", ""}, "value1");
  cache->UpdateKeyValue({"key2", ""}, "value2");
  cache->UpdateKeyValue({"key3", ""}, "value3");

  Cache::Key cache_key1;
  cache_key1.key = "key1";
  Cache::Key cache_key2;
  cache_key2.key = "key2";
  std::vector<Cache::Key> cache_keys = {cache_key1, cache_key2};

  std::vector<std::pair<Cache::Key, std::string>> kv_pairs =
      cache->GetKeyValuePairs(cache_keys);
  EXPECT_EQ(kv_pairs.size(), 2);
  EXPECT_THAT(kv_pairs, UnorderedElementsAre(KVPairEq(cache_key1, "value1"),
                                             KVPairEq(cache_key2, "value2")));
}

TEST(CacheTest, GetAfterUpdateReturnsNewValue) {
  std::unique_ptr<Cache> cache = Cache::Create();
  cache->UpdateKeyValue({"my_key", ""}, "my_value");

  Cache::Key cache_key;
  cache_key.key = "my_key";
  std::vector<Cache::Key> cache_keys = {cache_key};

  std::vector<std::pair<Cache::Key, std::string>> kv_pairs =
      cache->GetKeyValuePairs(cache_keys);
  EXPECT_THAT(kv_pairs, UnorderedElementsAre(KVPairEq(cache_key, "my_value")));

  cache->UpdateKeyValue({"my_key", ""}, "my_new_value");

  kv_pairs = cache->GetKeyValuePairs(cache_keys);
  EXPECT_EQ(kv_pairs.size(), 1);
  EXPECT_THAT(kv_pairs,
              UnorderedElementsAre(KVPairEq(cache_key, "my_new_value")));
}

TEST(CacheTest, GetAfterUpdateDifferentKeyWithSameSubkeyReturnsSameValue) {
  std::unique_ptr<Cache> cache = Cache::Create();
  cache->UpdateKeyValue({"my_key", "my_subkey"}, "my_value");
  cache->UpdateKeyValue({"new_key", "my_subkey"}, "new_value");

  Cache::Key cache_key;
  cache_key.subkey = "my_subkey";
  cache_key.key = "my_key";
  std::vector<Cache::Key> cache_keys = {cache_key};

  std::vector<std::pair<Cache::Key, std::string>> kv_pairs =
      cache->GetKeyValuePairs(cache_keys);
  EXPECT_THAT(kv_pairs, UnorderedElementsAre(KVPairEq(cache_key, "my_value")));
}

TEST(CacheTest, GetForEmptyCacheReturnsEmptyList) {
  std::unique_ptr<Cache> cache = Cache::Create();

  Cache::Key cache_key;
  cache_key.key = "my_key";
  std::vector<Cache::Key> cache_keys = {cache_key};

  std::vector<std::pair<Cache::Key, std::string>> kv_pairs =
      cache->GetKeyValuePairs(cache_keys);
  EXPECT_EQ(kv_pairs.size(), 0);
}

TEST(CacheTest, GetValuesForMissingSubkeyRetrievesEmptySubkeyEntry) {
  std::unique_ptr<Cache> cache = Cache::Create();
  cache->UpdateKeyValue({"my_key", ""}, "my_value");

  Cache::Key cache_key;
  cache_key.key = "my_key";
  cache_key.subkey = "wrong_subkey";
  std::vector<Cache::Key> cache_keys = {cache_key};

  std::vector<std::pair<Cache::Key, std::string>> kv_pairs =
      cache->GetKeyValuePairs(cache_keys);
  EXPECT_EQ(kv_pairs.size(), 1);
  EXPECT_THAT(kv_pairs, UnorderedElementsAre(KVPairEq(cache_key, "my_value")));
}

TEST(CacheTest,
     GetValuesForMissingSubkeyWithoutEmptySubkeyEntryReturnsEmptyList) {
  std::unique_ptr<Cache> cache = Cache::Create();
  cache->UpdateKeyValue({"my_key", "my_subkey"}, "my_value");

  Cache::Key cache_key;
  cache_key.key = "my_key";
  std::vector<Cache::Key> cache_keys = {cache_key};

  std::vector<std::pair<Cache::Key, std::string>> kv_pairs =
      cache->GetKeyValuePairs(cache_keys);
  EXPECT_EQ(kv_pairs.size(), 0);
}

TEST(CacheTest, UpdateWithoutSubkeySetsEmptySubkey) {
  std::unique_ptr<Cache> cache = Cache::Create();
  cache->UpdateKeyValue({"my_key", ""}, "my_value");

  Cache::Key cache_key;
  cache_key.key = "my_key";
  cache_key.subkey = "";
  std::vector<Cache::Key> cache_keys = {cache_key};

  std::vector<std::pair<Cache::Key, std::string>> kv_pairs =
      cache->GetKeyValuePairs(cache_keys);
  EXPECT_EQ(kv_pairs.size(), 1);
  EXPECT_THAT(kv_pairs, UnorderedElementsAre(KVPairEq(cache_key, "my_value")));
}

TEST(DeleteKeyTest, RemovesKeyEntry) {
  std::unique_ptr<Cache> cache = Cache::Create();
  cache->UpdateKeyValue({"my_key", ""}, "my_value");
  cache->DeleteKey({"my_key", ""});

  Cache::Key cache_key;
  cache_key.key = "my_key";
  std::vector<Cache::Key> cache_keys = {cache_key};

  std::vector<std::pair<Cache::Key, std::string>> kv_pairs =
      cache->GetKeyValuePairs(cache_keys);
  EXPECT_EQ(kv_pairs.size(), 0);
}

TEST(DeleteKeyTest, WrongSubkeyDoesNotRemoveEntry) {
  std::unique_ptr<Cache> cache = Cache::Create();
  cache->UpdateKeyValue({"my_key", ""}, "my_value");
  cache->DeleteKey({"my_key", "wrong_subkey"});

  Cache::Key cache_key;
  cache_key.key = "my_key";
  std::vector<Cache::Key> cache_keys = {cache_key};

  std::vector<std::pair<Cache::Key, std::string>> kv_pairs =
      cache->GetKeyValuePairs(cache_keys);
  EXPECT_THAT(kv_pairs, UnorderedElementsAre(KVPairEq(cache_key, "my_value")));
}

TEST(DeleteKeyTest, RemoveKeyKeepsOtherKeysWithSameSubkey) {
  std::unique_ptr<Cache> cache = Cache::Create();
  cache->UpdateKeyValue({"key1", "subkey"}, "value1");
  cache->UpdateKeyValue({"key2", "subkey"}, "value2");

  cache->DeleteKey({"key2", "subkey"});

  Cache::Key cache_key;
  cache_key.subkey = "subkey";
  cache_key.key = "key1";
  std::vector<Cache::Key> cache_keys = {cache_key};

  std::vector<std::pair<Cache::Key, std::string>> kv_pairs =
      cache->GetKeyValuePairs(cache_keys);
  EXPECT_THAT(kv_pairs, UnorderedElementsAre(KVPairEq(cache_key, "value1")));
}

}  // namespace
}  // namespace fledge::kv_server
