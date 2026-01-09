#include <SPI.h>
#include <string.h>
#include <stdlib.h>    

#include "epd_2inch13.h"
#include "epd_gui.h"

//Configure the serial port to use the standard printf function
//start
//int serial_putc( char c, struct __file * )
//{
//    Serial.write( c );
//    return c;
//}
//void printf_begin(void)
//{
//    fdevopen( &serial_putc, 0 );
//}
//end
   
///////////////////EXTERNAL FUNCTION////////////////////////////////////////////////////////////////////////
/////////////////////delay//////////////////////////////////////
void driver_delay_us(unsigned int xus)  //1us
{
    for(;xus>1;xus--);
}
void driver_delay_xms(unsigned long xms) //1ms
{  
    unsigned long i = 0 , j=0;

    for(j=0;j<xms;j++)
    {
        for(i=0; i<256; i++);
    }
}
void DELAY_S(unsigned int delaytime)     
{
    int i,j,k;
    for(i=0;i<delaytime;i++)
    {
        for(j=0;j<4000;j++)           
        {
            for(k=0;k<222;k++);               
        }
    }
}
//////////////////////SPI///////////////////////////////////
void SPI_Delay(unsigned char xrate)
{
  unsigned char i;
  while(xrate)
  {
      for(i=0;i<2;i++);
      xrate--;
  }
}
void SPI_Write(unsigned char value)                                    
{                                                           
    unsigned char i;  
    SPI_Delay(1);
    for(i=0; i<8; i++)   
    {
        EPD_W21_CLK_0;
        SPI_Delay(1);
        if(value & 0x80)
           EPD_W21_MOSI_1;
        else
           EPD_W21_MOSI_0;   
        value = (value << 1); 
        SPI_Delay(1);
        driver_delay_us(1);
        EPD_W21_CLK_1; 
        SPI_Delay(1);
    }
}

void Epaper_Write_Command(unsigned char command)
{
   SPI_Delay(1);
   EPD_W21_CS_0;                   
   EPD_W21_DC_0;   // command write
   SPI_Write(command);
   EPD_W21_CS_1;
}
void Epaper_Write_Data(unsigned char data)
{
   SPI_Delay(1);
   EPD_W21_CS_0;                   
   EPD_W21_DC_1;   // command write
   SPI_Write(data);
   EPD_W21_CS_1;
}

/////////////////EPD settings Functions/////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////
void EPD_HW_Init(void)
{
  EPD_W21_RST_0;     
  delay(1); 
  EPD_W21_RST_1; //hard reset  
  delay(1); 

  Epaper_READBUSY();   
  Epaper_Write_Command(0x12);  //SWRESET
  Epaper_READBUSY();   
    
  Epaper_Write_Command(0x01); //Driver output control      
  Epaper_Write_Data(0xF9);
  Epaper_Write_Data(0x00);
  Epaper_Write_Data(0x00);

  Epaper_Write_Command(0x11); //data entry mode       
  Epaper_Write_Data(0x02);

  Epaper_Write_Command(0x44); //set Ram-X address start/end position   
  Epaper_Write_Data(0x0F);
  Epaper_Write_Data(0x00);    //0x0C-->(15+1)*8=128

  Epaper_Write_Command(0x45); //set Ram-Y address start/end position          
  Epaper_Write_Data(0x00);   //0xF9-->(249+1)=250
  Epaper_Write_Data(0x00);
  Epaper_Write_Data(0xF9);
  Epaper_Write_Data(0x00); 

  Epaper_Write_Command(0x3C); //BorderWavefrom
  Epaper_Write_Data(0x05); 
   
  Epaper_Write_Command(0x21); //  Display update control
  Epaper_Write_Data(0x00);    
  Epaper_Write_Data(0x80); 
         
  Epaper_Write_Command(0x18); 
  Epaper_Write_Data(0x80);  

  Epaper_Write_Command(0x4E);   // set RAM x address count to 0;
  Epaper_Write_Data(0x0F);
  Epaper_Write_Command(0x4F);   // set RAM y address count to 249;    
  Epaper_Write_Data(0x00);
  Epaper_Write_Data(0x00);
  Epaper_READBUSY();
  
}
void EPD_HW_Init_Fast(void)
{
  EPD_W21_RST_0;     
  delay(1); 
  EPD_W21_RST_1; //hard reset  
  delay(1); 
		
	Epaper_Write_Command(0x12);  //SWRESET
	Epaper_READBUSY();   
	Epaper_Write_Command(0x01); 
	Epaper_Write_Data(0x27);
    Epaper_Write_Data(0x01);
    Epaper_Write_Data(0x00);	
	
	Epaper_Write_Command(0x11); //Read built-in temperature sensor
	Epaper_Write_Data(0x02);	

	Epaper_Write_Command(0x44); 
	Epaper_Write_Data(0x0F);
    Epaper_Write_Data(0x00);
    
	Epaper_Write_Command(0x45); 
	Epaper_Write_Data(0x00);
    Epaper_Write_Data(0x00);
    Epaper_Write_Data(0xF9);
    Epaper_Write_Data(0x00);
			
	Epaper_Write_Command(0x18); //Read built-in temperature sensor
	Epaper_Write_Data(0x80);	
				
	Epaper_Write_Command(0x22); // Load temperature value
	Epaper_Write_Data(0xB1);		
	Epaper_Write_Command(0x20);	
	Epaper_READBUSY(); 
	
	Epaper_Write_Command(0x4E); 
	Epaper_Write_Data(0x0F);  
	Epaper_Write_Command(0x4F); 
	Epaper_Write_Data(0x00); 
	Epaper_Write_Data(0x00);   
	
	Epaper_Write_Command(0x1A); // Write to temperature register
	Epaper_Write_Data(0x64);		
	Epaper_Write_Data(0x00);	
							
	Epaper_Write_Command(0x22); // Load temperature value
	Epaper_Write_Data(0x91);		
	Epaper_Write_Command(0x20);	
	Epaper_READBUSY();   
}
//////////////////////////////All screen update////////////////////////////////////////////
void EPD_WhiteScreen_ALL(const unsigned char *BWdatas,const unsigned char *RWdatas)
{
   unsigned int i,j,index;
    Epaper_Write_Command(0x24);   //write RAM for black(0)/white (1)
  for(j=0;j<MAX_COLUMN_BYTES;j++)//
  {
		for(i=0;i<MAX_LINE_BYTES;i++)
		{    
      index = i+j*MAX_LINE_BYTES;
			if(i==0)  
			{
			   Epaper_Write_Data((pgm_read_byte(&BWdatas[index])>>6)|0xfc);
			}
			else
			{
				Epaper_Write_Data((pgm_read_byte(&BWdatas[index-1])<<2)|(pgm_read_byte(&BWdatas[index])>>6));
			}         
		}
  }
    Epaper_Write_Command(0x26);   //write RAM for red(0)/white (1)
  for(j=0;j<MAX_COLUMN_BYTES;j++)//
  {
		for(i=0;i<MAX_LINE_BYTES;i++)
		{    
      index = i+j*MAX_LINE_BYTES;
			if(i==0)  
			{
			   Epaper_Write_Data(~((pgm_read_byte(&RWdatas[index])>>6)|0xfc));
			}
			else
			{
				Epaper_Write_Data(~((pgm_read_byte(&RWdatas[index-1])<<2)|(pgm_read_byte(&RWdatas[index])>>6)));
			}         
		}
  }
   EPD_Update();   
}
void EPD_WhiteScreen_ALL_Fast(const unsigned char *BWdatas,const unsigned char *RWdatas)
{
   unsigned int i,j,index;  
    Epaper_Write_Command(0x24);   //write RAM for black(0)/white (1)
  for(j=0;j<MAX_COLUMN_BYTES;j++)//
  {
		for(i=0;i<MAX_LINE_BYTES;i++)
		{    
      index = i+j*MAX_LINE_BYTES;
			if(i==0)  
			{
			   Epaper_Write_Data((pgm_read_byte(&BWdatas[index])>>6)|0xfc);
			}
			else
			{
				Epaper_Write_Data((pgm_read_byte(&BWdatas[index-1])<<2)|(pgm_read_byte(&BWdatas[index])>>6));
			}         
		}
  } 
    Epaper_Write_Command(0x26);   //write RAM for red(0)/white (1)
  for(j=0;j<MAX_COLUMN_BYTES;j++)//
  {
		for(i=0;i<MAX_LINE_BYTES;i++)
		{    
      index = i+j*MAX_LINE_BYTES;
			if(i==0)  
			{
			   Epaper_Write_Data(~((pgm_read_byte(&RWdatas[index])>>6)|0xfc));
			}
			else
			{
				Epaper_Write_Data(~((pgm_read_byte(&RWdatas[index-1])<<2)|(pgm_read_byte(&RWdatas[index])>>6)));
			}         
		}
  }
   EPD_Update_Fast();  
}
/////////////////////////////////////////////////////////////////////////////////////////
void EPD_Update(void)
{   
  Epaper_Write_Command(0x22); 
  Epaper_Write_Data(0xF7);   
  Epaper_Write_Command(0x20); 
  Epaper_READBUSY();  

}
void EPD_Part_Update(void)
{
  Epaper_Write_Command(0x22); 
  Epaper_Write_Data(0xFF);   
  Epaper_Write_Command(0x20); 
  Epaper_READBUSY();      
}

void EPD_Update_Fast(void)
{   
  Epaper_Write_Command(0x22); //Display Update Control
  Epaper_Write_Data(0xC7);   
  Epaper_Write_Command(0x20); //Activate Display Update Sequence
  Epaper_READBUSY();   

}

void EPD_Update_BaseMap(void)
{   
  Epaper_Write_Command(0x22); //Display Update Control
  Epaper_Write_Data(0xF4);   
  Epaper_Write_Command(0x20); //Activate Display Update Sequence
  Epaper_READBUSY();   
}

void EPD_DeepSleep(void)
{  
  Epaper_Write_Command(0x10); //enter deep sleep
  Epaper_Write_Data(0x01); 
  delay(100);
}
void Epaper_READBUSY(void)
{ 
  while(1)
  {   //=1 BUSY
     if(isEPD_W21_BUSY==0) break;
  }  
}
///////////////////////////Part update//////////////////////////////////////////////
void EPD_SetRAMValue_BaseMap(const unsigned char *BWdatas,const unsigned char *RWdatas)
{
  unsigned int i,j,index;   
  const unsigned char  *datas_flag;   
  datas_flag=BWdatas;
  Epaper_Write_Command(0x24);   //Write Black and White image to RAM
  for(j=0;j<MAX_COLUMN_BYTES;j++)//
  {
		for(i=0;i<MAX_LINE_BYTES;i++)
		{    
      index = i+j*MAX_LINE_BYTES;
			if(i==0)  
			{
			   Epaper_Write_Data((pgm_read_byte(&BWdatas[index])>>6)|0xfc);
			}
			else
			{
				Epaper_Write_Data((pgm_read_byte(&BWdatas[index-1])<<2)|(pgm_read_byte(&BWdatas[index])>>6));
			}         
		}
  }
  Epaper_Write_Command(0x26);   
  for(j=0;j<MAX_COLUMN_BYTES;j++)//
  {
		for(i=0;i<MAX_LINE_BYTES;i++)
		{    
      index = i+j*MAX_LINE_BYTES;
			if(i==0)  
			{
			   Epaper_Write_Data(~((pgm_read_byte(&RWdatas[index])>>6)|0xfc));
			}
			else
			{
				Epaper_Write_Data(~((pgm_read_byte(&RWdatas[index-1])<<2)|(pgm_read_byte(&RWdatas[index])>>6)));
			}         
		}
  }
	EPD_Update_BaseMap();

	Epaper_READBUSY(); 
   BWdatas=datas_flag;
  Epaper_Write_Command(0x26);   //Write Black and White image to RAM
  for(j=0;j<MAX_COLUMN_BYTES;j++)//
  {
		for(i=0;i<MAX_LINE_BYTES;i++)
		{    
      index = i+j*MAX_LINE_BYTES;
			if(i==0)  
			{
			   Epaper_Write_Data((pgm_read_byte(&BWdatas[index])>>6)|0xfc);
			}
			else
			{
				Epaper_Write_Data((pgm_read_byte(&BWdatas[index-1])<<2)|(pgm_read_byte(&BWdatas[index])>>6));
			}         
		}
  }
}

void EPD_HW_Init_GUI(void)
{
  EPD_W21_RST_0;  // Module reset   
  delay(10);//At least 10ms delay 
  EPD_W21_RST_1;
  delay(10); //At least 10ms delay 
  
  Epaper_READBUSY();   
  Epaper_Write_Command(0x12);  //SWRESET
  Epaper_READBUSY();   
    
  Epaper_Write_Command(0x01); //Driver output control 
  delay(10); //At least 10ms delay  
  Epaper_Write_Data(0xF9);
  delay(10); //At least 10ms delay 
  Epaper_Write_Data(0x00);
  delay(10); //At least 10ms delay 
  Epaper_Write_Data(0x00);//0x00:Show normal 0x01:Show mirror

  Epaper_Write_Command(0x11); //data entry mode       
  Epaper_Write_Data(0x01);

  Epaper_Write_Command(0x44); //set Ram-X address start/end position   
  Epaper_Write_Data(0x00);
  Epaper_Write_Data(0x0F);    //0x0C-->(15+1)*8=128

  Epaper_Write_Command(0x45); //set Ram-Y address start/end position          
  Epaper_Write_Data(0xF9);    //0xF9-->(249+1)=250
  Epaper_Write_Data(0x00);
  Epaper_Write_Data(0x00);
  Epaper_Write_Data(0x00); 

  Epaper_Write_Command(0x3C); //BorderWavefrom
  Epaper_Write_Data(0x05);  
      
  Epaper_Write_Command(0x18); //Read built-in temperature sensor
  Epaper_Write_Data(0x80);  

  Epaper_Write_Command(0x4E);   // set RAM x address count to 0;
  Epaper_Write_Data(0x00);
  Epaper_Write_Command(0x4F);   // set RAM y address count to 249;    
  Epaper_Write_Data(0xF9);
  Epaper_Write_Data(0x00);
  Epaper_READBUSY();
  
}


void EPD_Dis_Part(unsigned int x_start, unsigned int y_start,
                  const unsigned char *datas,
                  unsigned int PART_COLUMN, unsigned int PART_LINE)
{
  unsigned int i;
  unsigned int x_end, y_start1, y_start2, y_end1, y_end2;

  x_start = x_start / 8;
  x_end   = 0x0F - (x_start + PART_LINE / 8 - 1);
  x_start = 0x0F - x_start;

  y_start1 = 0;
  y_start2 = y_start;
  if (y_start >= 256) { y_start1 = y_start2 / 256; y_start2 = y_start2 % 256; }

  y_end1 = 0;
  y_end2 = y_start + PART_COLUMN - 1;
  if (y_end2 >= 256) { y_end1 = y_end2 / 256; y_end2 = y_end2 % 256; }

  // DO NOT reset here. It causes flashing of the screen
  // EPD_W21_RST_0; delay(10); EPD_W21_RST_1; delay(10);

  // keep border stable (helps a bit)
  Epaper_Write_Command(0x3C);
  Epaper_Write_Data(0x80);

  Epaper_Write_Command(0x44);
  Epaper_Write_Data(x_start);
  Epaper_Write_Data(x_end);

  Epaper_Write_Command(0x45);
  Epaper_Write_Data(0xF9 - y_start2);
  Epaper_Write_Data(0xF9 - y_start1);
  Epaper_Write_Data(0xF9 - y_end2);
  Epaper_Write_Data(0xF9 - y_end1);

  Epaper_Write_Command(0x4E);
  Epaper_Write_Data(x_start);

  Epaper_Write_Command(0x4F);
  Epaper_Write_Data(y_start2);
  Epaper_Write_Data(y_start1);

  Epaper_Write_Command(0x24);
  for (i = 0; i < (PART_COLUMN * PART_LINE / 8); i++)
  {
    // datas is RAM (your boxBuf), so read directly
    Epaper_Write_Data(datas[i]);
  }

  Epaper_Write_Command(0x4E);
  Epaper_Write_Data(x_start);

  Epaper_Write_Command(0x4F);
  Epaper_Write_Data(y_start2);
  Epaper_Write_Data(y_start1);

  Epaper_Write_Command(0x26);
  for (i = 0; i < (PART_COLUMN * PART_LINE / 8); i++)
  {
    Epaper_Write_Data(0x00);
  }

  EPD_Part_Update();
}

/////////////////////////////////Single display////////////////////////////////////////////////
// void EPD_WhiteScreen_Black(void)

// {
//     unsigned int i,k;
//     Epaper_Write_Command(0x24);   //write RAM for black(0)/white (1)
//     for(k=0;k<250;k++)
//     {
//         for(i=0;i<16;i++)
//         {
//             Epaper_Write_Data(0x00);
//         }
//     }
//     EPD_Update();
// }

void EPD_WhiteScreen_White(void)

{
    unsigned int i,k;
	Epaper_Write_Command(0x24);   //write RAM for black(0)/white (1)
	for(k=0;k<250;k++)
    {
	    for(i=0;i<16;i++)
		{
		    Epaper_Write_Data(0xff);
		}
    }
	Epaper_Write_Command(0x26);   
	for(k=0;k<250;k++)
    {
	    for(i=0;i<16;i++)
		{
		    Epaper_Write_Data(0x00);
		}
    }
	EPD_Update();
}

void EPD_Display(unsigned char *BWImage,unsigned char *RWImage)
{
    unsigned int width, Height,i,j;
    width = (EPD_WIDTH % 8 == 0)? (EPD_WIDTH / 8 ): (EPD_WIDTH / 8 + 1);
    Height = EPD_HEIGHT;

    Epaper_Write_Command(0x24);
    for ( j = 0; j < Height; j++) 
	{
        for ( i = 0; i < width; i++) 
		{
           Epaper_Write_Data(BWImage[i + j * width]);
        }
    }
	Epaper_Write_Command(0x26);
    for ( j = 0; j < Height; j++) 
	{
        for ( i = 0; i < width; i++) 
		{
           Epaper_Write_Data(~ RWImage[i + j * width]);
        }
    }
    EPD_Update();    
}

/////////////////////////////////////TIME///////////////////////////////////////////////////
void EPD_Dis_Part_myself(unsigned int x_startA,unsigned int y_startA,const unsigned char * datasA,
                         unsigned int x_startB,unsigned int y_startB,const unsigned char * datasB,
                         unsigned int x_startC,unsigned int y_startC,const unsigned char * datasC,
                         unsigned int x_startD,unsigned int y_startD,const unsigned char * datasD,
                         unsigned int x_startE,unsigned int y_startE,const unsigned char * datasE,
                         unsigned int PART_COLUMN,unsigned int PART_LINE
                        )
{
    unsigned int i;  
    unsigned int x_end,y_start1,y_start2,y_end1,y_end2;
    
    //Data A////////////////////////////
    x_startA=x_startA/8;//Convert to byte
		x_end=0x0f-(x_startA+PART_LINE/8-1); 
		x_startA = 0X0F-x_startA;
    
    y_start1=0;
    y_start2=y_startA-1;
    if(y_startA>=256)
    {
        y_start1=y_start2/256;
        y_start2=y_start2%256;
    }
    y_end1=0;
    y_end2=y_startA+PART_COLUMN-1;
    if(y_end2>=256)
    {
        y_end1=y_end2/256;
        y_end2=y_end2%256;    
    }   
    
    // Add hardware reset to prevent background color change
    // EPD_W21_RST_0;  // Module reset   
    // delay(10);//At least 10ms delay 
    // EPD_W21_RST_1;
    // delay(10); //At least 10ms delay  
      //Lock the border to prevent flashing
    Epaper_Write_Command(0x3C); //BorderWavefrom,
    Epaper_Write_Data(0x80);  
    
    Epaper_Write_Command(0x44);       // set RAM x address start/end, in page 35
    Epaper_Write_Data(x_startA);    // RAM x address start at 00h;
    Epaper_Write_Data(x_end);    // RAM x address end at 0fh(15+1)*8->128 
    Epaper_Write_Command(0x45);       // set RAM y address start/end, in page 35
	  Epaper_Write_Data(0XF9-y_start2);    // RAM y address start at 0127h;
	  Epaper_Write_Data(0XF9-y_start1);    // RAM y address start at 0127h;
	  Epaper_Write_Data(0XF9-y_end2);    // RAM y address end at 00h;
	  Epaper_Write_Data(0XF9-y_end1);  


    Epaper_Write_Command(0x4E);   // set RAM x address count to 0;
    Epaper_Write_Data(x_startA); 
    Epaper_Write_Command(0x4F);   // set RAM y address count to 0X127;    
    Epaper_Write_Data(y_start2);
    Epaper_Write_Data(y_start1);
  
  
   Epaper_Write_Command(0x24);   //Write Black and White image to RAM
   for(i=0;i<PART_COLUMN*PART_LINE/8;i++)
   {                         
       Epaper_Write_Data(*datasA);
       datasA++;
   } 
  //Data B/////////////////////////////////////
  x_startB=x_startB/8;//Convert to byte
	x_end=0X0F-(x_startB+PART_LINE/8-1); 
	x_startB=0X0F-x_startB; 
  
  y_start1=0;
  y_start2=y_startB-1;
  if(y_startB>=256)
  {
      y_start1=y_start2/256;
      y_start2=y_start2%256;
  }
  y_end1=0;
  y_end2=y_startB+PART_COLUMN-1;
  if(y_end2>=256)
  {
      y_end1=y_end2/256;
      y_end2=y_end2%256;    
  }   
  
  Epaper_Write_Command(0x44);       // set RAM x address start/end, in page 35
  Epaper_Write_Data(x_startB);    // RAM x address start at 00h;
  Epaper_Write_Data(x_end);    // RAM x address end at 0fh(15+1)*8->128 
  Epaper_Write_Command(0x45);       // set RAM y address start/end, in page 35
	Epaper_Write_Data(0XF9-y_start2);    // RAM y address start at 0127h;
	Epaper_Write_Data(0XF9-y_start1);    // RAM y address start at 0127h;
	Epaper_Write_Data(0XF9-y_end2);    // RAM y address end at 00h;
	Epaper_Write_Data(0XF9-y_end1);  


  Epaper_Write_Command(0x4E);   // set RAM x address count to 0;
  Epaper_Write_Data(x_startB); 
  Epaper_Write_Command(0x4F);   // set RAM y address count to 0X127;    
  Epaper_Write_Data(y_start2);
  Epaper_Write_Data(y_start1);
  
  
  Epaper_Write_Command(0x24);   //Write Black and White image to RAM
  for(i=0;i<PART_COLUMN*PART_LINE/8;i++)
  {                         
      Epaper_Write_Data(*datasB);
      datasB++;
  }    
   
  //Data C//////////////////////////////////////
  x_startC=x_startC/8;//Convert to byte
	x_end=0X0F-(x_startC+PART_LINE/8-1); 
	x_startC=0X0F-x_startC;
  
  y_start1=0;
  y_start2=y_startC-1;
  if(y_startC>=256)
  {
      y_start1=y_start2/256;
      y_start2=y_start2%256;
  }
  y_end1=0;
  y_end2=y_startC+PART_COLUMN-1;
  if(y_end2>=256)
  {
      y_end1=y_end2/256;
      y_end2=y_end2%256;    
  }   
  
  Epaper_Write_Command(0x44);       // set RAM x address start/end, in page 35
  Epaper_Write_Data(x_startC);    // RAM x address start at 00h;
  Epaper_Write_Data(x_end);    // RAM x address end at 0fh(15+1)*8->128 
  Epaper_Write_Command(0x45);       // set RAM y address start/end, in page 35
	Epaper_Write_Data(0XF9-y_start2);    // RAM y address start at 0127h;
	Epaper_Write_Data(0XF9-y_start1);    // RAM y address start at 0127h;
	Epaper_Write_Data(0XF9-y_end2);    // RAM y address end at 00h;
	Epaper_Write_Data(0XF9-y_end1);  


  Epaper_Write_Command(0x4E);   // set RAM x address count to 0;
  Epaper_Write_Data(x_startC); 
  Epaper_Write_Command(0x4F);   // set RAM y address count to 0X127;    
  Epaper_Write_Data(y_start2);
  Epaper_Write_Data(y_start1);
  
  
  Epaper_Write_Command(0x24);   //Write Black and White image to RAM
  for(i=0;i<PART_COLUMN*PART_LINE/8;i++)
  {                         
      Epaper_Write_Data(*datasC);
      datasC++;
  }      
   
  //Data D//////////////////////////////////////
  x_startD=x_startD/8;//Convert to byte
	x_end=0X0F-(x_startD+PART_LINE/8-1); 
	x_startD=0X0F-x_startD;
  
  y_start1=0;
  y_start2=y_startD-1;
  if(y_startD>=256)
  {
      y_start1=y_start2/256;
      y_start2=y_start2%256;
  }
  y_end1=0;
  y_end2=y_startD+PART_COLUMN-1;
  if(y_end2>=256)
  {
      y_end1=y_end2/256;
      y_end2=y_end2%256;    
  }   
  
  Epaper_Write_Command(0x44);       // set RAM x address start/end, in page 35
  Epaper_Write_Data(x_startD);    // RAM x address start at 00h;
  Epaper_Write_Data(x_end);        // RAM x address end at 0fh(15+1)*8->128 
  Epaper_Write_Command(0x45);       // set RAM y address start/end, in page 35
	Epaper_Write_Data(0XF9-y_start2);    // RAM y address start at 0127h;
	Epaper_Write_Data(0XF9-y_start1);    // RAM y address start at 0127h;
	Epaper_Write_Data(0XF9-y_end2);    // RAM y address end at 00h;
	Epaper_Write_Data(0XF9-y_end1);   


  Epaper_Write_Command(0x4E);   // set RAM x address count to 0;
  Epaper_Write_Data(x_startD); 
  Epaper_Write_Command(0x4F);   // set RAM y address count to 0X127;    
  Epaper_Write_Data(y_start2);
  Epaper_Write_Data(y_start1);
  
  
  Epaper_Write_Command(0x24);   //Write Black and White image to RAM
  for(i=0;i<PART_COLUMN*PART_LINE/8;i++)
  {                         
      Epaper_Write_Data(*datasD);
      datasD++;
  } 
  //Data E//////////////////////////////////////
  x_startE=x_startE/8;//Convert to byte
	x_end=0X0F-(x_startE+PART_LINE/8-1); 
	x_startE=0X0F-x_startE;
  
  y_start1=0;
  y_start2=y_startE-1;
  if(y_startE>=256)
  {
      y_start1=y_start2/256;
      y_start2=y_start2%256;
  }
  y_end1=0;
  y_end2=y_startE+PART_COLUMN-1;
  if(y_end2>=256)
  {
      y_end1=y_end2/256;
      y_end2=y_end2%256;    
  }   
  
  Epaper_Write_Command(0x44);       // set RAM x address start/end, in page 35
  Epaper_Write_Data(x_startE);    // RAM x address start at 00h;
  Epaper_Write_Data(x_end);    // RAM x address end at 0fh(15+1)*8->128 
  Epaper_Write_Command(0x45);       // set RAM y address start/end, in page 35
	Epaper_Write_Data(0XF9-y_start2);    // RAM y address start at 0127h;
	Epaper_Write_Data(0XF9-y_start1);    // RAM y address start at 0127h;
	Epaper_Write_Data(0XF9-y_end2);    // RAM y address end at 00h;
	Epaper_Write_Data(0XF9-y_end1);   


  Epaper_Write_Command(0x4E);   // set RAM x address count to 0;
  Epaper_Write_Data(x_startE); 
  Epaper_Write_Command(0x4F);   // set RAM y address count to 0X127;    
  Epaper_Write_Data(y_start2);
  Epaper_Write_Data(y_start1);
  
  
  Epaper_Write_Command(0x24);   //Write Black and White image to RAM
  for(i=0;i<PART_COLUMN*PART_LINE/8;i++)
  {                         
      Epaper_Write_Data(*datasE);
      datasE++;
  }     
  EPD_Part_Update();

}

//////////////////////////////////END//////////////////////////////////////////////////
