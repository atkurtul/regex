
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

#define ALPHABET_WIDTH 1

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
  DFA* edges[(1<<ALPHABET_WIDTH)];
  bool halting;
  inline static int next_id = 0;
  int id = next_id++;
  void print_(std::set<DFA*>& visited);
  void print();
  bool is_trap();
  void print_dot_(FILE* f, std::set<DFA*>& visited);
  void print_dot(std::string file);

  bool operator==(DFA const& r) const {
    if (halting != r.halting)
      return false;
    for (int i = 0; i < (1<<ALPHABET_WIDTH); i++)
      if (edges[i] != r.edges[i])
        return false;
    return true;
  }

  DFA* find_remapping(std::set<DFA*> const& pool, std::map<DFA*, DFA*>& remap) {
    if (auto it = remap.find(this); it != remap.end())
      return it->second;

    DFA* mapping = this;
    std::set<DFA*> remapped_to_this;

    for (auto& dfa : pool) {
      if (this != dfa && *this == *dfa) {
        if (auto it = remap.find(dfa); it != remap.end()) {
          mapping = remap[this] = it->second;
          for (auto dfa : remapped_to_this)
            remap[dfa] = mapping;
          break;
        } else {
          remap[dfa] = this;
          remapped_to_this.insert(dfa);
        }
      }
    }
    return mapping;
  }

  void remap(std::map<DFA*, DFA*>& remap, std::set<u32>& free_ids) {
    if (!free_ids.empty()) {
      const u32 last = *--free_ids.end();
      if (last < id) {
        id = last;
        free_ids.erase(last);
        free_ids.insert(id);
      }
    }

    for (auto& e : edges)
      if (auto it = remap.find(e); it != remap.end())
        e = it->second;
  }

  static DFA* minimize(DFA* root) {
    std::set<DFA*> pool;
    std::map<DFA*, DFA*> remap;
    root->gather(pool);

    std::set<u32> free_ids;

    do {
      remap.clear();
      for (auto dfa : pool)
        dfa->find_remapping(pool, remap);
      if (auto it = remap.find(root); it != remap.end())
        root = it->second;
      for (auto [dfa, _] : remap) {
        free_ids.insert(dfa->id);
        pool.erase(dfa);
        delete dfa;
      }
      for (auto dfa : pool)
        dfa->remap(remap, free_ids);
    } while (!remap.empty());

    return root;
  }

  void gather(std::set<DFA*>& pool) {
    if (pool.contains(this))
      return;
    pool.insert(this);
    for (auto e : edges)
      e->gather(pool);
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
    if (auto dfa = cache.find(nfa); dfa != cache.end())
      return dfa->second;

    DFA* dfa = cache[nfa] = ALLOC(DFA{});

    for (int i = 0; i < (1<<ALPHABET_WIDTH); ++i) {
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
    DFA* trap = ALLOC(DFA{});
    for (int i = 0; i < (1<<ALPHABET_WIDTH); ++i)
      trap->edges[i] = trap;
    std::map<std::set<NFA*>, DFA*> cache;
    cache[{}] = trap;
    return to_dfa_(std::set<NFA*>{this}, cache);
  }
};

struct Label {
  std::bitset<(1<<ALPHABET_WIDTH)> c;
  enum { JMP, ALT, HALT } tag;
  Label* next;
  Label* alt;

  inline static int next_id = 0;
  int id = next_id++;

  bool has_match();
  bool match(char p, std::vector<Label*>& states);
  void print_header(int d);
  void print_(int d, std::set<Label*>&);
  void print();

  NFA* to_nfa_(std::map<Label*, NFA*>& cache) {
    if (auto it = cache.find(this); it != cache.end())
      return it->second;

    NFA* nfa = cache[this] = ALLOC(NFA{});
    std::set<Label*> self;
    gather(nfa->halting, self);

    for (auto e : self)
      for (int i = 0; i < (1<<ALPHABET_WIDTH); ++i)
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

Label* compile(std::string p);
DFA* compile_dfa(std::string p);
std::string match(const char* pat, const char* str);
std::string match(DFA* pat, std::string str);