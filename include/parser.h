#ifndef _PARSER_H_
#define _PARSER_H_

#include <stdio.h>
#include <string.h>

#define PARSER_BUF_SIZE 255
#define NODE_NUM 100
#define ROOT	0
#define HTML	1
#define HEAD	2
#define TITLE	3
#define BODY	4
#define TEXT	5

typedef struct node {
	int tag;
	char content[256];
	struct node* parent;
	struct node* child;
	struct node* next;
} node_t;

extern node_t dom;

int find_tag(char *string, int offset);
node_t* sort_tag(char* buf, node_t* node, node_t* n);
int init_tree(node_t* node);
int add_node(int tag, const char* content, node_t* parent, node_t* node);
void show_tree(node_t* node);
char* solve_node(node_t* node, int tag);

#endif
