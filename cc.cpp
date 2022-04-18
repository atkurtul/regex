#include "nfa.h"

Label* parse_post(const char*& p, Label*);
Label* parse_cat(const char*& p, Label*);
Label* parse_alt(const char*& p, Label*);

Label* parse_atom(const char*& p, Label* tail) {
  char c = *p++, x;

  std::bitset<(1 << ALPHABET_WIDTH)> bs[8 / ALPHABET_WIDTH] = {};
  if (*p == '\\') {
    ++p;
    goto DEFAULT;
  }
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
            for (char q = *p++; q <= p[-3]; q++) {
              x = q;
              for (auto& s : bs) {
                s.set(x % (1 << ALPHABET_WIDTH));
                x /= (1 << ALPHABET_WIDTH);
              }
            }
            break;
          case 0:
            impossible;
          default:
            x = c;
            for (auto& s : bs) {
              s.set(x % (1 << ALPHABET_WIDTH));
              x /= (1 << ALPHABET_WIDTH);
            }
        }

      break;
    case 0:
      impossible;
    default:
    DEFAULT:
      x = c;
      for (auto& s : bs) {
        s.set(x % (1 << ALPHABET_WIDTH));
        x /= (1 << ALPHABET_WIDTH);
      }
  }

  for (auto s : bs)
    tail = ALLOC(Label{s, Label::JMP, tail});

  return tail;
}

Label* parse_post(const char*& p, Label* tail) {
  if (p[1] == '\\')
    return parse_atom(p, tail);
  int n = 0;
  Label* tmp;
  switch (*p) {
    case '*':
      p++;
      tail = ALLOC(Label{
          .tag = Label::ALT,
          .alt = tail,
      });
      tail->next = parse_atom(p, tail);
      break;
    case '+':
      p++;
      tail = ALLOC(Label{
          .tag = Label::ALT,
          .alt = tail,
      });
      tail = tail->next = parse_atom(p, tail);
      break;
    case '?':
      p++;
      tail = ALLOC(Label{
          .tag = Label::ALT,
          .next = parse_atom(p, tail),
          .alt = tail,
      });
      break;
    case '}':
      while ((*++p) != '{') {
        assert(isdigit(*p));
        n *= 10;
        n += *p - '0';
      }
      ++p;
      assert(n > 0);
      const char* head;
      while (n--) {
        head = p;
        tail = parse_atom(head, tail);
      }
      p = head;
      break;
    default:
      return parse_atom(p, tail);
  }
  return tail;
}

Label* parse_cat(const char*& p, Label* tail) {
  Label* cat = parse_post(p, tail);
  for (;;) {
    switch (*p) {
      case '|':
      case '(':
        if (p[1] != '\\')
          return cat;
        break;
      case 0:
        return cat;
    }
    cat = parse_post(p, cat);
  }
  return cat;
}

Label* parse_alt(const char*& p, Label* tail) {
  Label* alt = parse_cat(p, tail);
  while (*p && (*p == '|' && p[1] != '\\')) {
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

int match(Label* pat, const char* str, const u32 len) {
  std::vector<Label*> front = {pat};
  std::vector<Label*> back = {};

  int last = -1;
  int cur = 0;

  for (; cur < len; ++cur) {
    bool match = false;
    for (auto state : front)
      match |= state->match(str[cur], back);

    if (match)
      last = cur;

    if (back.empty())
      return last;

    front.clear();
    front.swap(back);
  }

  for (auto state : front)
    if (state->has_match())
      return cur;

  return last;
}

int match(DFA* state, const char* str, const u32 len) {
  int last = -1;
  int cur = 0;

  while (!state->is_trap() && cur < len) {
    if (state->halt)
      last = cur;
    state = state->edges[str[cur++]];
  }

  if (state->halt)
    last = cur;

  return last;
}

int match(const char* pat, const char* str, const u32 len) {
  return match(compile_dfa(pat), str, len);
}