#include "nfa.h"

Label* parse_post(const char*& p, Label*);
Label* parse_cat(const char*& p, Label*);
Label* parse_alt(const char*& p, Label*);

Label* parse_atom(const char*& p, Label* tail) {
  char c = *p++;
  std::bitset<256> bs(1);
  switch (c) {
    case '.':
      return ALLOC(Label{bs.set(), Label::JMP, tail});
    case ')': {
      Label* lbl = parse_alt(p, tail);
      assert(*p++ == '(');
      return lbl;
    }
    case ']':
      bs.reset();
      while ((c = *p++) != '[') {
        switch (c) {
          case '^':
            bs.flip();
            assert(p[0] == '[');
            break;
          case '-':
            for (char i = *p++; i <= p[-3]; i++)
              bs.set(i);
            break;
          case 0:
            impossible;
          default:
            bs.set(c);
        }
      }
      return ALLOC(Label{bs, Label::JMP, tail});

    default:
      return ALLOC(Label{bs << c, Label::JMP, tail});
  }
  impossible;
  return 0;
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

DFA* compile(std::string p) {
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
  return match(compile(pat), str);
}