#ifndef TINY_XML_H
#define TINY_XML_H

#include "tokenizer.h"

#ifndef OK
  #define OK(x) (x == 0)
#endif

int set_avg_map(int);
int parse_html(char* html, int size);

#endif