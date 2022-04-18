#include "nfa.h"

Label* parse_post(const char*& p, Label*);
Label* parse_cat(const char*& p, Label*);
Label* parse_alt(const char*& p, Label*);

Label* parse_atom(const char*& p, Label* tail) {
  char c = *p++;
  std::bitset<(1 << ALPHABET_WIDTH)> bs[8 / ALPHABET_WIDTH] = {};

  switch (c) {
    case '.':
      for (auto& s : bs)
        s.set();
      break;
    case ')':
      tail = parse_alt(p, tail);
      assert(*p++ == '(');
      return tail;
    case ']':
      while ((c = *p++) != '[')
        switch (c) {
          case '^':
            for (auto& s : bs)
              s.flip();
            assert(p[0] == '[');
            break;
          case '-':
            for (c = *p++; c <= p[-3]; c++) {
              for (auto& s : bs) {
                s.set(c % (1 << ALPHABET_WIDTH));
                c /= (1 << ALPHABET_WIDTH);
              }
            }
            break;
          case 0:
            impossible;
          default:
            for (auto& s : bs) {
              s.set(c % (1 << ALPHABET_WIDTH));
              c /= (1 << ALPHABET_WIDTH);
            }
        }
      break;
    default:
      for (auto& s : bs) {
        s.set(c % (1 << ALPHABET_WIDTH));
        c /= (1 << ALPHABET_WIDTH);
      }
  }

  for (auto s : bs)
    tail = ALLOC(Label{s, Label::JMP, tail});

  return tail;
}

Label* parse_post(const char*& p, Label* tail) {
  Label* post = tail;
  switch (*p) {
    case '*':
      p++;
      post = ALLOC(Label{
          .tag = Label::ALT,
          .alt = tail,
      });
      post->next = parse_atom(p, post);
      break;
    case '?':
      p++;
      post = ALLOC(Label{
          .tag = Label::ALT,
          .next = parse_atom(p, tail),
          .alt = tail,
      });
      break;
    default:
      return parse_atom(p, tail);
  }
  return post;
}

Label* parse_cat(const char*& p, Label* tail) {
  Label* cat = parse_post(p, tail);
  for (;;) {
    switch (*p) {
      case '|':
      case '(':
      case 0:
        return cat;
    }
    cat = parse_post(p, cat);
  }
  return cat;
}

Label* parse_alt(const char*& p, Label* tail) {
  Label* alt = parse_cat(p, tail);
  while (*p && *p == '|') {
    ++p;
    alt = ALLOC(Label{
        .tag = Label::ALT,
        .next = parse_cat(p, tail),
        .alt = alt,
    });
  }
  return alt;
}

Label* compile(std::string p) {
  std::reverse(p.begin(), p.end());
  const char* s = &p.front();
  Label* tail = ALLOC(Label{.tag = Label::HALT});
  return parse_alt(s, tail);
}

DFA* compile_dfa(std::string p) {
  std::reverse(p.begin(), p.end());
  const char* s = &p.front();
  Label* tail = ALLOC(Label{.tag = Label::HALT});
  Label* l = parse_alt(s, tail);
  return l->to_nfa()->to_dfa();
}

std::string match(Label* pat, std::string str) {
  std::vector<Label*> front = {pat};
  std::vector<Label*> back = {};

  int last = 0;
  int cur = 0;
  for (; cur < str.length(); ++cur) {
    bool match = false;
    for (auto state : front)
      match |= state->match(str[cur], back);

    if (match)
      last = cur;

    if (back.empty())
      return str.substr(0, last);

    front.clear();
    front.swap(back);
  }

  for (auto state : front) {
    if (state->has_match()) {
      return str.substr(0, cur);
    }
  }

  return str.substr(0, last);
}

std::string match(DFA* pat, std::string str) {
  DFA* state = pat;
  u32 cur = 0;
  u32 last = 0;
  const u32 len = str.length();
  while (state && cur < len) {
    if (state->halting)
      last = cur;
    state = state->edges[str[cur++]];
  }

  if (state && state->halting)
    last = cur;

  return str.substr(0, last);
}

std::string match(const char* pat, const char* str) {
  return match(compile_dfa(pat), str);
}