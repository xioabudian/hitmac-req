#include "IMP.h"
#include <stdio.h>
// #include "cc26xx-uart.h"

void send(U8 d){//by huangxiaobing , screen display
  printf("%c",d);
  // cc26xx_uart_write_byte(d);
}

void display_ID()      
{
    send(0xaa);
    send(0x00);
}

void reset()
{
    send(0xaa);
    send(0x01);
}

void modify_ID(U8 oid,U8 nid)
{
    send(0xaa);
    send(oid);
    send(0x15);
    send(nid);
}

void clear_display(U8 ID)
{
    send(0xaa);
	if(ID!=0)
	{
        send(ID);
	}
    send(0x13);
}

void display_on(U8 ID)
{
    send(0xaa);
    if(ID!=0)
	{
        send(ID);
	}
    send(0x10);
}

void display_off(U8 ID)
{
    send(0xaa);
    if(ID!=0)
	{
        send(ID);
	}
    send(0x11);
}


void baud(U8 ID,U32 n)
{
    send(0xaa);
    if(ID!=0)
	{
        send(ID);
	}
    send(0x14);
    if(n==9600)
    {
        send(0);
    }
    else if(n==19200)
    {
        send(1);
    }

    else if(n==38400)
    {
        send(2);
    }

    else if(n==57600)
    {
        send(3);
    }
    else if(n==115200)
    {
        send(4);
    }


}

void foreground_color(U8 ID,U8 r,U8 g,U8 b)
{
    send(0xaa);
    if(ID!=0)
	{
        send(ID);
	}
    send(0x21);
    send(r);
    send(g);
    send(b);
}

void background_color(U8 ID,U8 r,U8 g,U8 b)
{
    send(0xaa);
    if(ID!=0)
	{
        send(ID);
	}
    send(0x20);
    send(r);
    send(g);
    send(b);
}

void background_transparen(U8 ID)
{
    send(0xaa);
    if(ID!=0)
	{
        send(ID);
	}
    send(0x22);
    send(0x01);
}

void background_nor_transparen(U8 ID)
{
    send(0xaa);
    if(ID!=0)
	{
        send(ID);
	}
    send(0x22);
    send(0x00);
}

void cusor_xy(U8 ID,U16 x,U16 y)
{
    send(0xaa);
    if(ID!=0)
	{
        send(ID);
	}
    send(0x23);
    send(x>>8);
    send(x);
    send(y>>8);
    send(y);
}

void simplified_chinese(U8 ID)
{
    send(0xaa);
    if(ID!=0)
	{
        send(ID);
	}
    send(0x24);
    send(0);
}

void traditional_chinese(U8 ID)
{
    send(0xaa);
    if(ID!=0)
	{
        send(ID);
	}
    send(0x24);
    send(1);
}

void japanese(U8 ID)
{
    send(0xaa);
    if(ID!=0)
	{
        send(ID);
	}
    send(0x24);
    send(2);
}

void korean(U8 ID)
{
    send(0xaa);
    if(ID!=0)
	{
        send(ID);
	}
    send(0x24);
    send(3);
}

void font_size_16(U8 ID)
{
    send(0xaa);
    if(ID!=0)
	{
        send(ID);
	}
    send(0x25);
    send(0);
}

void font_size_24(U8 ID)
{
    send(0xaa);
    if(ID!=0)
	{
        send(ID);
	}
    send(0x25);
    send(1);
}

void font_size_32(U8 ID)
{
    send(0xaa);
    if(ID!=0)
	{
        send(ID);
	}
    send(0x25);
    send(2);
}

void line_spacing(U8 ID,U8 n)
{
    send(0xaa);
    if(ID!=0)
	{
        send(ID);
	}
    send(0x26);
    send(n);
}

void row_spacing(U8 ID,U8 n)
{
    send(0xaa);
    if(ID!=0)
	{
        send(ID);
	}
    send(0x27);
    send(n);
}

void print_string(U8 ID,U8 *ptr)
{
    send(0xaa);
    if(ID!=0)
	{
        send(ID);
	}
    send(0x28);
    while(*ptr!='\0')
    {
        send(*ptr++);             
    }
    send(0X0d);
}

void print_string_xy(U8 ID,U16 x,U16 y,U8 *ptr)
{

    send(0xaa);
    if(ID!=0)
	{
        send(ID);
	}
    send(0x23);
    send(x>>8);
    send(x);
    send(y>>8);
    send(y);
    send(0xaa);	
	if(ID!=0)
	{
        send(ID);
	}
    send(0x28);
    while(*ptr!='\0')
    {
        send(*ptr++);             
    }
    send(0X0d);
}

void print_U32(U8 ID,U32 n)
{
    U32 i,temp;
    U8 t=0;
    send(0xaa);
    if(ID!=0)
	{
        send(ID);
	}
    send(0x28);
	if(n==0)
	{
        send(0x30);
	}
	else
	{
        for(i=1000000000;i>=1;i=i/10)
        {
            temp=n/i;
            if(t==0&&temp)
            {
                send(temp%10+0x30);
                t=1;
            }
            else if(t==1)
            {
                send(temp%10+0x30);
            }
		}
    }
    send(0x0d);
}


void print_U32_xy(U8 ID,U16 x,U16 y,U32 n)
{
    U32 i,temp;
    U8 t=0;
    send(0xaa);
    if(ID!=0)
	{
        send(ID);
	}
    send(0x23);
    send(x>>8);
    send(x);
    send(y>>8);
    send(y);
    send(0xaa);
    if(ID!=0)
	{
        send(ID);
	}
    send(0x28);
	if(n==0)
	{
        send(0x30);
	}
	else
	{
        for(i=1000000000;i>=1;i=i/10)
        {
            temp=n/i;
            if(t==0&&temp)
            {
                send(temp%10+0x30);
                t=1;
            }
            else if(t==1)
            {
                send(temp%10+0x30);
            }
		}
    }
    send(0x0d);
}

void draw_pixel(U8 ID,U16 x,U16 y,U8 r,U8 g,U8 b)
{
    send(0xaa);
    if(ID!=0)
	{
        send(ID);
	}
    send(0x30);
    send(x>>8);
    send(x);
    send(y>>8);
    send(y);
    send(r);
    send(g);
    send(b);
}

void draw_line(U8 ID,U16 x1,U16 y1,U16 x2,U16 y2,U8 r,U8 g,U8 b)
{
    send(0xaa);
    if(ID!=0)
	{
        send(ID);
	}
    send(0x31);
    send(x1>>8);
    send(x1);
    send(y1>>8);
    send(y1);
    send(x2>>8);
    send(x2);
    send(y2>>8);
    send(y2);
    send(r);
    send(g);
    send(b);
}

void draw_rectangle(U8 ID,U16 x,U16 y,U16 w,U16 h,U8 r,U8 g,U8 b)
{
    send(0xaa);
    if(ID!=0)
	{
        send(ID);
	}
    send(0x36);
    send(x>>8);
    send(x);
    send(y>>8);
    send(y);
    send(w>>8);
    send(w);
    send(h>>8);
    send(h);
    send(r);
    send(g);
    send(b);
}

void draw_rectangle_fill(U8 ID,U16 x,U16 y,U16 w,U16 h,U8 r,U8 g,U8 b)
{
    send(0xaa);
    if(ID!=0)
	{
        send(ID);
	}
    send(0x37);
    send(x>>8);
    send(x);
    send(y>>8);
    send(y);
    send(w>>8);
    send(w);
    send(h>>8);
    send(h);
    send(r);
    send(g);
    send(b);
}

void draw_circle_square(U8 ID,U16 x,U16 y,U16 w,U16 h,U16 ra,U8 r,U8 g,U8 b)
{
    send(0xaa);
    if(ID!=0)
	{
        send(ID);
	}
    send(0x38);
    send(x>>8);
    send(x);
    send(y>>8);
    send(y);
    send(w>>8);
    send(w);
    send(h>>8);
    send(h);
	send(ra>>8);
    send(ra);
    send(r);
    send(g);
    send(b);
}

void draw_circle_square_fill(U8 ID,U16 x,U16 y,U16 w,U16 h,U16 ra,U8 r,U8 g,U8 b)
{
    send(0xaa);
    if(ID!=0)
	{
        send(ID);
	}
    send(0x39);
    send(x>>8);
    send(x);
    send(y>>8);
    send(y);
    send(w>>8);
    send(w);
    send(h>>8);
    send(h);
	send(ra>>8);
    send(ra);
    send(r);
    send(g);
    send(b);
}


void draw_circle(U8 ID,U16 x,U16 y,U16 ra,U8 r,U8 g,U8 b)
{
    send(0xaa);
    if(ID!=0)
	{
        send(ID);
	}
    send(0x32);
    send(x>>8);
    send(x);
    send(y>>8);
    send(y);
    send(ra>>8);
    send(ra);
    send(r);
    send(g);
    send(b);
}

void draw_circle_fill(U8 ID,U16 x,U16 y,U16 ra,U8 r,U8 g,U8 b)
{
    send(0xaa);
    if(ID!=0)
	{
        send(ID);
	}
    send(0x33);
    send(x>>8);
    send(x);
    send(y>>8);
    send(y);
    send(ra>>8);
    send(ra);
    send(r);
    send(g);
    send(b);
}

void draw_ellipse(U8 ID,U16 x,U16 y,U16 xra,U16 yra,U8 r,U8 g,U8 b)
{
    send(0xaa);
    if(ID!=0)
	{
        send(ID);
	}
    send(0x34);
    send(x>>8);
    send(x);
    send(y>>8);
    send(y);
    send(xra>>8);
    send(xra);
	send(yra>>8);
    send(yra);
    send(r);
    send(g);
    send(b);
}
void draw_ellipse_fill(U8 ID,U16 x,U16 y,U16 xra,U16 yra,U8 r,U8 g,U8 b)
{
    send(0xaa);
    if(ID!=0)
	{
        send(ID);
	}
    send(0x35);
    send(x>>8);
    send(x);
    send(y>>8);
    send(y);
    send(xra>>8);
    send(xra);
	send(yra>>8);
    send(yra);
    send(r);
    send(g);
    send(b);
}

void picture_xy(U8 ID,U8 *filename,U16 x,U16 y)
{
    send(0xaa);
    if(ID!=0)
	{
        send(ID);
	}
    send(0x40);
	while(*filename!='\0')
    {
        send(*filename++);             
    }
    send(0X0d);
    send(x>>8);
    send(x);
    send(y>>8);
    send(y);

}

void pitrure_spread(U8 ID,U8 *filename,U16 x,U16 y,U8 s)
{
    send(0xaa);
    if(ID!=0)
	{
        send(ID);
	}
    send(0x41);
	while(*filename!='\0')
    {
        send(*filename++);             
    }
    send(0X0d);
    send(x>>8);
    send(x);
    send(y>>8);
    send(y);
	send(0);
	send(s);

}
void picture_right_to_left(U8 ID,U8 *filename,U16 x,U16 y,U8 s)
{
    send(0xaa);
    if(ID!=0)
	{
        send(ID);
	}
    send(0x41);
	while(*filename!='\0')
    {
        send(*filename++);             
    }
    send(0X0d);
    send(x>>8);
    send(x);
    send(y>>8);
    send(y);
	send(2);
	send(s);

}

void picture_left_to_right(U8 ID,U8 *filename,U16 x,U16 y,U8 s)
{
    send(0xaa);
    if(ID!=0)
	{
        send(ID);
	}
    send(0x41);
	while(*filename!='\0')
    {
        send(*filename++);             
    }
    send(0X0d);
    send(x>>8);
    send(x);
    send(y>>8);
    send(y);
	send(1);
	send(s);

}

void picture_up_to_down(U8 ID,U8 *filename,U16 x,U16 y,U8 s)
{
    send(0xaa);
    if(ID!=0)
	{
        send(ID);
	}
    send(0x41);
	while(*filename!='\0')
    {
        send(*filename++);             
    }
    send(0X0d);
    send(x>>8);
    send(x);
    send(y>>8);
    send(y);
	send(3);
	send(s);

}

void picture_down_to_up(U8 ID,U8 *filename,U16 x,U16 y,U8 s)
{
    send(0xaa);
    if(ID!=0)
	{
        send(ID);
	}
    send(0x41);
	while(*filename!='\0')
    {
        send(*filename++);             
    }
    send(0X0d);
    send(x>>8);
    send(x);
    send(y>>8);
    send(y);
	send(4);
	send(s);

}

void picture_horizontally(U8 ID,U8 *filename,U16 x,U16 y,U8 s)
{
    send(0xaa);
    if(ID!=0)
	{
        send(ID);
	}
    send(0x41);
	while(*filename!='\0')
    {
        send(*filename++);             
    }
    send(0X0d);
    send(x>>8);
    send(x);
    send(y>>8);
    send(y);
	send(5);
	send(s);

}

void picture_vertical(U8 ID,U8 *filename,U16 x,U16 y,U8 s)
{
    send(0xaa);
    if(ID!=0)
	{
        send(ID);
	}
    send(0x41);
	while(*filename!='\0')
    {
        send(*filename++);             
    }
    send(0X0d);
    send(x>>8);
    send(x);
    send(y>>8);
    send(y);
	send(6);
	send(s);

}

void picture_clip(U8 ID,U8 *filename,U16 x,U16 y,U16 lx,U16 ly,U16 w,U16 h)
{
    send(0xaa);
    if(ID!=0)
	{
        send(ID);
	}
    send(0x42);
	while(*filename!='\0')
    {
        send(*filename++);             
    }
    send(0X0d);
    send(x>>8);
    send(x);
    send(y>>8);
    send(y);
    send(lx>>8);
    send(lx);
    send(ly>>8);
    send(ly);
	send(w>>8);
    send(w);
    send(h>>8);
    send(h);
}

void picture_zom(U8 ID,U8 *filename,U16 x,U16 y,U16 w,U16 h)
{
    send(0xaa);
    if(ID!=0)
	{
        send(ID);
	}
    send(0x43);
	while(*filename!='\0')
    {
        send(*filename++);             
    }
    send(0X0d);
    send(x>>8);
    send(x);
    send(y>>8);
    send(y);
	send(w>>8);
    send(w);
    send(h>>8);
    send(h);
}
void picture_best_show(U8 ID,U8 *filename,U16 x,U16 y,U16 w,U16 h)
{
    send(0xaa);
    if(ID!=0)
	{
        send(ID);
	}
    send(0x44);
	while(*filename!='\0')
    {
        send(*filename++);             
    }
    send(0X0d);
    send(x>>8);
    send(x);
    send(y>>8);
    send(y);
	send(w>>8);
    send(w);
    send(h>>8);
    send(h);
}

void load_buffer(U8 ID,U8 *filename,U8 n)
{
    send(0xaa);
    if(ID!=0)
	{
        send(ID);
	}
    send(0x45);
	while(*filename!='\0')
    {
        send(*filename++);             
    }
    send(0X0d);
    send(n);
}

void unload_buffer(U8 ID,U8 n)
{
    send(0xaa);
    if(ID!=0)
	{
        send(ID);
	}
    send(0x46);
    send(n);
}

void buffer_xy(U8 ID,U8 n,U16 x,U16 y)
{
    send(0xaa);
    if(ID!=0)
	{
        send(ID);
	}
    send(0x47);
	send(n);
    send(x>>8);
    send(x);
    send(y>>8);
    send(y);

}

void buffer_spread(U8 ID,U8 n,U16 x,U16 y,U8 s)
{
    send(0xaa);
    if(ID!=0)
	{
        send(ID);
	}
    send(0x48);
	send(n);
    send(x>>8);
    send(x);
    send(y>>8);
    send(y);
	send(0);
	send(s);

}
void buffer_right_to_left(U8 ID,U8 n,U16 x,U16 y,U8 s)
{
    send(0xaa);
    if(ID!=0)
	{
        send(ID);
	}
    send(0x48);
	send(n);
    send(x>>8);
    send(x);
    send(y>>8);
    send(y);
	send(2);
	send(s);

}

void buffer_left_to_right(U8 ID,U8 n,U16 x,U16 y,U8 s)
{
    send(0xaa);
    if(ID!=0)
	{
        send(ID);
	}
    send(0x48);
	send(n);
    send(x>>8);
    send(x);
    send(y>>8);
    send(y);
	send(1);
	send(s);

}

void buffer_up_to_down(U8 ID,U8 n,U16 x,U16 y,U8 s)
{
    send(0xaa);
    if(ID!=0)
	{
        send(ID);
	}
    send(0x48);
	send(n);
    send(x>>8);
    send(x);
    send(y>>8);
    send(y);
	send(3);
	send(s);

}

void buffer_down_to_up(U8 ID,U8 n,U16 x,U16 y,U8 s)
{
    send(0xaa);
    if(ID!=0)
	{
        send(ID);
	}
    send(0x48);
	send(n);
    send(x>>8);
    send(x);
    send(y>>8);
    send(y);
	send(4);
	send(s);

}

void buffer_horizontally(U8 ID,U8 n,U16 x,U16 y,U8 s)
{
    send(0xaa);
    if(ID!=0)
	{
        send(ID);
	}
    send(0x48);
	send(n);
    send(x>>8);
    send(x);
    send(y>>8);
    send(y);
	send(5);
	send(s);

}

void buffer_vertical(U8 ID,U8 n,U16 x,U16 y,U8 s)
{
    send(0xaa);
    if(ID!=0)
	{
        send(ID);
	}
    send(0x48);
	send(n);
    send(x>>8);
    send(x);
    send(y>>8);
    send(y);
	send(6);
	send(s);

}

void buffer_clip(U8 ID,U8 n,U16 x,U16 y,U16 lx,U16 ly,U16 w,U16 h)
{
    send(0xaa);
    if(ID!=0)
	{
        send(ID);
	}
    send(0x49);
	send(n);
    send(x>>8);
    send(x);
    send(y>>8);
    send(y);
    send(lx>>8);
    send(lx);
    send(ly>>8);
    send(ly);
	send(w>>8);
    send(w);
    send(h>>8);
    send(h);
}

void buffer_zom(U8 ID,U8 n,U16 x,U16 y,U16 w,U16 h)
{
    send(0xaa);
    if(ID!=0)
	{
        send(ID);
	}
    send(0x4A);
	send(n);
    send(x>>8);
    send(x);
    send(y>>8);
    send(y);
	send(w>>8);
    send(w);
    send(h>>8);
    send(h);
}
void buffer_best_show(U8 ID,U8 n,U16 x,U16 y,U16 w,U16 h)
{
    send(0xaa);
    if(ID!=0)
	{
        send(ID);
	}
    send(0x4B);
	send(n);
    send(x>>8);
    send(x);
    send(y>>8);
    send(y);
	send(w>>8);
    send(w);
    send(h>>8);
    send(h);
}

void buffer_autodisplay(U8 ID,U8 n1,U8 n2,U16 x,U16 y)
{
    send(0xaa);
    if(ID!=0)
	{
        send(ID);
	}
	send(0X4C);
    send(n1);
	send(n2);
    send(x>>8);
    send(x);
    send(y>>8);
    send(y);
}

void buffer_autodisplay_speed(U8 ID,U8 n)
{
    send(0xaa);
    if(ID!=0)
	{
        send(ID);
	}
	send(0X4D);
    send(n);
}

void buffer_autodisplay_stop(U8 ID)
{
    send(0xaa);
    if(ID!=0)
	{
        send(ID);
	}
	send(0X4E);
}

void windows(U8 ID, U16 x,U16 y,U16 w,U16 h,U8 *name)
{
    send(0xaa);
    if(ID!=0)
	{
        send(ID);
	}
    send(0x50);
    send(x>>8);
    send(x);
    send(y>>8);
    send(y);
	send(w>>8);
    send(w);
    send(h>>8);
    send(h);
	while(*name!='\0')
    {
        send(*name++);             
    }
    send(0X0d);
}

void windows_click(U8 ID, U16 x,U16 y,U16 w,U16 h)
{
    send(0xaa);
    if(ID!=0)
	{
        send(ID);
	}
    send(0x51);
    send(x>>8);
    send(x);
    send(y>>8);
    send(y);
	send(w>>8);
    send(w);
    send(h>>8);
    send(h);
}

void button(U8 ID, U16 x,U16 y,U16 w,U16 h)
{
    send(0xaa);
    if(ID!=0)
	{
        send(ID);
	}
    send(0x52);
    send(x>>8);
    send(x);
    send(y>>8);
    send(y);
	send(w>>8);
    send(w);
    send(h>>8);
    send(h);
}

void button_click(U8 ID, U16 x,U16 y,U16 w,U16 h)
{
    send(0xaa);
    if(ID!=0)
	{
        send(ID);
	}
    send(0x53);
    send(x>>8);
    send(x);
    send(y>>8);
    send(y);
	send(w>>8);
    send(w);
    send(h>>8);
    send(h);
}

void textbox(U8 ID, U16 x,U16 y,U16 w,U16 h)
{
    send(0xaa);
    if(ID!=0)
	{
        send(ID);
	}
    send(0x54);
    send(x>>8);
    send(x);
    send(y>>8);
    send(y);
	send(w>>8);
    send(w);
    send(h>>8);
    send(h);
}

void frame(U8 ID, U16 x,U16 y,U16 w,U16 h,U8 *name)
{
    send(0xaa);
    if(ID!=0)
	{
        send(ID);
	}
    send(0x55);
    send(x>>8);
    send(x);
    send(y>>8);
    send(y);
	send(w>>8);
    send(w);
    send(h>>8);
    send(h);
	while(*name!='\0')
    {
        send(*name++);             
    }
    send(0X0d);
}

void touch_reset()
{
    send(0xaa);
    send(0x72);
}

void touch_press_down()
{
    send(0xaa);
    send(0x70);
}

void touch_loosen()
{
    send(0xaa);
    send(0x1);
}

void print_key_value(char key[],int value)
{
    static char print_count = 0;

    unsigned char strcpy[105];
    char *ptr = key;
    unsigned char str[3] = "-";
    int len = 0;
    while(*ptr!='\0'){
        strcpy[len] = (unsigned char)*ptr;
        ptr++;
        len++;
        if(len>100){
            break;
        }
    }
    strcpy[len]='\0';
    print_string_xy(0xFF,0,print_count*20,strcpy);
    if(value>=0){
        print_U32(0xFF,value);
    }else{
        print_string(0xFF,str);
        print_U32(0xFF,value*(-1));
    }

    if(print_count==11){
        print_count = 0;
        clear_display(0xFF);
    }
    print_count ++;
}