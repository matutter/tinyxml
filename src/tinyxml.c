#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "tokenizer.h"
#include "source_map.h"
#include "dom_tree.h"
#include "tinyxml.h"
#include "debug.h"

typedef struct PARSER_GUTS parser;
struct PARSER_GUTS {

  union {
    char* ptr;
    char* html;
  };
  char* end_ptr;

  struct {
    source_map_t* list;
    // current index
    int idx;
    // number of slots
    int limit;
    // size of current allocation
    int size;
  } sm;

  source_map_t* sm_prev;
};

int get_memory_guess(char*, char*, int*, int*);
char* get_highlight(int);
int increase_parser_memory(parser*, char*, char*);
int insert_source_map(parser*, source_map_t*);

static int reallocs = 0;

/**
* 
*/
char * get_highlight(int id) {

  switch(id) {
    case HTML_ANCHOR:         return KDIM KBLU;
    case HTML_TAG_NAME:       return KBOLD KCYN;
    case HTML_ATTR_NAME:      return KCYN;
    case HTML_ATTR_EQ:        return KBOLD KWHT;
    case HTML_ATTR_VALUE:     return KGRN;
    case HTML_CONTENT_TEXT:   return KDIM KYEL;
    case HTML_CONTENT_STYLE:  return KDIM KMAG;
    case HTML_CONTENT_SCRIPT: return KDIM KGRN;
    case HTML_COMMENT:        return KDIM KWHT;
    case 0:                   return KRST "\n"; // end
    default:
      debug_danger("unknown token id %d", id);
      break;
  }

  return "";
}

/**
* 
*/
int increase_parser_memory(parser* p, char* p1, char* p2) {

  int status = -1;
  int required_mem = 0;
  int item_count = 0;
  void* ptr = NULL;

  status = get_memory_guess(p1, p2, &item_count, &required_mem);
  if(OK(status)) {

    int total_mem = required_mem + p->sm.size;
    ptr = realloc(p->sm.list, total_mem);
    if(ptr) {
      reallocs++;
      #if 0
        debug_success("source map increased was: %d, increased: %d", p->sm.limit, item_count);
      #endif

      p->sm.limit += item_count;
      p->sm.size = total_mem;
      p->sm.list = (source_map_t*)ptr;
      memset(&p->sm.list[p->sm.idx], 0, required_mem);
      status = 0;
    } else {
      debug_danger("failed to reallocate source map vector");
      status = -1;
    }
  }

  return status;
}

/**
* 
*/
int insert_source_map(parser* p, source_map_t* sm) {
  int status = -1;

  if(p->sm.idx >= p->sm.limit) {
    status = increase_parser_memory(p, sm->ptr, p->end_ptr);
    if(!OK(status)) {
      debug_danger("failed to increase parser memory");
      return -1;
    }
  }

  source_map_t* new_sm = &p->sm.list[p->sm.idx++];
  memcpy(new_sm, sm, sizeof(source_map_t));

  status = 0;

  return status;
}

/**
* 
*/
int map_fn(source_map_t* sm, void* arg) {
  int status = -1;

  parser* p = (parser*)arg;

  status = insert_source_map(p, sm);

  return status;
}

/**
* Guess at how much memory is required to create `source_map_t` for the HTML 
* that is within the area of memory between `p1` and `p2`. The guess is made
* by counting the amount of closing tags used in the string.
*
* @param p1 - The start of the HTML string.
* @param p2 - The start of the HTML string.
* @param item_count - Approximately how many `source_map_t` are required.
* @param required_mem - How much memory is required to fit `item_count` elements.
*/
int get_memory_guess(char* p1, char* p2, int* item_count, int* required_mem) {

  if(p1 > p2) {
    debug_danger("p1: %p, p2: %p", p1, p2);
    errno = EINVAL;
    return -1;
  }

  // weirdly enough this constantly happens, often 1 short
  if(p1 == p2) {
    *item_count = 1;
    *required_mem = sizeof(source_map_t);
    return 0;
  }

  const int factor = 3;
  int count = factor;
  char* ptr = p1;
  while(ptr++ < p2) {
    if(*ptr == '>') count+=factor; 
  }

  *item_count = count;
  *required_mem = *item_count * sizeof(source_map_t);

  #if 0
    debug_info(
      "input: %d, count: %d, mem: %d"
      , (p2 - p1)
      , *item_count
      , *required_mem
    );
  #endif

  return 0;
}

/**
* 
*/
int parse_html(char* html, int html_size) {
  int status = -1;

  debug_info("parsing document %d", html_size);

  char* end = (html + html_size);
  int required_mem = 0;
  int item_count = 0;
  void* ptr = NULL;

  status = get_memory_guess(html, end, &item_count, &required_mem);
  if(!OK(status)) {
    debug_warning("Invalid initial memory calculation for source map.");
    return -1;
  }

  ptr = calloc(1, required_mem);
  parser p = {
    .html = html,
    .end_ptr = end,
    .sm = {
      .list = ptr,
      .idx = 0,
      .limit = item_count,
      .size = required_mem
    }
  };

  status = scan_html(html, html_size, map_fn, &p);
  if(OK(status)) {
    debug_success("scanner exited");

    #if 1

      for(int i = 0; i < p.sm.idx; i++) {
        source_map_t* sm = &p.sm.list[i];
        printf(
          //"(%d)"
          "%s%.*s%.*s" KRST
          //, sm->id
          , get_highlight(sm->id)
          , sm->ws_size, sm->ws
          , sm->text_size, sm->text
        );
      }

    #endif

    #if 0
      debug_info("\n"
        " - map used: %d\n"
        " - map limit: %d\n"
        KBOLD " > map free: %d\n"
        KRST KDIM "----------------------\n" KRST KCYN
        " - mem total: %d\n"
        " - mem used: %d\n"
        KBOLD " > mem free: %d\n"
        KRST KDIM "----------------------\n" KRST KCYN
        KBOLD " > reallocs: %d"
        , p.sm.idx
        , p.sm.limit
        , (p.sm.limit - p.sm.idx)
        , p.sm.size
        , (p.sm.idx * (int)sizeof(source_map_t))
        , (p.sm.size - (p.sm.idx * (int)sizeof(source_map_t)))
        , reallocs
      );
    #endif

  } else {
    debug_warning("scanner encountered an error");
  }

  return status;
}
