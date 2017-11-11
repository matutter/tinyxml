#ifndef HTML_SOURCE_MAP_H
#define HTML_SOURCE_MAP_H

enum HTML_MAP_ID {
  HTML_ANCHOR         = 10,
  HTML_TAG_NAME       = 20,
  HTML_ATTR_NAME      = 30,
  HTML_ATTR_EQ        = 40,
  HTML_ATTR_VALUE     = 50,
  HTML_CONTENT_SCRIPT = 60,
  HTML_CONTENT_STYLE  = 70,
  HTML_CONTENT_TEXT   = 80,
  HTML_COMMENT        = 90
};

/**
* `src_map` describes the smallest parts of a document
* such as the `<` anchor in a tag or the `   id` spaces and name of an
* `dom_node` `dom_attr`.
*/
typedef struct HTML_SOURCE_MAP source_map_t;
struct HTML_SOURCE_MAP {

  // the kind of syntactical mapping this describes
  enum HTML_MAP_ID id;

  // pointer to any leading whitespace before certain parts of HTML
  char* ws;
  int ws_size;

  // the start of content, not including leading whitespace
  char* text;
  int text_size;

};

typedef int (* source_map_fn)(source_map_t*, void*);

#endif 