/*--------------------------------------------------------------------------
IMP.H
Copyright 瑞丽光电科技有限公司
http://www.rl-display.com
TEL:0755-28169430
--------------------------------------------------------------------------*/

#ifndef __IMP_H__
#define __IMP_H__

typedef unsigned char U8;
typedef unsigned int U16;
typedef unsigned long U32; 

void send(U8 d);
extern void display_ID();
extern void reset();
extern void modify_ID(U8 oid,U8 nid);
extern void clear_display(U8 ID);
extern void display_on(U8 ID);
extern void display_off(U8 ID);
extern void baud(U8 ID,U32 n);

extern void foreground_color(U8 ID,U8 r,U8 g,U8 b);
extern void background_color(U8 ID,U8 r,U8 g,U8 b);
extern void background_transparen(U8 ID);
extern void background_nor_transparen(U8 ID);
extern void cusor_xy(U8 ID,U16 x,U16 y);
extern void simplified_chinese(U8 ID);
extern void traditional_chinese(U8 ID);
extern void japanese(U8 ID);
extern void korean(U8 ID);
extern void font_size_16(U8 ID);
extern void font_size_24(U8 ID);
extern void font_size_32(U8 ID);
extern void line_spacing(U8 ID,U8 n);
extern void row_spacing(U8 ID,U8 n);
extern void print_string(U8 ID,U8 *ptr);
extern void print_string_xy(U8 ID,U16 x,U16 y,U8 *ptr);
extern void print_U32(U8 ID,U32 n);
extern void print_U32_xy(U8 ID,U16 x,U16 y,U32 n);
extern void print_key_value(char key[],int value);

extern void draw_pixel(U8 ID,U16 x,U16 y,U8 r,U8 g,U8 b);
extern void draw_line(U8 ID,U16 x1,U16 y1,U16 x2,U16 y2,U8 r,U8 g,U8 b);
extern void draw_rectangle(U8 ID,U16 x,U16 y,U16 w,U16 h,U8 r,U8 g,U8 b);
extern void draw_rectangle_fill(U8 ID,U16 x,U16 y,U16 w,U16 h,U8 r,U8 g,U8 b);
extern void draw_circle_square(U8 ID,U16 x,U16 y,U16 w,U16 h,U16 ra,U8 r,U8 g,U8 b);
extern void draw_circle_square_fill(U8 ID,U16 x,U16 y,U16 w,U16 h,U16 ra,U8 r,U8 g,U8 b);
extern void draw_circle(U8 ID,U16 x,U16 y,U16 ra,U8 r,U8 g,U8 b);
extern void draw_circle_fill(U8 ID,U16 x,U16 y,U16 ra,U8 r,U8 g,U8 b);
extern void draw_ellipse(U8 ID,U16 x,U16 y,U16 xra,U16 yra,U8 r,U8 g,U8 b);
extern void draw_ellipse_fill(U8 ID,U16 x,U16 y,U16 xra,U16 yra,U8 r,U8 g,U8 b);

extern void picture_xy(U8 ID,U8 *filename,U16 x,U16 y);
extern void pitrure_spread(U8 ID,U8 *filename,U16 x,U16 y,U8 s);
extern void picture_right_to_left(U8 ID,U8 *filename,U16 x,U16 y,U8 s);
extern void picture_left_to_right(U8 ID,U8 *filename,U16 x,U16 y,U8 s);
extern void picture_up_to_down(U8 ID,U8 *filename,U16 x,U16 y,U8 s);
extern void picture_down_to_up(U8 ID,U8 *filename,U16 x,U16 y,U8 s);
extern void picture_horizontally(U8 ID,U8 *filename,U16 x,U16 y,U8 s);
extern void picture_vertical(U8 ID,U8 *filename,U16 x,U16 y,U8 s);
extern void picture_clip(U8 ID,U8 *filename,U16 x,U16 y,U16 lx,U16 ly,U16 w,U16 h);
extern void picture_zom(U8 ID,U8 *filename,U16 x,U16 y,U16 w,U16 h);
extern void picture_best_show(U8 ID,U8 *filename,U16 x,U16 y,U16 w,U16 h);

extern void load_buffer(U8 ID,U8 *filename,U8 n);
extern void unload_buffer(U8 ID,U8 n);
extern void buffer_xy(U8 ID,U8 n,U16 x,U16 y);
extern void buffer_spread(U8 ID,U8 n,U16 x,U16 y,U8 s);
extern void buffer_right_to_left(U8 ID,U8 n,U16 x,U16 y,U8 s);
extern void buffer_left_to_right(U8 ID,U8 n,U16 x,U16 y,U8 s);
extern void buffer_up_to_down(U8 ID,U8 n,U16 x,U16 y,U8 s);
extern void buffer_down_to_up(U8 ID,U8 n,U16 x,U16 y,U8 s);
extern void buffer_horizontally(U8 ID,U8 n,U16 x,U16 y,U8 s);
extern void buffer_vertical(U8 ID,U8 n,U16 x,U16 y,U8 s);
extern void buffer_clip(U8 ID,U8 n,U16 x,U16 y,U16 lx,U16 ly,U16 w,U16 h);
extern void buffer_zom(U8 ID,U8 n,U16 x,U16 y,U16 w,U16 h);
extern void buffer_best_show(U8 ID,U8 n,U16 x,U16 y,U16 w,U16 h);
extern void buffer_autodisplay(U8 ID,U8 n1,U8 n2,U16 x,U16 y);
extern void buffer_autodisplay_speed(U8 ID,U8 n);
extern void buffer_autodisplay_stop(U8 ID);

extern void windows(U8 ID, U16 x,U16 y,U16 w,U16 h,U8 *name);
extern void windows_click(U8 ID, U16 x,U16 y,U16 w,U16 h);
extern void button(U8 ID, U16 x,U16 y,U16 w,U16 h);
extern void button_click(U8 ID, U16 x,U16 y,U16 w,U16 h);
extern void textbox(U8 ID, U16 x,U16 y,U16 w,U16 h);
extern void frame(U8 ID, U16 x,U16 y,U16 w,U16 h,U8 *name);

extern void touch_reset();
extern void touch_press_down();
extern void touch_loosen();

#endif