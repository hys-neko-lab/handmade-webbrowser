#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <linux/omapfb.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include "client.h"
#include "parser.h"

#define HTML_BUFSIZE 1024
#define HTML_HEADERSIZE 256

#define FB_PATH "/dev/fb0"
#define FONT_PATH "/usr/share/fonts/truetype/ubuntu/Ubuntu-B.ttf"
#define FONT_PT 12
#define FONT_PTSCALE 64

#define WIDTH   2048
#define HEIGHT  2048
#define TITLE_MAXLEN 10
#define TABCOLOR      0xFFDDDDDD
#define TABEMPCOLOR   0xFFAAAAAA
#define URLBACKCOLOR  0xFFDDDDDD
#define URLBOXCOLOR   0xFFFFFFFF
#define FONTCOLOR     0xFF000000
#define LINKCOLOR     0xFF0000FF
#define BGCOLOR       0xFFFFFFFF

unsigned int image[HEIGHT][WIDTH];
unsigned int bgimage[HEIGHT][WIDTH];
node_t dom;

char links[10][128];
int link_num = 0;
unsigned int  fcolor;
unsigned int  bgcolor;

/* Divide HTML */
int get_source(char *source)
{
	FILE *fp;
	char header[HTML_HEADERSIZE];
  char str[256] = "";
	int count = 0;
	if((fp = fopen("cache", "r")) == NULL) {
		perror("fopen");
		return 1;
	}

	while(fgets(header, HTML_HEADERSIZE, fp) != NULL && 7 > ++count);
	while(fgets(str, HTML_BUFSIZE, fp) != NULL){
    strncat(source, str, 256);
  }
	printf("SOURCE:\n%s\n", source);

	fclose(fp);
}

/* Draw bitmap of text */
void draw_bitmap( FT_Bitmap* bitmap, FT_Int x, FT_Int y, unsigned int fcolor)
{
  FT_Int  i, j, p, q;
  FT_Int  x_max = x + bitmap->width;
  FT_Int  y_max = y + bitmap->rows;
  unsigned int d;

  for ( i = x, p = 0; i < x_max; i++, p++ ){
    for ( j = y, q = 0; j < y_max; j++, q++ ){
      if ( i < 0      || j < 0       ||
           i >= WIDTH || j >= HEIGHT )
        continue;
      if (bitmap->buffer[q * bitmap->width + p] != 0){
        d = bitmap->buffer[q * bitmap->width + p];
        image[j][i] |= (fcolor & 0xFF000000) * d / 255;
        image[j][i] |= (fcolor & 0x00FF0000) * d / 255;
        image[j][i] |= (fcolor & 0x0000FF00) * d / 255;
        image[j][i] |= (fcolor & 0x000000FF) * d / 255;
      }
    }
  }
}

void draw_text(char* text, FT_Vector *pen, int target_height,
  FT_Face face, FT_Matrix matrix, FT_GlyphSlot slot, unsigned int fcolor)
 {
   int n;
   int num_chars = strlen(text);
  for (n = 0; n < num_chars; n++)
  {
    // set transformation and 
    // load glyph image into the slot (erase previous one)
    FT_Set_Transform(face, &matrix, pen);
    FT_Load_Char(face, text[n], FT_LOAD_RENDER);

    /* now, draw to our target surface (convert position) */
    draw_bitmap(&slot->bitmap,
                 slot->bitmap_left,
                 target_height - slot->bitmap_top, fcolor);

    /* increment pen position */
    pen->x += slot->advance.x;
    pen->y += slot->advance.y;
  }
 }

/* Draw box on background */
void draw_box(int x, int y, int width,
  int height, unsigned int color)
{
  for(int iy = y; iy < height; iy++){
    for(int ix = x; ix < width; ix++){
      bgimage[iy][ix] = color;
    }
  }
}

void draw_body(node_t* node, FT_Vector pen, int target_height,
  FT_Face face, FT_Matrix matrix, FT_GlyphSlot slot, unsigned int fcolor)
{
  char* text;
  char bgc[10] = "";
  char fc[10] = "";

  if (node == NULL)
    return;

  text = node->content;
  if (node->tag == COND) {
    if (strcmp(node->content, "bgcolor") == 0) {
      strncpy(bgc, &node->child->content[1], 6);
      sscanf(bgc, "%x", &bgcolor);
    }
    else if (strcmp(node->content, "font") == 0) {
      strncpy(fc, &node->child->content[1], 6);
      sscanf(fc, "%x", &fcolor);
    }
  }
  else if (node->tag == ANCH) {
    /* Draw text */
    text = node->child->next->content;
    draw_text(text, &pen, target_height, face, matrix, slot, LINKCOLOR);
    /* And store link */
    strcpy(links[link_num], node->child->child->content);
    link_num++;
  }
  else if (node->tag == BR) {
    pen.x = 10 * FONT_PTSCALE;
    pen.y -= (FONT_PT+3) * FONT_PTSCALE;
  }
  else if (node->tag == TEXT) {
    draw_text(text, &pen, target_height, face, matrix, slot, fcolor);
  }

  draw_body(node->next, pen, target_height, face, matrix, slot, fcolor);
}

/* Show background and text */
void show_image(void)
{
	int fd;
	fd = open(FB_PATH, O_RDWR);
	uint32_t *frameBuffer = 
    (uint32_t *)mmap(NULL,WIDTH * HEIGHT * 4, 
      PROT_WRITE, MAP_SHARED, fd, 0);

	for(int iy = 0; iy < HEIGHT; iy++){
		for(int ix = 0; ix < WIDTH; ix++){
			frameBuffer[ix + iy * WIDTH] = 
        image[iy][ix] == 0 ? bgimage[iy][ix]
          : image[iy][ix];
		}
	}

	msync(frameBuffer, WIDTH * HEIGHT * 4, 0);
	munmap(frameBuffer, WIDTH * HEIGHT * 4);
	close(fd);
}

int main(void)
{
  FT_Library    library;
  FT_Face       face;
  FT_GlyphSlot  slot;
  FT_Matrix     matrix;
  FT_Vector     pen;
  char*         dest;
  char*         font;
  char*         text;
  double        angle;
  int           target_height, n, num_chars;
  char source[HTML_BUFSIZE] = "";
  char path[128];
  char url[256];
  int port;
  int cmd;

  dest = "localhost";
  strcpy(path, "index.html");
  port = 8000;

  font = FONT_PATH;
  node_t *n_body;

  /* Initialize FreeType library */
  FT_Init_FreeType(&library);
  FT_New_Face(library, font, 0, &face);
  FT_Set_Char_Size(face, FONT_PT * FONT_PTSCALE, 0, 100, 0);
  slot = face->glyph;

  draw_box(0, 0, 100, 25, TABCOLOR);        // title tab
  draw_box(100, 0, 2048, 25, TABEMPCOLOR);  // empty tab space
  draw_box(0, 25, 2048, 50, URLBACKCOLOR);  // URL bar(back)
  draw_box(100, 30, 700, 48, URLBOXCOLOR);  // URL bar(textbox)

  while(cmd != 'q'){
    /* Init command */
    cmd = 0;

    /* Init color settings */
    fcolor = FONTCOLOR;
    bgcolor = BGCOLOR;

    /* Get HTTP & make DOM tree */
    get_http(dest, port, path);
    get_source(source);
    init_tree(&dom);
    find_tag(source, 0);
    show_tree(&dom);

    /* set up matrix */
    angle = 0;
    matrix.xx = (FT_Fixed)(cos(angle) * 0x10000L);
    matrix.xy = (FT_Fixed)(-sin(angle) * 0x10000L);
    matrix.yx = (FT_Fixed)(sin(angle) * 0x10000L);
    matrix.yy = (FT_Fixed)(cos(angle) * 0x10000L);

    target_height = HEIGHT;

    /* Title */
    text = solve_node(&dom, TITLE);
    if ( TITLE_MAXLEN < strlen(text))
      text[TITLE_MAXLEN - 1] = '\0';
    pen.x = 10 * FONT_PTSCALE;
    pen.y = (target_height - 20) * FONT_PTSCALE;
    draw_text(text, &pen, target_height, face, matrix, slot, FONTCOLOR);

    /* URL */
    pen.x = 100 * FONT_PTSCALE;
    pen.y = (target_height - 45) * FONT_PTSCALE;
    sprintf(url, "%s:%d/%s", dest, port, path);
    draw_text(url, &pen, target_height, face, matrix, slot, FONTCOLOR);

    /* Body */
    pen.x = 10 * FONT_PTSCALE;
    pen.y = (target_height - 80) * FONT_PTSCALE;
    n_body = solve_body(&dom, BODY);
    draw_body(n_body->child, pen, target_height, face, matrix, slot, fcolor);
    draw_box(0, 50, 2048, 2048, bgcolor); // body background

    /* Show image */
    show_image();

    /* Command */
    while (cmd != 'q') {
      cmd = getchar();
      if(cmd >= '0' && cmd <= '9') {
        strcpy(path, links[cmd - '0']);
        break;
      }
      show_image();
    }

    /* Clear for next page */
    memset(source, 0, HTML_BUFSIZE);
    memset(image, 0, HEIGHT*WIDTH);
    link_num = 0;
    for(int i = 0; i < 10; i++)
      memset(links[i], 0, sizeof(links[i]));
  }

  FT_Done_Face    (face);
  FT_Done_FreeType(library);
  return 0;
}
