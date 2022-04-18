#include <stdio.h>
#include "nfa.h"

void DFA::print_(std::set<DFA*>& visited) {
  if (visited.contains(this))
    return;
  visited.insert(this);
  const char* tag[] = {"@", "$"};

  printf("%s[%d]:\n", tag[halt], id);

  for (int i = 0; i < (1 << ALPHABET_WIDTH); ++i)
    if (edges[i])
      printf("\t%d -> [ %d ]\n", i, edges[i]->id);
    else
      printf("\t%d -> [ Trap ]\n", i);

  for (auto e : edges)
    if (e)
      e->print_(visited);
}

void DFA::print() {
  std::set<DFA*> v = {};
  print_(v);
}

void DFA::print_dot_(FILE* f, std::set<DFA*>& visited) {
  if (visited.contains(this))
    return;
  visited.insert(this);

  const char* col[] = {"green", "yellow"};
  fprintf(f, "\t%d [style=filled;fillcolor=%s];\n", id, col[!halt]);

  std::map<DFA*, std::set<int>> next;
  std::map<DFA*, std::vector<std::pair<int, int>>> dfa_ranges;

  for (int i = 0; i < (1 << ALPHABET_WIDTH); ++i)
    next[edges[i]].insert(i);

  for (auto& [dfa, edge] : next) {
    auto& ranges = dfa_ranges[dfa];
    for (auto& c : edge) {
      if (ranges.empty()) {
        ranges.push_back({c, c});
        continue;
      }
      if (ranges.back().second + 1 == c) {
        ranges.back().second++;
        continue;
      }
      ranges.push_back({c, c});
    }
  }

  for (auto [dfa, ranges] : dfa_ranges) {
    if (dfa)
      fprintf(f, "\t%d -> %d [label=\"", id, dfa->id);
    else
      fprintf(f, "\t%d -> Trap [label=\"", id);
    for (auto& [l, r] : ranges) {
      if (l == r)
        fprintf(f, " %d ", l);
      else
        fprintf(f, "[%d-%d]", l, r);
    }
    fprintf(f, "\"];\n");
  }

  for (auto e : edges)
    if (e)
      e->print_dot_(f, visited);
}

void DFA::print_dot(std::string file) {
  std::set<DFA*> visited;
  FILE* f;
  fopen_s(&f, file.c_str(), "w");
  fprintf(f, "digraph {\n");
  print_dot_(f, visited);
  fprintf(f, "}\n");
  fclose(f);
}