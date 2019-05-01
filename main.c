/*
 * AtmelStudio project
 *
 * Created: 30.04.2019 14:07:43
 * Author : majki
 */ 

/*
 * main.c
 *
 *  Created on: 27 wrz 2014
 *      Author: majki
 *
 *      - najszybszy interwal (Canon 30D) to 2 sekundy
 */

#define F_CPU 16000000UL  //definiujemy F_CPU na 16MHz  
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include "LCD_Nokia5110/pcd8544.h"
// you came
#define PRZ1	(1<<PD0)
#define PRZ2	(1<<PD1)
#define PRZ3	(1<<PD3)
#define PRZ4	(1<<PD2)
#define FOCUS	(1<<PC2)
#define SHUTTER	(1<<PC3)
#define MOTOR_CL	(1<<PD6)
#define MOTOR_CCL	(1<<PD5)

//void po_przycisku(void);
void delay_ms8(uint8_t);
void delay_ms16(uint16_t);
void mega328p_config();
void PCD_labels_print();

const uint8_t camera_sleep_time=60;
//uint8_t	abc=126;
uint16_t exposure_time=100, shot_interval=120;
uint16_t trolley_move_time=300;
uint16_t shots_number=60, shots_taken;
uint16_t liczba_sekund;
uint16_t temp;
volatile uint8_t setne_sek_mcu, sek_mcu, sek_mcu_flaga;
//volatile uint16_t setne_pomiar;

enum keys_status {RELEASED, PRESSED};
enum menu_state {CONFIG_SHOTS_NUMBER, CONFIG_INTERVAL, CONFIG_MOVE_TIME, SHOOTING};
enum motor_state {CLOCKWISE, COUNTER_CLOCKWISE, STOP};

struct
{
	uint8_t	state;
} focus;

struct
{
	uint8_t	state;
} shutter;

struct
{
	uint8_t state;
} menu;

struct
{
	uint8_t dzien;
	uint8_t godzina;
	uint8_t minuta;
	uint8_t sekunda;
} czas;

struct
{
	uint8_t state;
} motor;

int main(void)
{
	mega328p_config();

    PCD_Ini();  // inicjacja LCD

    PCD_Contr(0x40);  // kontrast
    PCD_Clr();
    PCD_Upd();

//    PCD_GotoXYFont(1,6);
//	PCD_Chr(FONT_1X,abc+34);
//	PCD_Upd();

	PCD_labels_print();

	sei();

    while(1)
    {
    	if(menu.state == CONFIG_SHOTS_NUMBER || menu.state == CONFIG_INTERVAL || menu.state == CONFIG_MOVE_TIME)
		{
			PCD_GotoXYFont(10,1);
			if(shots_number<100)
				PCD_FStr(FONT_1X,(unsigned char*)PSTR("0"));
			if(shots_number<10)
				PCD_FStr(FONT_1X,(unsigned char*)PSTR("0"));
			PCD_Int(FONT_1X,shots_number);
			
//			PCD_FStr(FONT_1X,(unsigned char*)PSTR("  "));		// kasowanie pozostalosci za wartoscia

			PCD_GotoXYFont(10,2);
			PCD_Int(FONT_1X,shot_interval);
			PCD_FStr(FONT_1X,(unsigned char*)PSTR("s "));		// kasowanie pozostalosci za wartoscia

			PCD_GotoXYFont(10,3);
			PCD_Int(FONT_1X,trolley_move_time);

			PCD_Upd();
		}

//    	if(menu.state == CONFIG_INTERVAL)
//    	{
//
//    	}

    	if(menu.state == SHOOTING && (sek_mcu%shot_interval == 0) && (shots_taken < shots_number))
		{
    		// budzenie aparatu, focusowanie przez krotki czas
    		if(shot_interval > camera_sleep_time)
    		{
    			PORTC	|=	FOCUS;
//    			focus.state	=	PINC & FOCUS;
    			_delay_ms(50);
    			PORTC	&=	~(FOCUS);
//    			focus.state	=	PINC & FOCUS;
    			_delay_ms(100);
    		}

			PCD_GotoXYFont(1,6);
		    PCD_FStr(FONT_1X,(unsigned char*)PSTR("A"));
			PCD_Upd();

			PORTC	|=	FOCUS;
			focus.state	=	PINC & FOCUS;
//			focus.state	=	PRESSED;
			_delay_ms(50);

			PCD_GotoXYFont(1,6);
		    PCD_FStr(FONT_1X,(unsigned char*)PSTR(" "));
			PCD_Upd();


			PCD_GotoXYFont(2,6);
		    PCD_FStr(FONT_1X,(unsigned char*)PSTR("S"));
			PCD_Upd();

			PORTC	|=	SHUTTER;
			shutter.state	=	PINC & SHUTTER;
//			shutter.state	=	PRESSED;
			delay_ms16(exposure_time);								//czas ekspozycji, zmienione przy nowym toolchaine z AtmelStudio

			PCD_GotoXYFont(2,6);
		    PCD_FStr(FONT_1X,(unsigned char*)PSTR(" "));
			PCD_Upd();

			// Release Focusa i Shuttera
			PCD_GotoXYFont(3,6);
		    PCD_FStr(FONT_1X,(unsigned char*)PSTR("R"));
			PCD_Upd();

			PORTC	&=	~(SHUTTER);
			PORTC	&=	~(FOCUS);
			focus.state	=	PINC & FOCUS;
			shutter.state	=	PINC & SHUTTER;

			PCD_GotoXYFont(3,6);
		    PCD_FStr(FONT_1X,(unsigned char*)PSTR(" "));
			PCD_Upd();

			_delay_ms(200);											//czas miêdzy releasem a ruchem wózka (na ustabilizowanie siê wózka i aparatu)

			// Zmiana po³o¿enia wózka
			PCD_GotoXYFont(4,6);
		    PCD_FStr(FONT_1X,(unsigned char*)PSTR("M"));
			PCD_Upd();


			PORTD	&=	~(MOTOR_CL);
			motor.state	=	PIND & MOTOR_CL;
			delay_ms16(trolley_move_time);

			PORTD	|=	MOTOR_CL;
			motor.state	=	PIND & MOTOR_CL;


			PCD_GotoXYFont(4,6);
		    PCD_FStr(FONT_1X,(unsigned char*)PSTR(" "));
			PCD_Upd();


			shots_taken++;

			_delay_ms(1000);		//czas na gotowosc aparatu (po to, zeby poczekac do konca zerowej wartosci mcu_sek)
//			PCD_Clr();
//			PCD_Upd();
		}


		if(!(PIND & PRZ1))
		{
			_delay_ms(80);
			while(!(PIND & PRZ1)) {}

	    	if(menu.state > CONFIG_SHOTS_NUMBER)
			{
	    		PCD_GotoXYFont(1,menu.state+1);
	    		PCD_FStr(FONT_1X,(unsigned char*)PSTR(" "));
	    		PCD_Upd();

	    		menu.state--;

	    		PCD_GotoXYFont(1,menu.state+1);
	    		PCD_FStr(FONT_1X,(unsigned char*)PSTR(">"));
	    		PCD_Upd();
			}

			_delay_ms(80);
		}

		if(!(PIND & PRZ2))
		{
			_delay_ms(80);
			while(!(PIND & PRZ2)) {}


	    	if(menu.state == CONFIG_SHOTS_NUMBER)
			{
	    		if(shots_number > 60)
	    			shots_number -= 30;
	    		else if (shots_number > 20)
	    			shots_number -= 10;
	    		else
	    			shots_number--;
			}

	    	if(menu.state == CONFIG_INTERVAL)
			{
	    		if(shot_interval > 2)
	    			shot_interval--;
			}

	    	if(menu.state == CONFIG_MOVE_TIME && trolley_move_time > 300)
	    	{
	    		trolley_move_time -= 100;
	    	}

			_delay_ms(80);
		}

		if(!(PIND & PRZ3))
		{
			while(!(PIND & PRZ3)) {}	//jesli jeszcze wcisniety
//			PCD_GotoXYFont(5,5);
//		    PCD_FStr(FONT_1X,(unsigned char*)PSTR(" "));
//			PCD_Int(FONT_1X,setne_pomiar*10);
//		    PCD_FStr(FONT_1X,(unsigned char*)PSTR(" "));
//			PCD_Upd();

	    	if(menu.state == CONFIG_SHOTS_NUMBER && shots_number < (999-30))
			{
	    		if(shots_number >= 60)
	    			shots_number += 30;
	    		else if (shots_number >= 20)
	    			shots_number += 10;
	    		else
	    			shots_number++;
			}

	    	if(menu.state == CONFIG_INTERVAL)
			{
	    		shot_interval++;
			}

	    	if(menu.state == CONFIG_MOVE_TIME && trolley_move_time < 900)
	    	{
	    			trolley_move_time += 100;
	    	}

			_delay_ms(80);
		}

		if(!(PIND & PRZ4))
		{
			_delay_ms(80);
			while(!(PIND & PRZ4)) {}

	    	if(menu.state < SHOOTING)
			{
	    		PCD_GotoXYFont(1,menu.state+1);
	    		PCD_FStr(FONT_1X,(unsigned char*)PSTR(" "));
	    		PCD_Upd();

	    		menu.state++;

	    		PCD_GotoXYFont(1,menu.state+1);
	    		PCD_FStr(FONT_1X,(unsigned char*)PSTR(">"));
	    		PCD_Upd();

	    		if(menu.state == SHOOTING)
	    			sek_mcu	=	0;
			}

	    	/* pomiar napiecia zasilania i wyswietlanie wartosci
	    	ADCSRA	|=	(1<<ADSC);		//start pomiaru
	    	while (ADCSRA & (1<<ADSC));	//czekaj na koniec pomiaru
	    	PCD_GotoXYFont(2,1);
	    	PCD_Int(FONT_1X,ADCW);
	    	PCD_Upd();
*/
			_delay_ms(80);
		}

		if(sek_mcu_flaga)
		{
			// DEBUG

			// czas do konca sesji
			/*
			liczba_sekund	=	shot_count*shot_interval;
			czas.dzien		=	liczba_sekund	/ 3600*24;		//	czas.dzien		=	liczba_sekund	/ (60 * 60 * 24);
			liczba_sekund	-=	czas.dzien		* 3600*24;
			czas.godzina	=	liczba_sekund	/ (60 * 60);
			liczba_sekund	-=	czas.godzina	* (60 * 60);
			czas.minuta		=	liczba_sekund	/ 60;
			*/
			liczba_sekund	=	(shots_number-1)*shot_interval;

			if((menu.state == SHOOTING) && (shots_taken < shots_number))
			{
				liczba_sekund	-=	shots_taken*shot_interval - (shot_interval-sek_mcu%shot_interval);
			}

			czas.godzina	=	liczba_sekund / 3600;
			temp			=	liczba_sekund % 3600;
			czas.minuta		=	temp / 60;
			czas.sekunda	=	temp % 60;
//			PCD_GotoXYFont(1,5);
//			PCD_Int(FONT_1X,liczba_sekund);
//    		PCD_FStr(FONT_1X,(unsigned char*)PSTR("."));
//			PCD_Int(FONT_1X,temp);

			PCD_GotoXYFont(2,5);
//			PCD_Int(FONT_1X,czas.dzien);					//dni
//    		PCD_FStr(FONT_1X,(unsigned char*)PSTR(":"));
			if(czas.godzina<10)
	    		PCD_FStr(FONT_1X,(unsigned char*)PSTR("0"));
			PCD_Int(FONT_1X,czas.godzina);					//godziny
    		PCD_FStr(FONT_1X,(unsigned char*)PSTR(":"));
			if(czas.minuta<10)
	    		PCD_FStr(FONT_1X,(unsigned char*)PSTR("0"));
			PCD_Int(FONT_1X,czas.minuta);					//minuty
    		PCD_FStr(FONT_1X,(unsigned char*)PSTR(":"));
			if(czas.sekunda<10)
	    		PCD_FStr(FONT_1X,(unsigned char*)PSTR("0"));
			PCD_Int(FONT_1X,czas.sekunda);					//sekundy

/*
			PCD_GotoXYFont(6,6);
			if(sek_mcu<10)
	    		PCD_FStr(FONT_1X,(unsigned char*)PSTR("0"));
			PCD_Int(FONT_1X,sek_mcu);
//    		PCD_FStr(FONT_1X,(unsigned char*)PSTR("  "));
 */

    		if((menu.state == SHOOTING) && (shots_taken < shots_number))
    		{
				PCD_GotoXYFont(12,4);
				if(shot_interval-sek_mcu%shot_interval<10)
		    		PCD_FStr(FONT_1X,(unsigned char*)PSTR("0"));
				PCD_Int(FONT_1X,shot_interval-sek_mcu%shot_interval);	//pozostalo sekund do nastepnego shoot'a
//				PCD_FStr(FONT_1X,(unsigned char*)PSTR("  "));
    		}

			PCD_GotoXYFont(12,5);
			if(sek_mcu%shot_interval<10)
	    		PCD_FStr(FONT_1X,(unsigned char*)PSTR("0"));
			PCD_Int(FONT_1X,sek_mcu%shot_interval);

			PCD_GotoXYFont(8,6);
			if(shots_taken<100)
	    		PCD_FStr(FONT_1X,(unsigned char*)PSTR("0"));
			if(shots_taken<10)
	    		PCD_FStr(FONT_1X,(unsigned char*)PSTR("0"));
			PCD_Int(FONT_1X,shots_taken);

			PCD_GotoXYFont(11,6);
    		PCD_FStr(FONT_1X,(unsigned char*)PSTR("/"));
			if(shots_number<100)
	    		PCD_FStr(FONT_1X,(unsigned char*)PSTR("0"));
			if(shots_number<10)
	    		PCD_FStr(FONT_1X,(unsigned char*)PSTR("0"));
			PCD_Int(FONT_1X,shots_number);

			PCD_Upd();
			// DEBUG

			sek_mcu_flaga	=	0;
		}
    }
}

//void po_przycisku(void)
//{
//	lcd_locate(3,15);
//	lcd_int(OCR1A);
//	lcd_str("  ");
//	odczyt_AV02();

//	min_max_procent(dataA_ADC1);
//}

void delay_ms8(uint8_t time_ms)
{
	for(uint8_t i=0; i<time_ms; i++)
		_delay_ms(1);
}

void delay_ms16(uint16_t time_ms)
{
	for(uint16_t i=0; i<time_ms; i++)
	_delay_ms(1);
}

void mega328p_config()
{	
	DDRC	|=	FOCUS | SHUTTER;		// kierunek jako WYJŒCIE
	PORTC	&=	~(FOCUS | SHUTTER);
	DDRD	&=	~(PRZ1 | PRZ2 | PRZ3 | PRZ4);	// kierunek przycisków jako WEJŒCIE
	PORTD	|=	PRZ1 | PRZ2 | PRZ3 | PRZ4 | MOTOR_CL | MOTOR_CCL;		// podci¹gniêcie do VCC
	DDRD	|=	MOTOR_CL | MOTOR_CCL;			//kierunek jako WYJŒCIE


	//			KONFIGURACJA TIMER0 DO PRACY JAKO TIMER CTC
	TCCR0A	|= (1<<WGM01);				//tryb CTC
	TCCR0B	|= (1<<CS02);				//preskaler 256
//	TCCR0B	|= (1<<CS02)|(1<<CS00);		//preskaler 1024
	//	OCR0	= 255;						//dla f~=...
	OCR0A	= 249;						//dla f=125Hz (8ms) przy preskaler 256
//	OCR0A	= 155;						//dla f~=50Hz przy preskaler 1024
	//		OCR0A	= 108;						//dla f~=100Hz
	//	OCR0	=	36;						//dla f~=200Hz
	//	OCR0	=	18;						//dla f~=400Hz
	//	OCR0	=	9;						//dla f~=800Hz
	TIMSK0	|= (1<<OCIE0A);				//zezwolenie na przerwanie Compare Match

/*
	//			KONFIGURACJA TIMER1 DO PRACY JAKO CTC
	TCCR1A	|=	(1<<WGM12);				//tryb CTC
	TCCR1B	|=	(1<<CS11)|(1<<CS10);	//preskaler 64
//	TCCR1B	|=	(1<<CS12);				//preskaler 256
//	TCCR1B	|=	(1<<CS12)|(1<<CS10);	//preskaler 1024
	OCR1A	=	3124;					//dla f=40Hz (25ms) dla preskaler 64
//	OCR1A	=	3124;					//dla f=10Hz (100ms) dla preskaler 256
//	OCR1A	=	7812;					//dla f~=0.999936Hz (1000.064019ms) dla preskaler 1024
	TIMSK1	|=	(1<<OCIE1A);			//zezwolenie na przerwanie Compare Match
*/
	//		ADMUX	|=	(1<<REFS0)|(1<<REFS1);
	//		ADCSRA	|=	(1<<ADEN)|(1<<ADPS1)|(1<<ADPS2);
}

void PCD_labels_print()
{
	// drukowanie stalych etykiet
	PCD_GotoXYFont(2,1);
	PCD_FStr(FONT_1X,(unsigned char*)PSTR("Liczba:"));
	PCD_GotoXYFont(2,2);
	PCD_FStr(FONT_1X,(unsigned char*)PSTR("Interw:"));
	PCD_GotoXYFont(2,3);
	PCD_FStr(FONT_1X,(unsigned char*)PSTR("Skok:"));
	PCD_GotoXYFont(2,4);
	PCD_FStr(FONT_1X,(unsigned char*)PSTR("SHOOTING!"));
	PCD_GotoXYFont(1,menu.state+1);
	PCD_FStr(FONT_1X,(unsigned char*)PSTR(">"));
	PCD_Upd();
}

ISR(TIMER0_COMPA_vect)
//ISR(TIMER1_COMPA_vect)
{
	setne_sek_mcu++;
//	setne_pomiar++;
	if(setne_sek_mcu > 249)				//przy TIMER0 = CTC, preskaler 256, OCR0A = 249 i tutaj '> 249' mamy prawie równo 1s
//	if(setne_sek_mcu > 200)				//pierwotnie bylo 100, ale f byla 2 razy za duza :)
	{
		setne_sek_mcu = 0;
		sek_mcu++;
		sek_mcu_flaga	=	1;
//		uart_puts(".");
//		uart_putint(wlaczony_przez_ir,10);
//		uart_puts(".");
//		uart_putint(sek_mcu,10);
	}
//	dzielnik_f++;
}
