/********************************************************;
;*	 IC		: ST7539			*;
;*       MCU type       : W78E52B(8K ROM)               *;
;*       Data		: 2012.04.21      4Line mode 	*;
;*       	                       			 	*;
;********************************************************/
#include<reg51.h>
#include<intrins.h>
#include<P2.h>
#include<P3.h>
#include<P4.h>
#define uint unsigned int
#define uchar unsigned char
//========================================================
sbit RES=P3^1;
sbit CS1=P3^2;
sbit A0=P3^0;
sbit SCL=P1^0;
sbit SDI=P1^1;
sbit k1=P3^7;
sbit k2=P3^6;
sbit k3=P3^4;
//========================================================
void init();
void init1();
void clealddram();
void font1();
void font2();
void showpic(char *k);
void showpic1(char *k);
void write_com(int para);
void write_data(int para);
char keyscan();
void delay(int t);
int yy;
//=========================================================
/*
unsigned char code bmp1[]={
       0xFF,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,  
    0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,  
    0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,  
    0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,  
    0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,  
    0x01,0x01,0x01,0x01,0x01,0x01,0x01,0xFF,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x08,  
    0xF8,0x48,0xC8,0x88,0x40,0xB0,0x1C,0x10,0x90,0x70,0x00,0x40,0xE0,0x58,0x44,0xC8,0x10,0x00,0xF0,0x00,0x00,0xFC,0x00,0x00,  
    0xF0,0x50,0x50,0x50,0xFC,0x50,0x50,0x50,0xF8,0x10,0x00,0x80,0x84,0x84,0x84,0x84,0xE4,0xA4,0x94,0x8C,0xC4,0x80,0x00,0x08,  
    0x88,0x48,0xE8,0x38,0x2C,0x28,0x28,0xE8,0x08,0x08,0x00,0xFC,0x04,0x64,0x9C,0x00,0xFC,0x94,0x94,0x94,0xFC,0x00,0x00,0x80,  
    0x40,0x20,0x1C,0x00,0xC0,0x0C,0x30,0x40,0x80,0x80,0x00,0x20,0x24,0xA4,0xA4,0xA4,0xA4,0xB4,0x24,0x04,0xFC,0x00,0x00,0x00,  
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,  
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x0F,0x08,0x0F,0x10,0x10,0x08,0x05,0x02,0x01,0x00,0x00,0x00,0x1F,0x10,0x12,0x13,  
    0x10,0x1C,0x03,0x10,0x10,0x1F,0x00,0x00,0x07,0x02,0x02,0x02,0x0F,0x12,0x12,0x12,0x13,0x18,0x00,0x00,0x00,0x00,0x10,0x10,  
    0x1F,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x1F,0x05,0x05,0x05,0x15,0x1F,0x00,0x00,0x00,0x1F,0x02,0x02,0x03,0x00,  
    0x1F,0x10,0x0B,0x04,0x0A,0x11,0x00,0x00,0x10,0x18,0x14,0x13,0x10,0x0A,0x0C,0x18,0x00,0x00,0x00,0x00,0x00,0x07,0x04,0x04,  
    0x04,0x07,0x10,0x10,0x1F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,  
    0xFF,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,  
    0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,  
    0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,  
    0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,  
    0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,  
    0x80,0x80,0x80,0x80,0x80,0x80,0x80,0xFF};*/
//========================================================
void init()
{
    	RES=0;
    	delay(2);
    	RES=1;
    	delay(20);  	
	
    	write_com(0xa0);		//Frame频率
    	write_com(0xeb);		//1/9 Bias
    	write_com(0x81);		//Set VOP
    	write_com(0x85);		//
    	write_com(0xc2);		//MX\MY
    	write_com(0x84);
    	write_com(0xf1);		//Duty
    	write_com(0x3f);		//64
    	
    	//write_com(0xf2);		//
    	//write_com(0x10);		//
    	//write_com(0xf3);
    	//write_com(0x2f);		//
    	//write_com(0x85);		//
    	
    	clealddram();
    	write_com(0xaf);		//Display on
}
//========================================================
void clealddram()			//清屏
{
	int i,j;
	for(i=0Xb0;i<0Xb9;i++)
	{
	    write_com(i);
	    write_com(0x10);
	    write_com(0x00);
	    for(j=0;j<192;j++)
	{
	    write_data(0x00);
	}
	}
}
//========================================================
void font1()				//全显
{
	int i,j;
	for(i=0Xb0;i<0Xb9;i++)
	{
	    write_com(i);
	    write_com(0x10);
	    write_com(0x00);
	    for(j=0;j<192;j++)
	{
	    write_data(0xff);
	}
	}
}
//========================================================
void font2()				//横线
{
	int i,j;
	for(i=0Xb0;i<0Xb9;i++)
	{
	    write_com(i);
	    write_com(0x10);
	    write_com(0x00);
	    for(j=0;j<192;j++)
	{
	    write_data(0x55);
	}
	}
}
//========================================================
void font3()				//竖线
{
	int i,j;
	for(i=0Xb0;i<0Xb9;i++)
	{
	    write_com(i);
	    write_com(0x10);
	    write_com(0x00);
	    for(j=0;j<96;j++)
	{
	    write_data(0xff);
	    write_data(0x00);
	}
	}
}
//========================================================
void font4()				//雪花
{
	int i,j;
	for(i=0Xb0;i<0Xb9;i++)
	{
	    write_com(i);
	    write_com(0x10);
	    write_com(0x00);
	    for(j=0;j<96;j++)
	{
	    write_data(0x55);
	    write_data(0xaa);
	}
	}
}
//========================================================
void showpic(char *k)			//显示图片
{
	int i,j;
	    for(i=0xb0;i<0xb8;i++)
	{
	    write_com(0xc4);		//MX=1,MY=1
	    //write_com(0x40);		//Set Scroll Line
	    write_com(i);
	    write_com(0x10);
	    write_com(0x00);
	    for(j=0;j<192;j++)
	{
	    write_data(*k++);
	}
	}
}
//========================================================
void showpic1(char *k)			//显示图片
{
	int i,j;
	
	    for(i=0xb0;i<0xb8;i++)
	{
	    write_com(0xc2);		//MX=1,MY=0
	    //write_com(0x40);		//Set Scroll Line
	    write_com(i);
	    write_com(0x10);
	    write_com(0x00);
	    for(j=0;j<192;j++)
	{
	    write_data(*k++);
	}
	}
}
//========================================================
void write_com(int para)
{
	int j;
	j=8;
	CS1=0;
	A0=0;
	do
{
  	if(para&0x80)
    	SDI=1;
  	else
    	SDI=0;
    	SCL=0;
	//delay(2);
    	SCL=1;
  	--j;
    	para<<=1;
}
 	while(j);
	CS1=1;
}
//========================================================
void write_data(int para)
{
	int j;
	j=8;
	CS1=0;
	A0=1;
	do
{
  	if(para&0x80)
    	SDI=1;
  	else
    	SDI=0;
    	SCL=0;
    	//delay(2);
    	SCL=1;
  	--j;
    	para<<=1;
}
 	while(j);
	CS1=1;
}
//========================================================
char keyscan()
{
	while(1)
{
	delay(10);
	if(k1==0)
{
	delay(10);
	return(1);
}

	delay(10);
	if(k2==0)
{
	delay(10);
	return(2);
}

	delay(10);
	if(k3==0)
{
	delay(10);
	return(3);
}
}
}
//========================================================
void delay(int t)
{
	register int i,j;
	for(i=0;i<t;i++)		//后面不要跟分号
	for(j=0;j<125;j++);
}
//========================================================
main()
{
	init();
    	delay(200);
    	
	while(1)
{
	showpic(bmp3);
	delay(500);
	//keyscan();
    showpic1(bmp2);
	delay(500);
	//keyscan();
	
	//showpic1(bmp4);
	//delay(500);
	//keyscan();
	

	
	font1();                        //全显
	delay(500);
	//keyscan();
	
	font2();                        //横线
	delay(500);
	//keyscan();
	
	font3();			//竖线
	delay(500);
	//keyscan();
	
	font4();			//雪花
	delay(500);
	//keyscan();			
}
}

