#include "pda.h"

int main() {
  Terminal* Ident = new Terminal("[A-Za-z_][A-Za-z0-9_]*");
  Terminal* Num = new Terminal("[0-9]+");

  NonTerminal* Atom = new NonTerminal{{Ident, Num}};
  NonTerminal* Product = new NonTerminal{{Atom}};
  Product->add(new Rule({Product, new Terminal("\\*"), Atom}));
  Atom->add(new Rule({new Terminal("\\("), Product, new Terminal("\\)")}));

  std::string code = "2 * 2";
  printf("Here\n");
  printf("%d\n", Product->matches(code.c_str(), code.length()));
  printf("Success\n");
  // auto dfa = compile_dfa("[a-c]*");
  // printf("%d\n", match(dfa, code.c_str(), code.length()));
  // dfa->print_dot("dfa.dot");
  // compile_dfa(".")->print_dot("dfa.dot");
}
