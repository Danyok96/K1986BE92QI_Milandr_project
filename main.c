/* dz Mihailov-Ovsyannikov DS (Danyok)*/

#include <MDR32F9Qx_port.h> 
#include <MDR32F9Qx_uart.h> 
#include <MDR32F9Qx_rst_clk.h> 
#include <MDR32F9Qx_timer.h> 
#include "lcd.h" 
#include "text.h" 
#include "font_defs.h" 

#define UART_BAUNDRATE 19200 
#define UART_WORDLENGTH UART_WordLength8b 
#define UART_STOPBITS UART_StopBits1 
#define TIMER_FREQUENCY 7000 
#define INTERRUPT_DELAY 6000 

// Data segment 
const char STR_MYNAME[] = "Mihailov-Ovsyannikov D S"; 
const char STR_MYGROUP[] = "RL1-114"; 
const char STR_FREQUENCY[] = "56 MHz"; 
const char STR_DELAY_FREQUENCY[] = "6 sec"; 
const char STR_UART_SPEED[] = "19 200 bit/sec"; 

const char MESSAGE[] = "56 MHz\n\r"; 

int i, g_interrupt_counter = 0; 

#define DELAY(T) for (i = T; i > 0; i--)

void LCD_Init() 
{      
	LCD_INIT();
    CurrentFont = &Font_6x8;
    CurrentMethod = MET_AND; 
} 

void LCD_Write() 
{      
	char str_interrupt_counter[32];      
	sprintf(str_interrupt_counter, "%d", g_interrupt_counter);      
	LCD_CLS();      
	LCD_PUTS(0, 0, STR_MYNAME);      
	LCD_PUTS(0, 8, STR_MYGROUP);      
	LCD_PUTS(0, 17, STR_FREQUENCY);      
	LCD_PUTS(0, 26, STR_DELAY_FREQUENCY);      
	LCD_PUTS(0, 35, STR_UART_SPEED);      
	LCD_PUTS(0, 44, str_interrupt_counter); 
} 

// Use of external quartz 
void CLOCK_Setup(int A) 
{      
	MDR_RST_CLK->HS_CONTROL = 0x1 ;                 //переключение в режим внешнего кварца      
	while (MDR_RST_CLK->CLOCK_STATUS == 0x00) __NOP();  //ожидание стабилизации кварца      
	MDR_RST_CLK->CPU_CLOCK = 0x02;           //частота равна частоте внешнего кварца (8 МГц) 
	MDR_RST_CLK->PLL_CONTROL |=((A/8)-1)*0x0100;		//установка множителя PLL=7 cpu clk 56Mhz
	MDR_RST_CLK->PLL_CONTROL |= 0x04;              //Включение PLL для CPU      
	while (!(MDR_RST_CLK->CLOCK_STATUS & 0x02)) __NOP();  //ожидание стабилизации кварца      
	MDR_RST_CLK->CPU_CLOCK = 0x00000106;        //частота микроконтроллера 56 МГц      
	MDR_RST_CLK->TIM_CLOCK = 0x01000000;         //TIM1 enable 
} 

// Initialization of ports for UART and diodes 
void PORTSInit() 
{      
	PORT_InitTypeDef SETUP;      
	RST_CLK_PCLKcmd(RST_CLK_PCLK_PORTF, ENABLE);      
	PORT_StructInit(&SETUP);      
	SETUP.PORT_SPEED = PORT_SPEED_MAXFAST;      
	SETUP.PORT_FUNC = PORT_FUNC_OVERRID;      
	SETUP.PORT_MODE = PORT_MODE_DIGITAL;      
	SETUP.PORT_Pin = PORT_Pin_1;      
	SETUP.PORT_OE = PORT_OE_OUT;      
	PORT_Init(MDR_PORTF,&SETUP);      
	SETUP.PORT_Pin = PORT_Pin_0;      
	SETUP.PORT_OE = PORT_OE_IN;      
	PORT_Init(MDR_PORTF,&SETUP);      
	RST_CLK_PCLKcmd(RST_CLK_PCLK_PORTB, ENABLE);      
	PORT_InitTypeDef SETUP1;      
	SETUP1.PORT_Pin = PORT_Pin_0|PORT_Pin_1|PORT_Pin_2|PORT_Pin_3;      
	SETUP1.PORT_OE = PORT_OE_OUT;      
	SETUP1.PORT_FUNC = PORT_FUNC_PORT;      
	SETUP1.PORT_MODE = PORT_MODE_DIGITAL;      
	SETUP1.PORT_SPEED = PORT_SPEED_SLOW; 
  PORT_Init(MDR_PORTB,&SETUP1); 
} 

// Настройка UART на заданную скорость работы 
void UARTInit() 
{      
	RST_CLK_PCLKcmd(RST_CLK_PCLK_UART2, ENABLE);      
	UART_InitTypeDef UART_InitStructure;      
	UART_BRGInit(MDR_UART2, UART_HCLKdiv1);      
	UART_InitStructure.UART_BaudRate = UART_BAUNDRATE;     
	UART_InitStructure.UART_WordLength = UART_WORDLENGTH;      
	UART_InitStructure.UART_StopBits = UART_STOPBITS;      
	UART_InitStructure.UART_Parity = UART_Parity_No;        
	UART_InitStructure.UART_FIFOMode = UART_FIFO_OFF;      
	UART_InitStructure.UART_HardwareFlowControl = UART_HardwareFlowControl_RXE | UART_HardwareFlowControl_TXE;      
	//DELAY(6000);      
	UART_Init(MDR_UART2, &UART_InitStructure);      
	UART_Cmd(MDR_UART2, ENABLE);   
	} 

// Blinking LED 
void LED() 
{      
	static uint8_t counter = 0;      
	if (counter++ % 2)           
		PORT_SetBits(MDR_PORTB, PORT_Pin_0);      
	else           
		PORT_ResetBits(MDR_PORTB, PORT_Pin_0); 
} 

// Timer initialization 
TIMER_CntInitTypeDef TIM1Init;

void TIMER_Init() 
{      
	TIMER_CntInitTypeDef TIM1Init;      
	RST_CLK_PCLKcmd(RST_CLK_PCLK_TIMER1, ENABLE);      
	TIMER_CntStructInit(&TIM1Init);      
	TIMER_BRGInit(MDR_TIMER1, TIMER_HCLKdiv8);      
	TIM1Init.TIMER_Prescaler = TIMER_FREQUENCY;      
	TIM1Init.TIMER_Period = INTERRUPT_DELAY;      
	TIMER_CntInit(MDR_TIMER1, &TIM1Init);      
	NVIC_EnableIRQ(Timer1_IRQn);      
	NVIC_SetPriority(Timer1_IRQn, 1);      
	TIMER_ITConfig(MDR_TIMER1, TIMER_STATUS_CNT_ZERO, ENABLE);      
	TIMER_Cmd(MDR_TIMER1, ENABLE); 
	} 

	bool irFlag = 0;
	// Interrupt 
	void Timer1_IRQHandler() 
	{      
		if (TIMER_GetITStatus(MDR_TIMER1, TIMER_STATUS_CNT_ZERO))     
		{           
			g_interrupt_counter++;
			irFlag = 1;
			// LED(); 

			// LCD_Init();           
			// LCD_Write(); 

			// uint8_t c;    

			// for (c = 0; c < sizeof(MESSAGE) - 1; c++)           
			// {
			//     DELAY(INTERRUPT_DELAY);                
			// 	UART_SendData(MDR_UART2, MESSAGE[c]);                      
			// }
			TIMER_ClearITPendingBit(MDR_TIMER1, TIMER_STATUS_CNT_ZERO);      
		} 
	} 

	int main() 
	{      
		CLOCK_Setup(56);      
		PORTSInit();      
		TIMER_Init();     
		UARTInit();    

		while (1)      
		{
			if (irFlag == 1)
			{
				LED();
				LCD_Write();
				
			uint8_t c;    

			for (c = 0; c < sizeof(MESSAGE) - 1; c++)           
			{
			    DELAY(INTERRUPT_DELAY);                
				UART_SendData(MDR_UART2, MESSAGE[c]);                      
			}
			irFlag = 0;
			}
		}     
	}
