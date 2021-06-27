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
				printf("TEXT:%s\n", buf);
				now = sort_tag(buf, now, &n[nc]);
				nc++;
				memset(buf, 0, 255);
				bufc = 0;
			}
			// Start seeking '>'.
			printf("[find tag]");
			endtag = find_tag(string, i + 1);
			i = endtag;
		}
		else if (c == '>') {
			// Find '>', so string in buffer is tag.
			printf("%s\n", buf);
			now = sort_tag(buf, now, &n[nc]);
			nc++;
			memset(buf, 0, 255);
			bufc = 0;
			return i;
		}
		else {
			buf[bufc] = c;
			bufc += 1;
		}
	}

	return 0;
}

/* Sort found tags */
node_t* sort_tag(char* buf, node_t* node, node_t* n)
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
