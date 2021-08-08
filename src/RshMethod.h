#pragma once
#include <iostream>
#include <iostream>

struct RshMethod {
  std::string id, name, context;
  bool compiled = false;
  double rir2pir=0.0, opt = 0.0, bb = 0.0, p = 0.0, failed = 0.0, runtime = 0.0, effect = 0.0;
};


struct ContextRuntimeSummary {
  int runs = 0;
  double cmp = 0.0, opt = 0.0, run = 0.0;
};

struct MethodRuntimeSummary {
  double rir2pir = 0.0, opt = 0.0, runtime = 0.0, effect = 0.0;
};

template <typename Parameter>
struct TempSortObj {
  std::string id;
  Parameter param;
};

template <typename Parameter>
struct ObjectSorter {
  inline bool operator() (const TempSortObj<Parameter>& struct1, const TempSortObj<Parameter>& struct2) {
    return (struct1.param > struct2.param);
  }
};

struct Meta {
  std::string name;
  int runs;
};