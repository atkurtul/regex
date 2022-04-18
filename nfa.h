
#include <assert.h>
#include <stdint.h>
#include <algorithm>
#include <bitset>
#include <map>
#include <set>
#include <string>
#include <typeinfo>
#include <vector>

typedef uint8_t u8;
typedef uint32_t u32;

template <class T>
T* alloc(T t, const char* file, int line) {
  T* p = new T(t);
  printf("Allocationg %s at ./%s:%d\n", typeid(T).name(), file, line);
  return p;
}

#define ALLOC(...) alloc(__VA_ARGS__, __FILE__, __LINE__)
#define impossible                                       \
  {                                                      \
    printf("./%s:%d: impossible\n", __FILE__, __LINE__); \
    abort();                                             \
  }

struct DFA {
  DFA* edges[256];
  bool halting;
  inline static int next_id = 0;
  int id = next_id++;

  void print_(std::set<DFA*>& visited) {
    if (visited.contains(this))
      return;
    visited.insert(this);
    const char* tag[] = {"@", "$"};

    printf("%s[%d]:\n", tag[halting], id);

    for (int i = 0; i < 256; ++i)
      if (edges[i])
        printf("\t%d -> [ %d ]\n", i, edges[i]->id);

    for (int i = 0; i < 256; ++i)
      if (edges[i])
        edges[i]->print_(visited);
  }

  void print() {
    std::set<DFA*> v = {};
    print_(v);
  }
};

struct NFA {
  std::map<u8, std::set<NFA*>> edges;
  bool halting;
  inline static int next_id = 0;
  int id = next_id++;

  void print_(std::set<NFA*>& visited) {
    if (visited.contains(this))
      return;
    visited.insert(this);
    const char* tag[] = {"@", "$"};

    printf("%s[%d]:\n", tag[halting], id);
    for (auto& [c, v] : edges) {
      printf("\t%c -> [ ", c);
      for (auto& n : v)
        printf("%d ", n->id);
      printf("]\n");
    }
    for (auto& [c, v] : edges)
      for (auto& n : v)
        n->print_(visited);
  }

  void print() {
    std::set<NFA*> v = {};
    print_(v);
  }

  static DFA* to_dfa_(std::set<NFA*> nfa,
                      std::map<std::set<NFA*>, DFA*>& cache) {
    if (nfa.empty())
      return 0;

    if (auto dfa = cache.find(nfa); dfa != cache.end())
      return dfa->second;

    DFA* dfa = cache[nfa] = ALLOC(DFA{});
    for (int i = 0; i < 256; ++i) {
      std::set<NFA*> merged;
      for (auto n : nfa) {
        dfa->halting |= n->halting;
        merged.insert(n->edges[i].begin(), n->edges[i].end());
      }
      dfa->edges[i] = to_dfa_(merged, cache);
    }

    return dfa;
  }

  DFA* to_dfa() {
    std::map<std::set<NFA*>, DFA*> cache;
    return to_dfa_(std::set<NFA*>{this}, cache);
  }
};

struct Label {
  std::bitset<256> c;
  enum { JMP, ALT, HALT } tag;
  Label* next;
  Label* alt;

  inline static int next_id = 0;
  int id = next_id++;

  bool has_match();
  bool match(char p, std::vector<Label*>& states);
  void print_header(int d);
  void print(int d = 0, Label* inc = 0);

  NFA* to_nfa_(std::map<Label*, NFA*>& cache) {
    if (auto it = cache.find(this); it != cache.end())
      return it->second;

    NFA* nfa = cache[this] = ALLOC(NFA{});
    std::set<Label*> self;
    gather(nfa->halting, self);

    for (auto e : self)
      for (int i = 0; i < 256; ++i)
        if (e->c.test(i))
          nfa->edges[i].insert(e->next->to_nfa_(cache));

    return nfa;
  }

  NFA* to_nfa() {
    std::map<Label*, NFA*> cache;
    return to_nfa_(cache);
  }

  void gather(bool& halting, std::set<Label*>& labels) {
    switch (tag) {
      case HALT:
        halting = true;
        break;
      case ALT:
        next->gather(halting, labels);
        alt->gather(halting, labels);
      case JMP:
        labels.insert(this);
      default:
        break;
    }
  }
};

DFA* compile(std::string p);
std::string match(const char* pat, const char* str);
std::string match(DFA* pat, std::string str);