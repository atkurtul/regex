#include "nfa.h"

int main() {
  auto dfa = compile("xyzw(xyzw)*");
  // printf("%zu\n", nfa->edges.size());

  printf("------------\n");
  dfa = DFA::minimize(dfa);
  dfa->print_dot("dfa.dot");
  printf("%s\n", match(dfa, "111x222").c_str());
  printf("Succ\n");
}
