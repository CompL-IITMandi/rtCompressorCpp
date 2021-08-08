#define MAX_RT_STACK_SIZE 1000000
#include <iostream>
#include <string>
#include <stack>
#include <vector>
#include <sstream>
#include <fstream>
#include "./HugeStackManager.h"
#include "./RshMethod.h"
#include <stdlib.h>
#include <time.h>
#include <unordered_map>
#include <set>
#include <algorithm>
// lineData = line.split(",")
//
// Method Start: =
// id: lineData[1].slice(1,-1),
// name: lineData[2].slice(1,-1),
// compilation: { compiled: false },
//
// Method Compilation: @
// method["compilation"]["compiled"] = true;
// method["compilation"]["rir2pir"] = parseFloat(lineData[2]);
// method["compilation"]["opt"] = parseFloat(lineData[3]);
// method["compilation"]["bb"] = parseInt(lineData[4]);
// method["compilation"]["p"] = parseInt(lineData[5]);
// method["compilation"]["compiled"] = false;
// method["compilation"]["failed"] = parseFloat(lineData[2]);
//
// Method End: !
// method["context"] = "baseline";
// method["runtime"] = parseFloat(lineData[lineData.length - 1]);
// method["effect"] = 0;
// 
// Log End: ##

std::ostream& operator<<(std::ostream& os, const RshMethod& m)
{
    os << m.id << "," << m.name << "," << m.context << ","
      << m.compiled << ","
      << m.rir2pir << "," << m.opt << "," << m.bb << "," << m.p << "," << m.failed << "," << m.runtime << "," << m.effect;
    return os;
}

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
    }
    start = end + del.size();
    end = tp.find(del, start);
    index++;
  }
};

void(*singleLineObjectSerializer)(RshMethod &,std::ofstream &,std::string &) = [](RshMethod & m,std::ofstream & os, std::string & del) {
  os << m.id << del << m.name << del << m.context << del
    << m.compiled << del
    << m.rir2pir << del << m.opt << del << m.bb << del << m.p << del << m.failed << del << m.runtime << del << m.effect;
  os << std::endl;
};

class RshJsonParser {
  private:
  HugeStackManager<RshMethod>* hsm;
  std::string & del;

  std::unordered_map<std::string, std::unordered_map<std::string,std::vector<double>>> methodCSMap;     // [id,context] -> [runtimes]
  std::unordered_map<std::string, std::unordered_map<std::string,ContextRuntimeSummary>> methodCTMap;   // [id,context] -> { runs, cmp, opt, run }
  std::unordered_map<std::string, MethodRuntimeSummary> methodRuntimeMap;                               // id -> { rir2pir, opt, runtime, effect; }
  std::unordered_map<std::string, std::set<std::string>> methodContextMap;                              // id -> set [ context ]
  std::unordered_map<std::string, std::set<std::string>> methodCallMap;                                 // id -> set [ ids ]
  std::unordered_map<std::string, Meta> methodMeta;                                                     // id -> { name, runs }

  std::unordered_map<std::string, std::vector<int>> boxContextCallRuns;                                 // id -> [ contexts ]
  int colorGenerator = 1;
  std::unordered_map<std::string, int> boxContextResolution;                                            // context -> uid

  std::unordered_map<std::string, std::vector<RshMethod>> methodsThatCompiledMap;                       // context -> [ RshMethod ]

  std::vector<std::string> s_runtime, s_calls, s_contexts;                                              // [ ids ], [ ids ], [ ids ]

  double totalRuntime;

  void removeQuotes(std::string & str) {
    str.erase(0, 1);
    str.erase(str.size() - 1);
  }

  void pushMethod(std::string & line) {
    RshMethod method;
    size_t pos = 0;
    std::string token;
    int index = 0;
    while ((pos = line.find(del)) != std::string::npos) {
      token = line.substr(0, pos);
      switch(index) {
        case 1: removeQuotes(token); method.id=token; break;
      }
      line.erase(0, pos + del.length());
      index++;
    }
    removeQuotes(line);
    method.name = line;

    if (!hsm->empty()) {
      RshMethod parent = hsm->pop();
      updateMethodCallMap(parent.id, method.id);
      hsm->push(parent);
    }

    hsm->push(method);
  }

  void updateMethodCallMap(std::string & id, std::string & child) {
    methodCallMap[id].insert(child);
  }

  // @,       - 0     @
  // true,    - 1     false
  // 1.79932, - 2     0.003796
  // 35.5841, - 3
  // 42,      - 4
  // 8,       - 5

  void addMeta(std::string & line) {
    RshMethod method = hsm->pop();
    size_t pos = 0;
    std::string token;
    int index = 0;
    bool compiled = false;
    while ((pos = line.find(del)) != std::string::npos) {
      token = line.substr(0, pos);
      // std::cout << "token: " << token << std::endl;
      switch(index) {
        case 1: if (token.compare("true") == 0) compiled = true; break;
        case 2: method.rir2pir = std::stod(token); break;
        case 3: method.opt     = std::stod(token); break;
        case 4: method.bb      = std::stoi(token); break;
      }
      line.erase(0, pos + del.length());
      index++;
    }
    method.compiled = compiled;
    if (compiled) {
      method.p = std::stoi(line);
    } else {
      method.failed = std::stod(line);
    }
    hsm->push(method);
  }

  void finalizeMethod(std::string & line) {
    RshMethod method = hsm->pop();
    size_t pos = 0;
    std::string token, del = "\"";
    int index = 0;
    while ((pos = line.find(del)) != std::string::npos) {
      token = line.substr(0, pos);
      switch(index) {
        case 1: method.context=(token.compare("") == 0) ? "baseline" : token; break;
      }
      line.erase(0, pos + del.length());
      index++;
    }
    line.erase(0, 1);
    method.runtime = std::stod(line);
    updateMethodCSMap(method);
    updateMethodCTMap(method);
    updateMethodRuntimeMap(method);
    updateMethodContextMap(method);
    updateMethodMeta(method);
    updateBoxContextCallRuns(method);
    updateMethodsThatCompiledMap(method);
  }

  void updateMethodCSMap(RshMethod & method) {
    methodCSMap[method.id][method.context].push_back(method.runtime);
  }

  void updateMethodCTMap(RshMethod & method) {
    methodCTMap[method.id][method.context].runs++;
    methodCTMap[method.id][method.context].cmp += method.rir2pir;
    methodCTMap[method.id][method.context].opt += method.opt;
    methodCTMap[method.id][method.context].run += method.runtime;
  }

  void updateMethodRuntimeMap(RshMethod & method) {
    methodRuntimeMap[method.id].rir2pir += method.rir2pir;
    methodRuntimeMap[method.id].opt += method.opt;
    methodRuntimeMap[method.id].runtime += method.runtime;
    methodRuntimeMap[method.id].effect += method.effect;
  }

  void updateMethodContextMap(RshMethod & method) {
    methodContextMap[method.id].insert(method.context);
  }

  void updateMethodMeta(RshMethod & method) {
    methodMeta[method.id].name = method.name;
    methodMeta[method.id].runs++;
  }

  void updateBoxContextCallRuns(RshMethod & method) {
    if (boxContextResolution.find(method.context) == boxContextResolution.end()) { // if key does not exist, generate a new one
      int color = colorGenerator++;
      boxContextResolution[method.context] = color;
    }
    boxContextCallRuns[method.id].push_back(boxContextResolution[method.context]);
  }

  void updateMethodsThatCompiledMap(RshMethod & method) {
    if (method.compiled) {
      methodsThatCompiledMap[method.id].push_back(method);
    }
  }

  void createSorts() {
    std::vector<TempSortObj<double>> t_runtime;
    std::vector<TempSortObj<int>> t_contexts, t_calls;
    for (auto & method : methodRuntimeMap) {
      TempSortObj<double> t1;
      t1.id = method.first;
      t1.param = method.second.runtime;
      t_runtime.push_back(t1);

      TempSortObj<int> t2;
      t2.id = method.first;
      t2.param = methodContextMap[method.first].size();
      t_contexts.push_back(t2);

      TempSortObj<int> t3;
      t3.id = method.first;
      t3.param = methodMeta[method.first].runs;
      t_calls.push_back(t3);
    }
    std::sort(t_runtime.begin(), t_runtime.end(), ObjectSorter<double>());
    std::sort(t_contexts.begin(), t_contexts.end(), ObjectSorter<int>());
    std::sort(t_calls.begin(), t_calls.end(), ObjectSorter<int>());
    for (auto & ele : t_runtime) {
      s_runtime.push_back(ele.id);
    }
    for (auto & ele : t_contexts) {
      s_contexts.push_back(ele.id);
    }
    for (auto & ele : t_calls) {
      s_calls.push_back(ele.id);
    }
  }

  void saveSortsToStream(std::ofstream & jsonFile) {
    jsonFile << "\"s_runtime\": [";
    int index = 0;
    for (auto & ele : s_runtime) {
      jsonFile << "\"" << ele << "\"";
      index++;
      if (index != s_runtime.size()) jsonFile << ",";
    }
    jsonFile << "],\n";
    jsonFile << "\"s_calls\": [";
    index = 0;
    for (auto & ele : s_calls) {
      jsonFile << "\"" << ele << "\"";
      index++;
      if (index != s_calls.size()) jsonFile << ",";
    }
    jsonFile << "],\n";
    jsonFile << "\"s_contexts\": [";
    index = 0;
    for (auto & ele : s_contexts) {
      jsonFile << "\"" << ele << "\"";
      index++;
      if (index != s_contexts.size()) jsonFile << ",";
    }
    jsonFile << "],\n";
  }

  void saveMethodCTMap(std::ofstream & jsonFile) {
    jsonFile << "\"methodCTMap\": {";
    int index_outer = 0;
    for (auto & method : methodCTMap) {
      jsonFile << "\"" << method.first << "\":{";
      int index_inner = 0;
      for (auto & context : method.second) {
        // std::cout << context.first << "->" << context.second.run << std::endl;
        jsonFile << "\"" << context.first << "\":{";
        jsonFile << "\"runs\":" << context.second.runs << ",";
        jsonFile << "\"cmp\":" << context.second.cmp << ",";
        jsonFile << "\"opt\":" << context.second.opt << ",";
        jsonFile << "\"run\":" << context.second.run << "}";
        index_inner++;
        if (index_inner != method.second.size()) jsonFile << ",";
      }
      jsonFile << "}";
      index_outer++;
      if (index_outer != methodCTMap.size()) jsonFile << ",";
    }
    jsonFile << "},\n";
  }

  void saveMethodCSMap(std::ofstream & jsonFile) {
    jsonFile << "\"methodCSMap\": {";
    int index_outer = 0;
    for (auto & method : methodCSMap) {
      jsonFile << "\"" << method.first << "\":{";
      int index_inner = 0;
      for (auto & context : method.second) {
        // std::cout << context.first << "->" << context.second.run << std::endl;
        jsonFile << "\"" << context.first << "\":[";
        int index_inner_inner = 0;
        for (auto & ele : context.second) {
          jsonFile << ele;
          index_inner_inner++;
          if (index_inner_inner != context.second.size()) jsonFile << ",";
        }
        jsonFile << "]";
        index_inner++;
        if (index_inner != method.second.size()) jsonFile << ",";
      }
      jsonFile << "}";
      index_outer++;
      if (index_outer != methodCSMap.size()) jsonFile << ",";
    }
    jsonFile << "},\n";
  }

  void saveMethodMeta(std::ofstream & jsonFile) {
    jsonFile << "\"methodMeta\": {";
    int index_outer = 0;
    for (auto & method : methodMeta) {
      jsonFile << "\"" << method.first << "\":{";
      jsonFile << "\"name\":" << "\"" << method.second.name << "\",";
      jsonFile << "\"runs\":" << method.second.runs;
      jsonFile << "}";
      index_outer++;
      if (index_outer != methodMeta.size()) jsonFile << ",";
    }
    jsonFile << "},\n";
  }

  void saveMethodsThatCompiledMap(std::ofstream & jsonFile) {
    jsonFile << "\"methodsThatCompiledMap\": {";
    int index_outer = 0;
    for (auto & method : methodsThatCompiledMap) {
      jsonFile << "\"" << method.first << "\":[";
      int index_inner = 0;
      for (auto & rshMethod : method.second) {
        jsonFile << "[";

        jsonFile << "\"" << rshMethod.id << "\",";
        jsonFile << "\"" << rshMethod.name << "\",";
        jsonFile << "\"" << rshMethod.context << "\",";
        jsonFile << rshMethod.runtime << ",";
        jsonFile << ((rshMethod.runtime / totalRuntime) * 100) << ",";
        jsonFile << rshMethod.rir2pir << ",";
        jsonFile << rshMethod.opt << ",";
        jsonFile << rshMethod.bb << ",";
        jsonFile << rshMethod.p;

        jsonFile << "]";
        index_inner++;
        if (index_inner != method.second.size()) jsonFile << ",";
      }
      jsonFile << "]";
      index_outer++;
      if (index_outer != methodsThatCompiledMap.size()) jsonFile << ",";
    }
    jsonFile << "},\n";
  }

  void saveMethodRuntimeMap(std::ofstream & jsonFile) {
    jsonFile << "\"methodRuntimeMap\": {";
    int index_outer = 0;
    for (auto & method : methodRuntimeMap) {
      jsonFile << "\"" << method.first << "\":{";
      // {"runtime":673.517174,"effect":0,"rir2pir":5.9830000000000005,"opt":132.9219},
      jsonFile << "\"runtime\":" << method.second.runtime << ",";
      jsonFile << "\"effect\":" << ((method.second.runtime / totalRuntime) * 100) << ",";
      jsonFile << "\"rir2pir\":" << method.second.rir2pir << ",";
      jsonFile << "\"opt\":" << method.second.opt;
      jsonFile << "}";
      index_outer++;
      if (index_outer != methodRuntimeMap.size()) jsonFile << ",";
    }
    jsonFile << "},\n";
  }

  void saveMethodContextMap(std::ofstream & jsonFile) {
    jsonFile << "\"methodContextMap\": {";
    int index_outer = 0;
    for (auto & method : methodContextMap) {
      jsonFile << "\"" << method.first << "\":[";
      int index_inner = 0;
      for (auto & context : method.second) {
        jsonFile << "\"" << context << "\"";
        index_inner++;
        if (index_inner != method.second.size()) jsonFile << ",";
      }
      jsonFile << "]";
      index_outer++;
      if (index_outer != methodContextMap.size()) jsonFile << ",";
    }
    jsonFile << "},\n";
  }

  void saveMethodCallMap(std::ofstream & jsonFile) {
    jsonFile << "\"methodCallMap\": {";
    int index_outer = 0;
    for (auto & method : methodCallMap) {
      jsonFile << "\"" << method.first << "\":[";
      int index_inner = 0;
      for (auto & called_method_id : method.second) {
        jsonFile << "\"" << called_method_id << "\"";
        index_inner++;
        if (index_inner != method.second.size()) jsonFile << ",";
      }
      jsonFile << "]";
      index_outer++;
      if (index_outer != methodCallMap.size()) jsonFile << ",";
    }
    jsonFile << "},\n";
  }

  void saveBoxContextCallRuns(std::ofstream & jsonFile) {
    jsonFile << "\"boxContextCallRuns\": {";
    int index_outer = 0;
    for (auto & method : boxContextCallRuns) {
      jsonFile << "\"" << method.first << "\":[";
      int index_inner = 0;
      for (auto & dispatched_context : method.second) {
        jsonFile << dispatched_context;
        index_inner++;
        if (index_inner != method.second.size()) jsonFile << ",";
      }
      jsonFile << "]";
      index_outer++;
      if (index_outer != boxContextCallRuns.size()) jsonFile << ",";
    }
    jsonFile << "},\n";
  }

  void saveBoxContextResolution(std::ofstream & jsonFile) {
    jsonFile << "\"boxContextResolution\": {";
    int index_outer = 0;
    for (auto & context : boxContextResolution) {
      jsonFile << "\"" << context.first << "\":" << context.second;
      index_outer++;
      if (index_outer != boxContextResolution.size()) jsonFile << ",";
    }
    jsonFile << "}\n";
  }

  public:
  RshJsonParser(std::string & d) : del(d) {
    hsm = new HugeStackManager<RshMethod>(lineToObjectParser,singleLineObjectSerializer,d);
    totalRuntime = 0;
  }

  void processLine(std::string & line) {
    int start = 0;
    int end = line.find(del);
    std::string lookahead = line.substr(start, end - start);

    if (lookahead.compare("=") == 0) {
      pushMethod(line);
    } else if (lookahead.compare("!") == 0) {
      finalizeMethod(line);
    } else if (lookahead.compare("@") == 0) {
      addMeta(line);
    } else {
      createSorts();

      size_t pos = 0;
      std::string token;
      while ((pos = line.find(del)) != std::string::npos) {
        token = line.substr(0, pos);
        line.erase(0, pos + del.length());
      }
      totalRuntime = std::stod(line);

      std::cout << "initial parsing completed" << std::endl;

      // for (auto & method : methodCSMap) {
      //   std::cout << "id: " << method.first << std::endl;
      //   for (auto & context : method.second) {
      //     std::cout << context.first << "->" << context.second[0] << std::endl;
      //   }
      // }

      // for (auto & method : methodCTMap) {
      //   std::cout << "id: " << method.first << std::endl;
      //   for (auto & context : method.second) {
      //     std::cout << context.first << "->" << context.second.run << std::endl;
      //   }
      // }

      // for (auto & method : methodRuntimeMap) {
      //   std::cout << "id: " << method.first << "," << method.second.runtime << std::endl;
      // }

      // for (auto & method : methodContextMap) {
      //   std::cout << "id: " << method.first << "(" << methodMeta[method.first].name << ")  ";
      //   for (auto & context : method.second) {
      //     std::cout << context << "  ";
      //   }
      //   std::cout << std::endl;
      // }

      // for (auto & method : methodCallMap) {
      //   std::cout << "id: " << method.first << "(" << methodMeta[method.first].name << ")  ";
      //   for (auto & id : method.second) {
      //     std::cout << methodMeta[id].name << "  ";
      //   }
      //   std::cout << std::endl;
      // }

      // for (auto & method : boxContextCallRuns) {
      //   std::cout << "id: " << method.first << "(" << methodMeta[method.first].name << ") : [ ";
      //   for (auto & id : method.second) {
      //     std::cout << id << " ";
      //   }
      //   std::cout  << " ]" << std::endl;
      // }

      // for (auto & method : methodsThatCompiledMap) {
      //   std::cout << "id: " << method.first << "(" << methodMeta[method.first].name << ") : [ ";
      //   for (auto & m : method.second) {
      //     std::cout << m << " ";
      //   }
      //   std::cout  << " ]" << std::endl;
      // }

      // std::cout << "## :" << line << std::endl;
    }
  }

  void saveJson(std::string path) {
    std::ofstream jsonFile;
    jsonFile.open(path);

    jsonFile << "{\n";

    saveSortsToStream(jsonFile);
    saveMethodCTMap(jsonFile);
    saveMethodCSMap(jsonFile);
    
    jsonFile << "\"totalRuntime\":" << totalRuntime << ",\n";

    saveMethodMeta(jsonFile);
    saveMethodsThatCompiledMap(jsonFile);
    saveMethodRuntimeMap(jsonFile);
    saveMethodContextMap(jsonFile);
    saveMethodCallMap(jsonFile);
    saveBoxContextCallRuns(jsonFile);
    saveBoxContextResolution(jsonFile);
    jsonFile << "\n}";

    jsonFile.close();

  }
};

bool validInputPath(char * inputPath) {
  char * t = inputPath;
  while (*t != '\0') {
    if (*t == '.') {
      if (*(++t) == 'l' && *(++t) == 'o' && *(++t) == 'g' && *(++t) == 'g') return true;
      return false;
    }
    t++;
  }
  std::cout << std::endl;
  return false;
}

bool validOutputPath(char * inputPath) {
  char * t = inputPath;
  while (*t != '\0') {
    if (*t == '.') {
      if (*(++t) == 'j' && *(++t) == 's' && *(++t) == 'o' && *(++t) == 'n') return true;
      return false;
    }
    t++;
  }
  std::cout << std::endl;
  return false;
}

int main(int argc, char** argv) {
  srand(time(0));

  std::string del = ",";

  char* inputPath = argv[1];
  char* outputPath = argv[2];
  
  if (!validInputPath(inputPath)) {
    std::cerr << "ERR: Input file path not specified [with .logg extension]" << std::endl;
    exit(0);
  }
  if (!validOutputPath(outputPath)) {
    std::cerr << "ERR: Output file path not specified [with .json extension]" << std::endl;
    exit(0);
  }

  std::ifstream loggFile;
  loggFile.open(inputPath);
  if( !loggFile ) {
    std::cerr << "ERR: Unable to open logg file" << std::endl;
    exit(1);
  }
  
  RshJsonParser parser(del);

  // Start processing the file
  std::string line;
  while(getline(loggFile, line)){
    parser.processLine(line);
  }
  loggFile.close();

  parser.saveJson(outputPath);
  
  
}