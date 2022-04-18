#include "nfa.h"

int main() {
  auto dfa = compile_dfa("a");
  // printf("%zu\n", nfa->edges.size());

  printf("------------\n");
  dfa->print_dot("dfa.dot");
  // printf("%s\n", match(dfa, "111x222").c_str());
  printf("Succ\n");
}
