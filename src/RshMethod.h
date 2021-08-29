#pragma once
#include <iostream>
#include <fstream>
#include <vector>
#include <set>
double R_totalRuntime = 0.0;

struct RshMethod {
  std::string id, name, context, hast;
  bool compiled = false;
  unsigned long contextI = 0;
  double rir2pir=0.0, opt = 0.0, bb = 0.0, p = 0.0, failed = 0.0, runtime = 0.0, effect = 0.0;

  std::set<std::string> callsTo;

  void writeToStream(std::ofstream & file) {
    file << "[";
    file << "\"" << id      << "\"" << ",";;
    file << "\"" << name    << "\"" << ",";;
    file << "\"" << context << "\"" << ",";;

    file << ((runtime / R_totalRuntime) * 100)  << ",";
    file << rir2pir << ",";
    file << opt     << ",";
    file << bb      << ",";
    file << p;

    file << "]";
  }
};

const std::vector<std::string> RshMethodParams {
  "id", "name", "context", "hast",
  "compiled",
  "contextI",
  "rir2pir", "opt", "bb", ""
};

struct ContextRuntimeSummary {
  int runs = 0;
  unsigned long contextI = 0;
  double cmp = 0.0, opt = 0.0, run = 0.0;
};

struct MethodRuntimeSummary {
  std::string hast;
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

void(*lineToObjectParser)(RshMethod &,std::string &,std::string &) = [](RshMethod & method, std::string & tp, std::string & del) {
  int start = 0;
  int end = tp.find(del);
  int index = 0;
  while (end != -1) {
    switch(index) {
      case 0: method.id          = tp.substr(start, end - start);            break;
      case 1: method.name        = tp.substr(start, end - start);            break;
      case 2: method.context     = tp.substr(start, end - start);            break;
      case 4: method.compiled    = std::stoi(tp.substr(start, end - start)); break;
      case 5: method.rir2pir     = std::stod(tp.substr(start, end - start)); break;
      case 6: method.opt         = std::stod(tp.substr(start, end - start)); break;
      case 7: method.bb          = std::stoi(tp.substr(start, end - start)); break;
      case 8: method.p           = std::stoi(tp.substr(start, end - start)); break;
      case 9: method.failed      = std::stod(tp.substr(start, end - start)); break;
      case 10: method.runtime    = std::stod(tp.substr(start, end - start)); break;
      case 11: method.effect     = std::stod(tp.substr(start, end - start)); break;
      case 12: method.hast       = tp.substr(start, end - start);            break;
      case 13: method.contextI   = std::stoul(tp.substr(start, end - start));break;
    }
    start = end + del.size();
    end = tp.find(del, start);
    index++;
  }
};

void(*singleLineObjectSerializer)(RshMethod &,std::ofstream &,std::string &) = [](RshMethod & m,std::ofstream & os, std::string & del) {
  os << m.id << del << m.name << del << m.context << del
    << m.compiled << del
    << m.rir2pir << del << m.opt << del << m.bb << del << m.p << del << m.failed << del << m.runtime << del << m.effect << del << m.hast << del << m.contextI;
  os << std::endl;
};

std::ostream& operator<<(std::ostream& os, const RshMethod& m) {
  std::string del = ",";
  os << "id: " << m.id << del << " name: " << m.name << del << " context: "  << m.context << del
    << " compiled: " << m.compiled << del
    << " rir2pir: " <<  m.rir2pir << del << " opt: " << m.opt << del << " bb: " << m.bb << del << " p: " << m.p << del << " failed: " << m.failed << del << " runtime: " << m.runtime << del << " effect: " << m.effect << del << " hast: " << m.hast << del << " contextI: " << m.contextI;
  return os;
}
