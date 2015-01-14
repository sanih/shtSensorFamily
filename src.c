/*****************************************************
This program was produced by the
CodeWizardAVR V2.05.0 Professional
Automatic Program Generator
� Copyright 1998-2010 Pavel Haiduc, HP InfoTech s.r.l.
http://www.hpinfotech.com

Project : 
Version : 
Date    : 01/11/2015
Author  : Naeem Aleahmadi
Company : OulumRobotics
Comments: this is based on sample code bye SENSIRION


Chip type               : ATmega16
Program type            : Application
AVR Core Clock frequency: 8.000000 MHz
Memory model            : Small
External RAM size       : 0
Data Stack size         : 256
*****************************************************/

#include <mega16.h>
// Standard Input/Output functions
#include <stdio.h>
//must be included to use delay functions
#include <delay.h>
//must be included to use itoa ftoa functions
#include <stdlib.h >
//must be included to use log10 functions
#include <math.h >


// Alphanumeric LCD Module functions
#include <alcd.h>



typedef union
{ unsigned int i;
  float f;
} value;
enum {TEMP,HUMI};
#define        DATA_OUT           PORTC.0
#define        DATA_IN            PINC.0
#define        SCK                PORTC.1
#define noACK 0
#define ACK   1       

#define led PORTB.0
							//adr  command  r/w
#define STATUS_REG_W 0x06   //000   0011    0
#define STATUS_REG_R 0x07   //000   0011    1
#define MEASURE_TEMP 0x03   //000   0001    1
#define MEASURE_HUMI 0x05   //000   0010    1
#define RESET        0x1e   //000   1111    0
// Declare your global variables here
unsigned int tempervalue[2]={0,0};
/***********************************/
char SHT_WriteByte(unsigned char bytte)
{
  unsigned char i,error=0; 
  DDRC = 0b00000011;    //
  for (i=0x80;i>0;i/=2) //shift bit for masking
  {
	if (i & bytte)      
	DATA_OUT=1; //masking value with i , write to SENSI-BUS
	else DATA_OUT=0;                      
	SCK=1;      //clk for SENSI-BUS
	delay_us(5); //pulswith approx. 5 us         
	SCK=0;      
  }
  DATA_OUT=1;            //release dataline               
  DDRC = 0b00000010;    // DATA is Output
  SCK=1;                //clk #9 for ack
  delay_us(2);
  error=DATA_IN;       //check ack (DATA will be pulled down by SHT11)
  delay_us(2);
  SCK=0;       
  return error;       //error=1 in case of no acknowledge
}
//----------------------------------------------------------------------------------
// reads a byte form the Sensibus and gives an acknowledge in case of "ack=1"
//----------------------------------------------------------------------------------
char SHT_ReadByte(unsigned char ack)
{
  unsigned char i,val=0;
  DDRC = 0b00000010;    // DATA is Input
  for (i=0x80;i>0;i/=2)             //shift bit for masking
  { SCK=1;                          //clk for SENSI-BUS      
	delay_us(2);
	if (DATA_IN) val=(val | i);        //read bit 
	delay_us(2);
	SCK=0;
  }  
  DDRC = 0b00000011;    // DATA is Output
  DATA_OUT=!ack;        //in case of "ack==1" pull down DATA-Line
  SCK=1;                //clk #9 for ack
  delay_us(5);          //pulswith approx. 5 us
  SCK=0;
  DATA_OUT=1;           //release DATA-line  //ADD BY LUBING                                                              
  return val;
}
//----------------------------------------------------------------------------------
// generates a transmission start
//       _____         ________
// DATA:      |_______|
//           ___     ___
// SCK : ___|   |___|   |______
//----------------------------------------------------------------------------------
void SHT_Transstart(void)
{                       
   DDRC = 0b00000011;    // DATA is Output
   DATA_OUT=1; SCK=0;   //Initial state
   delay_us(2);
   SCK=1;
   delay_us(2);
   DATA_OUT=0;
   delay_us(2);
   SCK=0; 
   delay_us(5);
   SCK=1;
   delay_us(2);
   DATA_OUT=1;                  
   delay_us(2);
   SCK=0;     
   DDRC = 0b00000010;    // DATA is Input
}
//----------------------------------------------------------------------------------
// communication reset: DATA-line=1 and at least 9 SCK cycles followed by transstart
//       _____________________________________________________         ________
// DATA:                                                      |_______|
//          _    _    _    _    _    _    _    _    _        ___     ___
// SCK : __| |__| |__| |__| |__| |__| |__| |__| |__| |______|   |___|   |______
//----------------------------------------------------------------------------------
void SHT_ConnectionRest(void)
{ 
  unsigned char i;
  DDRC = 0b00000011;    // DATA is output
  DATA_OUT=1; SCK=0;                    //Initial state
  for(i=0;i<9;i++)                  //9 SCK cycles
  { SCK=1;
	delay_us(1);
	SCK=0;
	delay_us(1);
  }
  SHT_Transstart();                   //transmission start
  DDRC = 0b00000010;    // DATA is Input
}
//----------------------------------------------------------------------------------
// resets the sensor by a softreset
//----------------------------------------------------------------------------------
char SHT_SoftRst(void)
{
  unsigned char error=0; 
  SHT_ConnectionRest();              //reset communication
  error+=SHT_WriteByte(RESET);       //send RESET-command to sensor
  return error;                     //error=1 in case of no response form the sensor
}
//----------------------------------------------------------------------------------
// reads the status register with checksum (8-bit)
//----------------------------------------------------------------------------------
char SHT_Read_StatusReg(unsigned char *p_value, unsigned char *p_checksum)
{
  unsigned char error=0;
  SHT_Transstart();                   //transmission start
  error=SHT_WriteByte(STATUS_REG_R); //send command to sensor
  *p_value=SHT_ReadByte(ACK);        //read status register (8-bit)
  *p_checksum=SHT_ReadByte(noACK);   //read checksum (8-bit) 
  return error;                     //error=1 in case of no response form the sensor
}
//----------------------------------------------------------------------------------
// writes the status register with checksum (8-bit)
//----------------------------------------------------------------------------------
char SHT_Write_StatusReg(unsigned char *p_value)
{
  unsigned char error=0;
  SHT_Transstart();                   //transmission start
  error+=SHT_WriteByte(STATUS_REG_W);//send command to sensor
  error+=SHT_WriteByte(*p_value);    //send value of status register
  return error;                     //error>=1 in case of no response form the sensor
}  
//----------------------------------------------------------------------------------
// makes a measurement (humidity/temperature) with checksum
//----------------------------------------------------------------------------------
char SHT_Measure(unsigned char *p_value, unsigned char *p_checksum, unsigned char mode)
{
  unsigned error=0;
 unsigned int temp=0;
  SHT_Transstart();                   //transmission start   
  switch(mode){                     //send command to sensor
	case TEMP        : error+=SHT_WriteByte(MEASURE_TEMP); break;
	case HUMI        : error+=SHT_WriteByte(MEASURE_HUMI); break;
	default     : break;        
  }                 
  DDRC = 0b00000010;    // DATA is input
 while (1)
 {       
   if(DATA_IN==0) break; //wait until sensor has finished the measurement
  }
  if(DATA_IN) error+=1;                // or timeout (~2 sec.) is reached    
  switch(mode){                     //send command to sensor
	case TEMP        : temp=0;
					   temp=SHT_ReadByte(ACK);
					   temp<<=8;
					   tempervalue[0]=temp;
					   temp=0;
					   temp=SHT_ReadByte(ACK);
					   tempervalue[0]|=temp;
						break;
	case HUMI        : temp=0;
					   temp=SHT_ReadByte(ACK);
					   temp<<=8;
					   tempervalue[1]=temp;
					   temp=0;
					   temp=SHT_ReadByte(ACK);
					   tempervalue[1]|=temp;
						break;
	default     : break;        
  }  
  *p_checksum =SHT_ReadByte(noACK);  //read checksum    
  return error;
}
//----------------------------------------------------------------------------------------
// calculates temperature [?] and humidity [%RH]
// input :  humi [Ticks] (12 bit)
//          temp [Ticks] (14 bit)
// output:  humi [%RH]
//          temp [?]
//----------------------------------------------------------------------------------------

  const float C1=-4.0;              // for 12 Bit
  const float C2=+0.0405;           // for 12 Bit
  const float C3=-0.0000028;        // for 12 Bit
  const float T1=+0.01;             // for 14 Bit @ 5V
  const float T2=+0.00008;           // for 14 Bit @ 5V       

float Calc_SHT71(float p_humidity ,float *p_temperature)
{
  float rh_lin;                     // rh_lin:  Humidity linear
	float rh_true;                    // rh_true: Temperature compensated humidity
  float t=*p_temperature;           // t:       Temperature [Ticks] 14 Bit
  float rh=p_humidity;             // rh:      Humidity [Ticks] 12 Bit
  float t_C;                        // t_C   :  Temperature [?]
  t_C=t*0.01 - 40;                  //calc. temperature from ticks to [?]
  rh_lin=C3*rh*rh + C2*rh + C1;     //calc. humidity from ticks to [%RH]
  rh_true=(t_C-25)*(T1+T2*rh)+rh_lin;   //calc. temperature compensated humidity [%RH]
  if(rh_true>100)rh_true=100;       //cut if the value is outside of
  if(rh_true<0.1)rh_true=0.1;       //the physical possible range
  *p_temperature=t_C;               //return temperature [?]
   return rh_true;
}
//--------------------------------------------------------------------
float Calc_dewpoint(float h,float t)
//--------------------------------------------------------------------
// calculates dew point
// input:   humidity [%RH], temperature [?]
// output:  dew point [?]
{ float logEx,dew_point;
  logEx=0.66077+7.5*t/(237.3+t)+(log10(h)-2);
  dew_point = (logEx - 0.66077)*237.3/(0.66077+7.5-logEx);
  return dew_point;
}    

void loading(int loop)
	{   int i=0;
		for (i=0;i<=loop;i++)
			{
				led=1;
				lcd_clear();
				lcd_putsf("SHT 11 oulum");
				delay_ms(500);
				led=0;
				delay_ms(500);
			}
	}


// Declare your global variables here

void main(void)
{
// Declare your local variables here
value humi_val,temp_val;
  unsigned char error,checksum;
  float dewpoint=0.0;
  //unsigned char outp;
  //unsigned int humi;     
  char inp;
  unsigned char Humidity[5]={0,0,0,0,0};
  unsigned char Temperture[5]={0,0,0,0,0};
  unsigned char Depoint[5]={0,0,0,0,0};

// Input/Output Ports initialization
// Port A initialization
// Func7=In Func6=In Func5=In Func4=In Func3=In Func2=In Func1=In Func0=In 
// State7=T State6=T State5=T State4=T State3=T State2=T State1=T State0=T 
PORTA=0x00;
DDRA=0x00;

// Port B initialization
// Func7=In Func6=In Func5=In Func4=In Func3=In Func2=In Func1=In Func0=Out 
// State7=T State6=T State5=T State4=T State3=T State2=T State1=T State0=0 
PORTB=0x00;
DDRB=0x01;

// Port C initialization
// Func7=In Func6=In Func5=In Func4=In Func3=In Func2=In Func1=In Func0=In 
// State7=T State6=T State5=T State4=T State3=T State2=T State1=T State0=T 
PORTC=0x00;
DDRC=0x00;

// Port D initialization
// Func7=In Func6=In Func5=In Func4=In Func3=In Func2=In Func1=In Func0=In 
// State7=T State6=T State5=T State4=T State3=T State2=T State1=T State0=T 
PORTD=0x00;
DDRD=0x00;

// Timer/Counter 0 initialization
// Clock source: System Clock
// Clock value: Timer 0 Stopped
// Mode: Normal top=0xFF
// OC0 output: Disconnected
TCCR0=0x00;
TCNT0=0x00;
OCR0=0x00;

// Timer/Counter 1 initialization
// Clock source: System Clock
// Clock value: Timer1 Stopped
// Mode: Normal top=0xFFFF
// OC1A output: Discon.
// OC1B output: Discon.
// Noise Canceler: Off
// Input Capture on Falling Edge
// Timer1 Overflow Interrupt: Off
// Input Capture Interrupt: Off
// Compare A Match Interrupt: Off
// Compare B Match Interrupt: Off
TCCR1A=0x00;
TCCR1B=0x00;
TCNT1H=0x00;
TCNT1L=0x00;
ICR1H=0x00;
ICR1L=0x00;
OCR1AH=0x00;
OCR1AL=0x00;
OCR1BH=0x00;
OCR1BL=0x00;

// Timer/Counter 2 initialization
// Clock source: System Clock
// Clock value: Timer2 Stopped
// Mode: Normal top=0xFF
// OC2 output: Disconnected
ASSR=0x00;
TCCR2=0x00;
TCNT2=0x00;
OCR2=0x00;

// External Interrupt(s) initialization
// INT0: Off
// INT1: Off
// INT2: Off
MCUCR=0x00;
MCUCSR=0x00;

// Timer(s)/Counter(s) Interrupt(s) initialization
TIMSK=0x00;

// USART initialization
// USART disabled
UCSRB=0x00;

// Analog Comparator initialization
// Analog Comparator: Off
// Analog Comparator Input Capture by Timer/Counter 1: Off
ACSR=0x80;
SFIOR=0x00;

// ADC initialization
// ADC disabled
ADCSRA=0x00;

// SPI initialization
// SPI disabled
SPCR=0x00;

// TWI initialization
// TWI disabled
TWCR=0x00;

// Alphanumeric LCD initialization
// Connections specified in the
// Project|Configure|C Compiler|Libraries|Alphanumeric LCD menu:
// RS - PORTA Bit 0
// RD - PORTA Bit 1
// EN - PORTA Bit 2
// D4 - PORTA Bit 4
// D5 - PORTA Bit 5
// D6 - PORTA Bit 6
// D7 - PORTA Bit 7
// Characters/line: 16
lcd_init(16);
loading(1);

SHT_ConnectionRest();

  while (1)
	  {

	  // Place your code here
	  delay_ms(1000);       
	  led=1;
	  error=0;           
	  error+=SHT_Measure((unsigned char*)( &humi_val.i),&checksum,HUMI);  //measure humidity
	  error+=SHT_Measure((unsigned char*) (&temp_val.i),&checksum,TEMP);  //measure temperature              
	  error += SHT_Read_StatusReg(&inp, &checksum);
	  led=0;
	  if(error!=0)
		   {
			   SHT_ConnectionRest();                 //in case of an error: connection reset
			   putsf("Error");  
		   }      

	 else
		   {
			   humi_val.f=(float)tempervalue[1];                   //converts integer to float
			   temp_val.f=(float)tempervalue[0];                   //converts integer to float
			   humi_val.f=Calc_SHT71(humi_val.f,&temp_val.f);      //calculate humidity, temperature
			   dewpoint=Calc_dewpoint(humi_val.f,temp_val.f) ;
			   ftoa(humi_val.f,2,Humidity);


			   lcd_clear();
			   lcd_gotoxy(0,0);
			   lcd_putsf("H: ");
			   lcd_puts(Humidity);
			   lcd_gotoxy(0,1);
			   lcd_putsf("T: ");
			   ftoa(temp_val.f,2,Temperture);
			   lcd_puts(Temperture);

			   /*putsf(" Humidity: ");                         
			   puts(Humidity);
			   ftoa(temp_val.f,2,Temperture);
			   putsf(" Temperature: ");
			   puts(Temperture);
			   ftoa(dewpoint,2,Depoint);
			   putsf(" dewpoint: ");
			   puts(Depoint);*/
		   }       
	  };
}
