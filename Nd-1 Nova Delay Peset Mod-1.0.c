/*
 * Nd_1_Nova_Delay_Peset_Mod.c	Version 1.0a
 * Nova mod is a button preset pusher directly soldered to the preset button.
 * This code is two hande a 3 banks by 3 presets setup to set presets up on the nd1 (9 preset on nova nd-1)
 *
 * Created: 14/08/2012 5:05:56 PM
 *  Author: Dan Lopez
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include "util/delay.h"

int PresetTimerFlag = 0;			//Global Flag for Preset Timer (When 1 nd1 in preset mode. when 0 nd-1 in normal mode)

void Debounce(void)
{
		int TempReg = PIND;				//Get Input
		TempReg &= 0x03;				//Mask Input
		while (TempReg != 0)
						{
							TempReg = PIND;				//Get Input
							TempReg &= 0x03;				//Mask Input		
							_delay_loop_2(7000);
						}
						return;
}

void stopTimer(void)
{
	
	TCCR1B = 0;					//Stop Timer
	cli();						//Stop Interrupts
	
	TIMSK= 0;
	OCR1A = 0;	
	return;	
					
}

void IntTimer(void)
{
	stopTimer();
	PresetTimerFlag = 1;
	
	
	TCCR1B |= ( 1 << WGM12);	//Configure timer 1 for CTC mode
	TIMSK |= (1 << OCIE1A);		//Enable CTC Interrupt
	sei();						//Enable Global Interrupts 

	OCR1A = 0x0550;				// 3 Seconds @ 10MHz ?				TWEAK <<<<<<<<<<<<<<<<<<<<<<<<
	//OCR1A = 0x0570;				// 3 Seconds @ 10MHz ?
	TCCR1B|=0x05;				//Pre scaler set /1024
	return;

//	TCNT1=36239;		//3 Seconds @ 10mhz to 0xFF FF
//	TIMSK|=0x80;		//Enable TOIE1  (Overflow Interrupt)
	//sei;				//enable global interrupts
}

ISR(TIMER1_COMPA_vect)					//timer for preset mode (if in preset mode do not add extra push for wake up preset mode)
{
	stopTimer();
	PresetTimerFlag = 0;				//Clear Flag
	int TimeOutLoop = 30;
	PORTB |= 0x80;
	while (TimeOutLoop > 0)
	{
	_delay_loop_2(2000);			//TWEAK <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
	_delay_loop_2(10000);			//TWEAK <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
	TimeOutLoop--;	
	}
	PORTB &= 0x7F;
						
	
	return;
}



unsigned int PresetDiference(int CurrentPresest, int NewPreset)
{
	int Difference= 0;
	
	if (CurrentPresest < NewPreset)
	{
	Difference = NewPreset - CurrentPresest;
	return Difference;	
	}

	if (CurrentPresest > NewPreset)
	{
	Difference = 9-CurrentPresest+NewPreset;
	return Difference;	
	}

	if (CurrentPresest == NewPreset)
	{
	Difference = 0;
	return Difference;
	}
	
}

void RelayOut(int RlyNum)			// Out put pulse to relay
{
	stopTimer();
		if (PresetTimerFlag == 0)
		{
			RlyNum++;
		}

		while (RlyNum > 0 )
		{
			PORTB |= 0x40;
			_delay_loop_2(9000);
			PORTB &= 0xbf;
			_delay_loop_2(9000);
			RlyNum--;
		}	
		
		_delay_loop_2(9000);
		IntTimer();					//Start Preset mode timer		
		_delay_loop_2(9000);
		return;
}

unsigned int GetKeyPad(void)
	{
		unsigned int TempReg1;
		unsigned int Btn;
		
		Btn = 0;						//Clr
		
		PORTD = 0x04;					//Pattern 1
		TempReg1 = PIND;				//Get Input
		TempReg1 &= 0x03;				//Mask Input		
			
			switch(TempReg1)
				{
					case 0x01:
						stopTimer();
						Btn =1;
						Debounce();
						break;
				
					case 0x02:
						stopTimer();
						Btn =2;
						Debounce();
						break;
					
									}
			
			PORTD = 0x08;				//Pattern 2
			TempReg1 = PIND;				//Get Input
			TempReg1 &= 0x03;				//Mask Input		
			
				switch(TempReg1)
				{
					case 0x01:
						stopTimer();
						Btn =3;
						Debounce();
						break;
				
					case 0x02:
						Btn =4;
						Debounce();
						break;
								}
		
			PORTD = 0x10;				//Pattern 3
			TempReg1 = PIND;				//Get Input
			TempReg1 &= 0x03;				//Mask Input		
			
				switch(TempReg1)
				{
					case 0x01:
						Btn =5;
						Debounce();
						break;
				
					case 0x02:
						Btn =6;
						Debounce();
						break;
								}
		
		return Btn;
	}




int main(void)
{
	DDRB=0xff;		//Port B Set I/O to Out
	DDRD=0xfc;		//Port B Set 1111 1100 (1 are outputs)
		
		PORTB=0x00;		//CLr LEds
		_delay_loop_2(50000);
		PORTB=0xff;		//CLr LEds
		_delay_loop_2(50000);
		PORTB=0x09;		//Set Bank 1 P1
		
	IntTimer();
	while(PresetTimerFlag != 0)
	{
	_delay_loop_2(10000);	
	}
	
	unsigned int PrevPreset;			
	unsigned int KeyRtn;					//Button PRess Returned from GetKeyPad
	unsigned int Bank;						//Current BAnk
	unsigned int Preset;					//Current Preset
	unsigned int BankPresVal;				//USed to calculate actual pulse out for relay
	unsigned int BankOffsetVal;				//Offset Value
	unsigned int PresDiff;					//PResetDifference
	
	Bank=1;									//Set Up for Bank1 PReset 1
	PrevPreset=1;
	Preset=1;
	BankOffsetVal = 0;
	
	
	
    while(1)
    {

		
		KeyRtn = GetKeyPad();
		
		if (KeyRtn > 3)				//Bank Set
		{
		
			switch (KeyRtn)
			{
				
			
			case 4:
			PORTB |= 0x08;					//Set Bit
			PORTB &= 0x0F;					//Sync Other Leds (Turns off other bank leds)
			Bank = 1;						//Set Bank
			BankOffsetVal = 0;				//Set offser value
			break;
			
			case 5:
			PORTB |= 0x10;
			PORTB &= 0x17;
			Bank = 2;
			BankOffsetVal = 3;
			break;
			
			case 6:
			PORTB |= 0x20;
			PORTB &= 0x27;
			Bank = 3;
			BankOffsetVal = 6;
			break;
			}			
		}
				
	else
	{
		switch (KeyRtn)
			{
					
				
			case 1:

			PORTB |= 0x01;						//Set Preset led
			PORTB &= 0x39;						//Sync other leds (Turns off other preset leds)
			Preset =1 ;							//Set current preset
			
			BankPresVal = Preset+BankOffsetVal;	//Calculate Actual value for relay output
			PresDiff=PresetDiference(PrevPreset,BankPresVal);
			
			if (PrevPreset != BankPresVal)
			{
			RelayOut(PresDiff);				//OUtput to relay						
			}
						
			PrevPreset = BankPresVal;
			break;
			
			case 2:
			PORTB |= 0x02;
			PORTB &= 0x3A;
			Preset = 2;
						
			BankPresVal = Preset+BankOffsetVal;	//Calculate Actual value for relay output
			PresDiff=PresetDiference(PrevPreset,BankPresVal);
			
			if (PrevPreset != BankPresVal)
			{
			RelayOut(PresDiff);				//OUtput to relay						
			}
			
					
			PrevPreset = BankPresVal;
			break;

			
			case 3:
			PORTB |= 0x04;
			PORTB &= 0x3C;
			Preset = 3;
			
			BankPresVal = Preset+BankOffsetVal;	//Calculate Actual value for relay output
			PresDiff=PresetDiference(PrevPreset,BankPresVal);
			
			if (PrevPreset != BankPresVal)
			{
			RelayOut(PresDiff);				//OUtput to relay						
			}
			
			PrevPreset = BankPresVal;
			break;
			}		
	}
		
		
	
    }
}
