#include "nfa.h"

int main() {
  auto dfa = compile("([A-Za-z0-9]*abc*|a)(a*|ab*a*)");
  // printf("%zu\n", nfa->edges.size());

  printf("------------\n");
  // dfa->print();
  printf("%s\n", match(dfa, "xabcaaaaaaaaaaaaaaaaaaa").c_str());
  printf("Succ\n");
}
