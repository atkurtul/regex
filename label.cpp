#include "nfa.h"

void Label::print_header(int d) {
  constexpr const char* tags[] = {"jmp", "br", "label", "halt"};
  printf("[%d]%s%s -> %d", id, std::string(d, '\t').c_str(), tags[tag],
         next->id);
  if (alt)
    printf(" | %d", alt->id);
  printf("\n");
}

void Label::print_(int d, std::set<Label*>& vis) {
  if (vis.contains(this))
    return;
  vis.insert(this);

  switch (tag) {
    case JMP:
      print_header(d);
      next->print_(d, vis);
      break;
    case ALT:
      print_header(d);
      next->print_(d + 1, vis);
      alt->print_(d + 1, vis);
      break;
    default:
      break;
  };
}

void Label::print() {
  std::set<Label*> vis;
  print_(0, vis);
  printf("[0]HALT\n");
}

bool Label::has_match() {
  switch (tag) {
    case HALT:
      return true;
    case ALT:
      return next->has_match() || alt->has_match();
    default:
      break;
  }
  return false;
}

bool Label::match(char p, std::vector<Label*>& states) {
  switch (tag) {
    case HALT:
      return true;
    case ALT:
      next->match(p, states);
      alt->match(p, states);
      break;
    case JMP:
      if (c[p])
        states.push_back(next);
  }
  return false;
}