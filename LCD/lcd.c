/*============================================================================================
 *
 *  (C) 2013, LDM-SYSTEMS
 *
 *  ���������������� ������ ��� ���������� ����� LDM-K1986BE92QI
 *
 *  ������ �� ��������������� "��� ����", �.�. ������������� ��� ������, ���������� ���������
 *  ������������� ���������� �� ���������� ��� ����������� Milandr K1986BE92QI. �������� LDM-SYSTEMS
 *  �� ����� ������� ��������������� �� ��������� ����������� ������������� �������, ���
 *  �������������� ������������� �� ��� ������, ��.
 *
 *  ������������ ��� Keil 5 DeamonClub.RU
 *
 *--------------------------------------------------------------------------------------------
 *
 *  ���� lcd.c: ������� ������� ������ ��� ������ � LCD-������� ���� MT-12864J
 *
 *============================================================================================*/

#include "MDR32Fx.h"                    // Device header
#include "lcd.h"

// �������� ���������� ��������� 
LCD_Crystal CurrentCrystal = LCD_CRYSTAL1;  // ������� ��������� �������
LCD_Method CurrentMethod;                   // ������� ����� ���������

// ����� LCD-���������� 
const LCD_Ports CrystalPorts[NUM_LCD_CRYSTALS] = 
{
                        // �������� #1 
                        { 0x18100000,   // LCD_DATA1 
                          0x10100000 }, // LCD_CMD1  
                        // �������� #2 
                        { 0x18200000,   // LCD_DATA2
                          0x10200000 }  // LCD_CMD2 
};


// ��������� ��������� � ������� 

static u32 GetStatus(void);
static void WaitStatus(LCD_Status status);
static void Pause(void);

#define WAIT_BUSY   WaitStatus(BUSY)
#define WAIT_RESET  WaitStatus(RESET)
#define WAIT_ON     WaitStatus(ONOFF)
#define READ_RW()   MDR_PORTB->RXTX |= (1<<10)
#define WRITE_RW()  MDR_PORTB->RXTX &= ~(1<<10)
#define SET_A0()    MDR_PORTC->RXTX |= (1<<0)
#define CLEAR_A0()  MDR_PORTC->RXTX &= ~(1<<0)
#define SET_E()     MDR_PORTC->RXTX |= (1<<1)
#define CLEAR_E()   MDR_PORTC->RXTX &= ~(1<<1)
#define DATA_LINE_READ()    MDR_PORTA->OE &= ~0x003F; MDR_PORTF->OE &= ~0x000C
#define DATA_LINE_WRITE()   MDR_PORTA->OE |= 0x003F; MDR_PORTF->OE |= 0x000C


void WRITE_DATA(u32 data)
{
  MDR_PORTA->RXTX |= (data & 0x003F);
  MDR_PORTA->RXTX &= ~(~data & 0x003F);
  MDR_PORTF->RXTX |= ((data>>4) & 0x000C);
  MDR_PORTF->RXTX &= ~(~(data>>4) & 0x000C);
}

u32 READ_DATA()
{
  u32 ret;

  ret = (MDR_PORTA->RXTX & 0x003F) + ((MDR_PORTF->RXTX & 0x000C) << 4);

  return ret;
}

// ������� ������ � LCD 

void ResetLCD(void) 
{
    u32 i;

    /*PORTC->RXTX = 0x00000200;
    PORTC->OE = 0x00000200;
    for (i = 0; i < 255; i++)
      PORTC->RXTX = 0x00000000;
    PORTC->RXTX = 0x00000200;*/

    MDR_PORTB->RXTX |= 0x00000200;
    MDR_PORTB->OE |= 0x00000200;
    for (i = 0; i < 500; i++)
      MDR_PORTB->RXTX &= ~0x00000200;
    MDR_PORTB->RXTX |= 0x00000200;
}


void InitPortLCD(void) {
		MDR_RST_CLK->PER_CLOCK = 0xFFFFFFFF;
	
    MDR_PORTA->FUNC &= ~(0x00000FFF);   // Main Function ��� DATA[5:0] 
    MDR_PORTA->ANALOG |= 0x003F;     		// Digital 
    MDR_PORTA->PWR |= 0x00000AAA;    		// Fast 
    MDR_PORTA->PULL |= 0x0000003F;    	// DOWN 

    MDR_PORTB->FUNC &= ~(0x003FC000);   // Main Function ��� E1=B7 & E2=B8 & Reset=B9 & RW=B10 
    MDR_PORTB->ANALOG |= 0x0780;     		// Digital 
    MDR_PORTB->PWR |= 0x002A8000;    		// Fast 
    MDR_PORTB->OE |= 0x0780;

    MDR_PORTC->FUNC &= ~(0x0000000F);   // Main Function ��� E=1 & A0=0
    MDR_PORTC->ANALOG |= 0x0003;     		// Digital 
    MDR_PORTC->PWR |= 0x0000000A;    		// Fast 
    MDR_PORTC->OE |= 0x0003;

    MDR_PORTF->FUNC &= ~(0x000000F0);   // Main Function ��� DATA[7:6]
    MDR_PORTF->ANALOG |= 0x000C;     		// Digital
    MDR_PORTF->PWR |= 0x000000A0;    		// Fast
    MDR_PORTF->PULL |= 0x0000000C;    	// DOWN

    READ_RW();
    CLEAR_A0();
    SET_E();

    DATA_LINE_READ();
}



void SetCrystal(LCD_Crystal num) 
{
   MDR_PORTB->RXTX |= ((num + 1) << 7);
   MDR_PORTB->RXTX &= ~(~((num + 1) << 7)&(0x180));	//0b110000000

   MDR_PORTB->OE |= 0x180;		//0b110000000
   Pause();
   CurrentCrystal = num;
}

void WriteLCD_Cmd(unsigned char val) 
{
    SET_E();
    Pause();

    DATA_LINE_WRITE();
    WRITE_RW();
    CLEAR_A0();

    WRITE_DATA(val);

    Pause();
    CLEAR_E();

    //LCD_CMD(CurrentCrystal) = val;
    Pause();
}


void WriteLCD_Data(unsigned char val) 
{
    SET_E();
    Pause();

    DATA_LINE_WRITE();
    WRITE_RW();
    SET_A0();

    WRITE_DATA(val);

    Pause();
    CLEAR_E();

    //LCD_DATA(CurrentCrystal) = val;
    Pause();
}


unsigned char ReadLCD_Cmd(void) 
{
    u32 ret;
    SET_E();
    Pause();

    DATA_LINE_READ();
    READ_RW();
    CLEAR_A0();

    Pause();
    CLEAR_E();

    ret = READ_DATA();

    //u32 ret = LCD_CMD(CurrentCrystal);
    Pause();
    return ret;
}


unsigned char ReadLCD_Data() 
{
    unsigned char ret;
    SET_E();
    Pause();


    DATA_LINE_READ();
    READ_RW();
    SET_A0();

    Pause();
    CLEAR_E();

    READ_DATA();  // ������ ������ - ���������� ��� ��������� ���������� ������ 

    Pause();
    CLEAR_E();
    Pause();
    SET_E();

    ret = READ_DATA();

    Pause();
    return ret;
}


void LCD_INIT(void) 
{
    u32 crystal;

    InitPortLCD();      // ������������� ������ ������� ���� � ������� ��� ������ � ������� 
    ResetLCD();         // ����������� ����� ������ 

    // ������������� ���� ���������� 
    for (crystal = LCD_CRYSTAL1; crystal < NUM_LCD_CRYSTALS; crystal++) 
		{
      SetCrystal((LCD_Crystal)crystal);
      WAIT_BUSY;
      LCD_ON;
      WAIT_ON;
      LCD_START_LINE(0);
    }
}


void LCD_CLS(void) 
{
    u32 i, j, crystal;

    // ������� ������ ��� ���� ���������� 
    for (crystal = LCD_CRYSTAL1; crystal < NUM_LCD_CRYSTALS; crystal++) 
		{
      SetCrystal((LCD_Crystal)crystal);
      WAIT_BUSY;
      LCD_OFF;
      LCD_SET_ADDRESS(0);
      for (i = 0; i < 8; i++) 
			{
        LCD_SET_PAGE(i);
        for (j = 0; j < 64; j++)
            WriteLCD_Data(0x00);
      }
      LCD_ON;
    }
}


// ��������� ������� 

static u32 GetStatus(void) 
{
    u32 ret;

    Pause();
    ret = ReadLCD_Cmd();
    Pause();
    return ret;
}


static void WaitStatus(LCD_Status status) {
        u32 stat;
    for (stat = GetStatus(); stat == (1 << status); stat = GetStatus());
}


// �������� ~ �� 15*3 ������ 
static void Pause(void) {
    u32 i;
    for (i = 15; i > 0; i--) __NOP();
}

/*============================================================================================
 * ����� ����� lcd.c
 *============================================================================================*/

