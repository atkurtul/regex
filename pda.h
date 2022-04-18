#pragma once

#include <ctype.h>
#include "nfa.h"

struct Symbol {
  virtual int match(const char*, u32) = 0;

  int matches(const char* str, u32 slen) {
    if (last == str)
      return 0;

    u32 skipped = 0;
    while (skipped < slen && isspace(str[skipped]))
      skipped++;

    last = str + skipped;
    int re = match(str + skipped, slen - skipped);
    while (skipped < slen && isspace(str[skipped]))
      skipped++;

    if (re != -1)
      re += skipped;

    return re;
  }

  const char* last = 0;
};

struct Terminal : Symbol {
  DFA* dfa;
  int match(const char* str, u32 slen) override {
    return ::match(dfa, str, slen);
  }
  Terminal(const char* pat)
      : dfa(DFA::minimize(compile(pat)->to_nfa()->to_dfa())) {}
};

struct Rule : Symbol {
  std::vector<Symbol*> symbols;
  Rule(std::vector<Symbol*> symbols) : symbols(std::move(symbols)) {}

  void add(Symbol* rule) { symbols.push_back(rule); }
  int match(const char* str, u32 slen) override {
    printf("Matching rules against %.*s\n", slen, str);
    int len = 0;
    for (auto s : symbols) {
      int mlen = s->matches(str + len, slen - len);
      if (mlen == -1)
        return -1;
      len += mlen;
    }
    return len;
  }
};

struct NonTerminal : Symbol {
  std::vector<Symbol*> symbols;
  NonTerminal(std::vector<Symbol*> symbols) : symbols(std::move(symbols)) {}
  void add(Symbol* rule) { symbols.push_back(rule); }
  int match(const char* str, u32 len) override {
    std::set<int> matches;
    for (auto s : symbols)
      matches.insert(s->matches(str, len));
    return *--matches.end();
  }
};

struct PDA {};