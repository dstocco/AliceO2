// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

///
/// @author  Diego Stocco

#include <benchmark/benchmark.h>
#include <vector>
#include <random>
#include <iostream>
#include <algorithm>
#include <boost/serialization/access.hpp>
#include <boost/serialization/array.hpp>
#include <boost/serialization/vector.hpp>
#include "Framework/TMessageSerializer.h"
#include "CommonUtils/BoostSerializer.h"
#include "DataFormatsMID/ColumnData.h"

namespace o2
{
namespace mid
{

struct MyTestData {
  int deId;                   ///< Index of the detection element
  std::array<int, 5> payload; ///< payload
};

class MyTestDataRoot : public MyTestData
{
  ClassDefNV(MyTestDataRoot, 1);
};

class MyTestDataBoost : public MyTestData
{
  friend class boost::serialization::access;
  template <class Archive>
  void serialize(Archive& ar, const unsigned int version)
  {
    ar& deId& payload;
  }
};
} // namespace mid
} // namespace o2

template <typename T>
std::vector<T> getData(int nvalues = 1000)
{
  std::random_device rd;
  std::mt19937 mt(rd());
  std::uniform_int_distribution<int> deId(0, 71);
  std::uniform_int_distribution<int> payload(0, 0xFFFF);

  std::vector<T> vec;
  for (int ival = 0; ival < nvalues; ++ival) {
    T data;
    data.deId = static_cast<uint8_t>(deId(mt));
    for (auto& pl : data.payload) {
      pl = payload(mt);
    }
    vec.emplace_back(data);
  }
  return vec;
}

static void BM_ROOT(benchmark::State& state)
{

  int nValues = state.range(0);

  auto data = getData<o2::mid::MyTestDataRoot>(nValues);
  auto* cl = TClass::GetClass(typeid(data));

  for (auto _ : state) {
    o2::framework::FairTMessage msg;
    o2::framework::TMessageSerializer::serialize(msg, &data, cl);
    auto buf = as_span(msg);
    auto out = o2::framework::TMessageSerializer::deserialize<std::vector<o2::mid::MyTestDataRoot>>(buf);
  }
}

BENCHMARK(BM_ROOT)->RangeMultiplier(10)->Range(1, 1000);

static void BM_BOOST(benchmark::State& state)
{

  int nValues = state.range(0);
  auto data = getData<o2::mid::MyTestDataBoost>(nValues);

  for (auto _ : state) {
    auto msgStr = o2::utils::BoostSerialize(data).str();
    auto out = o2::utils::BoostDeserialize<std::vector<o2::mid::MyTestDataBoost>>(msgStr);
  }
}

BENCHMARK(BM_BOOST)->RangeMultiplier(10)->Range(1, 1000);

BENCHMARK_MAIN();
