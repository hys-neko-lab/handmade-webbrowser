#include "parser.h"

/* Find tag from string (HTML) */
int find_tag(char *string, int offset)
{
  int i = 0;
  int endtag;
  char c;
  char buf[PARSER_BUF_SIZE];
  int bufc = 0;

  static node_t n[NODE_NUM];
  static int nc = 0;
  static int ftag = 0; // if 1, find_tag() in tag.
  static int fcond = 0; // if 1, find_tag() in condition.
  static node_t* now = &dom;

  // Store tag string into this buffer.
  memset(buf, 0, sizeof(buf));
  c = string[i];

  /* Scan every char */
  for(i = offset; c != '\0'; i++) {
    c = string[i];
    if(c == '<') {
      /* Find tag start '<', but buffer has string.
       * So this string is text.
       */
      if (strcmp(buf, "") != 0){
        if (buf[bufc - 1]==' ') buf[bufc - 1] = '\0';
        printf("TEXT:%s\n", buf);
        now = sort_tag(buf, now, &n[nc], fcond);
        nc++;
        memset(buf, 0, 255);
        bufc = 0;
      }
      // Start seeking '>'.
      printf("[find tag]");
      ftag = 1;
      endtag = find_tag(string, i + 1);
      i = endtag;
    }
    else if (c == '>') {
      // Find '>', so string in buffer is tag.
      printf("%s\n", buf);
      now = sort_tag(buf, now, &n[nc], fcond);
      nc++;
      memset(buf, 0, 255);
      bufc = 0;
      ftag = 0;
      fcond = 0;
      return i;
    }
    else if (c == '\n') {
      // Discard
    }
    else if (c == ' ' || c == '\t') {
      if (strcmp(buf, "") == 0) {
        // Discard
      }
      else if (buf[bufc-1] == ' ') {
        // Discard
      }
      else if(ftag == 1) {
        // Space in tag, next is condition
        printf("%s\n", buf);
        now = sort_tag(buf, now, &n[nc], fcond);
        nc++;
        memset(buf, 0, 255);
        fcond = 1;
        bufc = 0;
      }
      else {
        buf[bufc] = ' ';
        bufc += 1;
      }
    } 
    else {
      buf[bufc] = c;
      bufc += 1;
    }
  }

  return 0;
}

/* Sort found tags */
node_t* sort_tag(char* buf, node_t* node, node_t* n, int fcond)
{
  if (strcmp(buf, "html") == 0){
    add_node(HTML, NULL, node, n);
    return n;
  }
  else if (strcmp(buf, "head") == 0){
    add_node(HEAD, NULL, node, n);
    return n;
  }
  else if (strcmp(buf, "title") == 0){
    add_node(TITLE, NULL, node, n);
    return n;
  }
  else if (strcmp(buf, "body") == 0){
    add_node(BODY, NULL, node, n);
    return n;
  }
  else if (strcmp(buf, "a") == 0){
    add_node(ANCH, NULL, node, n);
    return n;
  }
  else if (strcmp(buf, "br") == 0){
    add_node(BR, NULL, node, n);
    return node;
  }
  else if (strcmp(buf, "/html") == 0){
    return node->parent;
  }
  else if (strcmp(buf, "/head") == 0){
    return node->parent;
  }
  else if (strcmp(buf, "/title") == 0){
    return node->parent;
  }
  else if (strcmp(buf, "/body") == 0){
    return node->parent;
  }
  else if (strcmp(buf, "/a") == 0){
    return node->parent;
  }
  else if (fcond == 1){
    char cond[32] = "";
    char value[32] = "";
    divide_cond(buf, cond, value);
    add_node(COND, cond, node, n);
    add_node(TEXT, value, n, n + sizeof(node_t));
    return n->parent;
  }
  else {
    add_node(TEXT, buf, node, n);
    return n->parent;
  }
}

/* Init tree by adding root node */
int init_tree(node_t* node)
{
  add_node(ROOT, NULL, NULL, node);
  return 0;
}

/* Add node to DOM tree */
int add_node(int tag, const char* content, node_t* parent, node_t* node)
{
  node->tag = tag;
  if (content != NULL) strcpy(node->content, content);
  else strcpy(node->content, "");
  node->parent = parent;
  node->child = NULL;
  node->next = NULL;
  if (parent == NULL)
    return 0;
  if (parent->child == NULL)
    parent->child = node;
  else {
    node_t * last = parent->child;
    while (last->next != NULL)
      last = last->next;
    last->next = node;
  }

  return 0;
}

/* Show DOM tree for debug */
void show_tree(node_t* node)
{
  static int depth = 0;
  if(node == NULL) return;

  /* Print tag */
  printf("TAG:%d", node->tag);
  if(strcmp(node->content, "") != 0)
    printf(" %s", node->content);
  printf("\n");

  /* Goto child */
  if(node->child != NULL){
    depth += 1;
    for(int i=0;i<depth;i++) printf("-");
    show_tree(node->child);
    depth -= 1;
  }

  /* Goto next */
  if(node->next != NULL){
    for(int i=0;i<depth;i++) printf("-");
    show_tree(node->next);
  }
}

/* Solve node from DOM tree from tag */
char* solve_node(node_t* node, int tag)
{
  char* cont = NULL;
  if(node == NULL) return NULL;

  /* Search tag */
  if(node->tag == tag)
    return node->child->content;
  else if(cont == NULL){
    /* Goto child */
    if(node->child != NULL)
      cont = solve_node(node->child, tag);
    /* Goto next */
    if(cont == NULL && node->next != NULL)
      cont = solve_node(node->next, tag);
  }
  return cont;
}

/* Return nodes */
node_t* solve_body(node_t* node, int tag)
{
  node_t* ret = NULL;
  if(node == NULL) return NULL;

  /* Search tag */
  if(node->tag == tag)
    return node;
  else if(ret == NULL){
    /* Goto child */
    if(node->child != NULL)
      ret = solve_body(node->child, tag);
    /* Goto next */
    if(ret == NULL && node->next != NULL)
      ret = solve_body(node->next, tag);
  }
  return ret;
}

/* Divide string by '=' */
void divide_cond(const char* content, char* cond, char* value)
{
  int i;
  int size = strlen(content);
  for(i = 0; i < size; i++) {
    if(content[i] == '=') {
      strncpy(cond, content, i);
      if(content[i + 1] == '\"' && content[size-1] == '\"')
        strncpy(value, &(content[i + 2]), size - (i + 3));
      else
        strncpy(value, &(content[i + 1]), size - (i + 1));
      break;
    } 
  }
}