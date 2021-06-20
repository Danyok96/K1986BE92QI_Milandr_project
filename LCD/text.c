/*============================================================================================
 *
 *  (C) 2013, LDM-SYSTEMS
 *
 *  Äåìîíñòðàöèîííûé ïðîåêò äëÿ îòëàäî÷íîé ïëàòû LDM-K1986BE92QI
 *
 *  Äàííîå ÏÎ ïðåäîñòàâëÿåòñÿ "ÊÀÊ ÅÑÒÜ", ò.å. èñêëþ÷èòåëüíî êàê ïðèìåð, ïðèçâàííûé îáëåã÷èòü
 *  ïîëüçîâàòåëÿì ðàçðàáîòêó èõ ïðèëîæåíèé äëÿ ïðîöåññîðîâ Milandr K1986BE92QI. Êîìïàíèÿ LDM-SYSTEMS
 *  íå íåñåò íèêàêîé îòâåòñòâåííîñòè çà âîçìîæíûå ïîñëåäñòâèÿ èñïîëüçîâàíèÿ äàííîãî, èëè
 *  ðàçðàáîòàííîãî ïîëüçîâàòåëåì íà åãî îñíîâå, ÏÎ.
 *
 *  Àäàïòèðîâàíî ïîä Keil 5 DeamonClub.RU
 *
 *--------------------------------------------------------------------------------------------
 *
 *  Ôàéë text.c: Âûâîä ñèìâîëîâ è òåêñòà íà ýêðàí
 *
 *============================================================================================*/

#include <string.h>

#include "lcd.h"
#include "text.h"

#include "systick.h"


// Âûáðàííûé øðèôò äëÿ îòðèñîâêè òåêñòà 
FONT *CurrentFont;

// Âûâîä áàéòà íà ýêðàí 
void LCD_PUT_BYTE(u8 x, u8 y, u8 data) 
{
    u32 tmp_data, page, shift_num, shift_num_ex;

    if ((x>MAX_X)|(y>MAX_Y)) return;

    // Âûáîð êðèñòàëëà è ñìåùåíèå ïî õ 
    SetCrystal((LCD_Crystal)(x/64));
    x %= 64;
    // Îïðåäåëÿåì ïåðâóþ ñòðàíèöó è ñìåùåíèå ïî y 
    page = y/8;
    shift_num = y%8;
    shift_num_ex = 8 - shift_num;

    // Ïåðâàÿ ñòðàíèöà 

    // ×èòàåì òåêóùåå çíà÷åíèå
    LCD_SET_PAGE(page);
    LCD_SET_ADDRESS(x);
    tmp_data = ReadLCD_Data();
    // Çàïèñûâàåì ìîäèôèöèðîâàííîå çíà÷åíèå
    LCD_SET_PAGE(page);
    LCD_SET_ADDRESS(x);
    switch (CurrentMethod){
        case MET_OR:
                WriteLCD_Data(tmp_data | (data << shift_num));
                break;
        case MET_XOR:
                WriteLCD_Data(tmp_data ^ (data << shift_num));
                break;
        case MET_NOT_OR:
                WriteLCD_Data(tmp_data | ((data ^ 0xFF) << shift_num));
                break;
        case MET_NOT_XOR:
                WriteLCD_Data(tmp_data ^ ((data ^ 0xFF) << shift_num));
                break;
        case MET_AND:
                WriteLCD_Data(tmp_data&(0xFF >> shift_num_ex) | (data << shift_num));
                break;
    }

    // Âòîðàÿ ñòðàíèöà (åñëè åñòü) 
    if (shift_num > 0) {
        // ×èòàåì òåêóùåå çíà÷åíèå
        LCD_SET_PAGE(page+1);
        LCD_SET_ADDRESS(x);
        tmp_data = ReadLCD_Data();
        // Çàïèñûâàåì ìîäèôèöèðîâàííîå çíà÷åíèå
        LCD_SET_PAGE(page+1);
        LCD_SET_ADDRESS(x);
        switch(CurrentMethod){
            case MET_OR:
                WriteLCD_Data(tmp_data | (data >> shift_num_ex));
                break;
            case MET_XOR:
                WriteLCD_Data(tmp_data ^ (data >> shift_num_ex));
                break;
            case MET_NOT_OR:
                WriteLCD_Data(tmp_data | ((data ^ 0xFF) >> shift_num_ex));
                break;
            case MET_NOT_XOR:
                WriteLCD_Data(tmp_data ^ ((data ^ 0xFF)>> shift_num_ex));
                break;
            case MET_AND:
                WriteLCD_Data(tmp_data & (0xFF << shift_num) | (data >> shift_num_ex));
                break;
        }
    }
}

// Âûâîä ñèìâîëîâ è ñòðîê òåêóùèì øðèôòîì 

void LCD_PUTC(u8 x, u8 y, u8 ch) 
{
    u32 i, j, line;
    unsigned char *sym;

    sym = Get_Char_Data_Addr(ch);
    line = CurrentFont->Height / 8;
    if (CurrentFont->Height % 8)
        line++;

    for (j = 0; j < line; j++)
        for( i = 0; i < CurrentFont->Width; i++)
            LCD_PUT_BYTE(x + i, y + j*8, sym[i + CurrentFont->Width*j]);
}


void LCD_PUTS(u8 x, u8 y, const char* str) 
{
    u32 i;
    for (i=0; str[i]; i++)
        LCD_PUTC(x + i*CurrentFont->Width, y, str[i]);
}


void LCD_PUTS_Ex(u8 x, u8 y, const char* str, u8 style) 
{
    u32 i;
    LCD_Method OldMethod = CurrentMethod;

    switch (style) {
        // Ïðîñòàÿ ñòðîêà 
        case StyleSimple:
            CurrentMethod = MET_AND;
            LCD_PUTS(x, y, str);
            break;
        // Ìåðöàþùàÿ ñòðîêà 
        case StyleBlink:
            CurrentMethod = MET_AND;
            LCD_PUTS(x, y, str);
            SysTickDelay(600);
            for (i = 0; i < strlen(str); i++)
                LCD_PUTC(x + ((CurrentFont->Width) * i), y, 0x20);
            break;
        // Ñòðîêà ñ èçìåíåíèåì ôîíà 
        case StyleFlipFlop:
            CurrentMethod = MET_AND;
            LCD_PUTS(x, y, str);
            CurrentMethod = MET_XOR;
            LCD_PUTS(x, y, str);
            CurrentMethod = MET_NOT_XOR;
            LCD_PUTS(x, y, str);
            SysTickDelay(1200);
            CurrentMethod = MET_AND;
            LCD_PUTS(x, y, str);
            SysTickDelay(150);
            break;
        // Äðîæàùàÿ ñòðîêà 
        case StyleVibratory:
            CurrentMethod = MET_AND;
            LCD_PUTS(x, y, str);
            SysTickDelay(600);
            LCD_PUTS(x+1, y+1, str);
            break;
    }
    CurrentMethod = OldMethod;
}

/*============================================================================================
 * Êîíåö ôàéëà text.c
 *============================================================================================*/

