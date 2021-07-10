/*
 * Nd_1_Nova_Delay_Peset_Mod.c	Version 1.1b
 * Nova mod is a button preset pusher directly soldered to the preset button.
 * This code is two handel a 3 banks by 3 presets setup to set presets up on the nd1 (9 preset on nova nd-1)
 *
 * Created: 14/08/2012 5:05:56 PM
 *  Author: Dan Lopez
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include "util/delay.h"

volatile int PresetTimerFlag = 0;			//Global Flag for Preset Timer (When 1 nd1 in preset mode. when 0 nd-1 in normal mode)
volatile int NewBankLed = 0;				//New bank led (bank leds only)
volatile int NewBankLedMask = 0xff;			//Mask out for other leds for new led setting (Bank leds only)
volatile int CurrentBankLed = 0x08;			//Current selected bank led (Bank leds only)


volatile int PresetBtnLockFlag = 0;		// Preset Buttons Lock when 1
volatile int Bank;						//Current BAnk
volatile int BankOffsetVal;				//Offset Value


void PresetBtnLock(void)						//These function holds the avr in here untill the nova delay settles back to non preset mode (runs between before the end of 3 seconf prset mode to normal mdoe)
{
		int TimeOutLoop = 30;
			PORTB |= 0x80;						//Initiate hold led				
			while (TimeOutLoop > 0)
			{
					_delay_loop_2(8000);			//TWEAK <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
				TimeOutLoop--;	
			}
			PORTB &= 0x7F;						//Turn off hold led
			PresetBtnLockFlag =0;
}

void Debounce(void)
{
		int TempReg = PIND;				//Get Input
		TempReg &= 0x77;				//Mask Input
		
		while (TempReg != 0)
						{
						TempReg = PIND;				//Get Input
						TempReg &= 0x77;				//Mask Input
						}
		return;
}

void stopTimer(void)
{
	TCCR1B = 0;					//Stop Timer
	TIMSK= 0;
	return;				
}

void IntTimer(void)
{
	stopTimer();
	PresetTimerFlag = 1;
	
	
	TCCR1B|=0x05;				//Pre scaler set /1024			<<Tweek  (First 3 secondish before return 2 normal mode (not preset mode)
	TCNT1 = 0xfb8f;				//Init Timer
	TIMSK |= ( 1 << TOIE1);		//TIMSK |= 0x08;				//TOIE 1 (COunter Overflow Int)
	


	return;

}

ISR(SIG_INT1)
{
	
	
	int tempreg1=0;
	
	tempreg1 = PIND;			//Get Button
	tempreg1 &= 0x70;			//Mask only bank buttons
	
	switch(tempreg1)
	{
		case 0x10:

			NewBankLed = 0x08;
			NewBankLedMask = 0xcf;			

			Bank = 1;						//Set Bank
			BankOffsetVal = 0;				//Set offset value
		break;
		
		case 0x20:
			NewBankLed = 0x10;
			NewBankLedMask = 0xD7;

			Bank = 2;
			BankOffsetVal = 3;
		break;
		
		case 0x40:
			NewBankLed = 0x20;
			NewBankLedMask = 0xE7;	

			Bank = 3;
			BankOffsetVal = 6;
		break;
	}
	
	PORTB |= NewBankLed;
	PORTB &= NewBankLedMask;
	Debounce();
	return;
}

//ISR(TIMER1_COMPA_vect)					//timer for preset mode (if in preset mode do not add extra push for wake up preset mode)
ISR(TIMER1_OVF_vect)
{
	stopTimer();
	PresetTimerFlag = 0;				//Clear Flag
	PresetBtnLockFlag = 1;				//Lock PReset Btns
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
		
		PORTB |= NewBankLed;
		PORTB &= NewBankLedMask;
		CurrentBankLed = NewBankLed;
		
						
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
		

		IntTimer();					//Start Preset mode timer			
		return;
}

unsigned int GetPreset(void)
	{
		unsigned int TempReg1;
		unsigned int Btn;
		
		Btn = 0;						//Clr
		TempReg1 = PIND;				//Get Button
		TempReg1 &= 0x07;				//Mask out only preset in
			
				switch(TempReg1)		
					{
						
					case 0x01:
//					stopTimer();
					Btn = 1;
					
					break;
					
					case 0x02:
//					stopTimer();
					Btn = 2 ;
					break;
					
					case 0x04:
//					stopTimer();
					Btn= 3 ;
					break;
					
					}				
		
					Debounce();
					return Btn;			
	}




int main(void)
{
	DDRB=0xff;		//Port B Set I/O to Out
	DDRD=0x00;		//Port D Set 0000 0000 (1 are outputs) set for inputs
		
	
//	PORTB=0xDf;		//CLr LEds
		
		
	sei();	
	IntTimer();
	
	while(PresetTimerFlag != 0)
	{
	
	}
	
	PORTB=0x09;		//Set Bank 1 P1
	
	
	MCUCR |= 0x0C;	// ISC11 & ISC set ( INT1 trigger rissing edge
	GIMSK |= 0x80;	// INT1 Enabled To Recive interups inputs
	
	
	unsigned int PrevPreset;			
	unsigned int Preset;					//Current Preset
	unsigned int BankPresVal;				//USed to calculate actual pulse out for relay
	unsigned int PresDiff;					//PResetDifference
	

	
	Bank=1;									//Set Up for Bank1 PReset 1
	PrevPreset=1;
	Preset=1;
	BankOffsetVal = 0;
	PresetBtnLockFlag = 0;
	
	
	
    while(1)
    {

		//NewBankLedMask |= CurrentBankLed;
		//PORTB &= NewBankLedMask;
	
		if (CurrentBankLed != NewBankLed)
		{
		PORTB ^= NewBankLed;	
		}
		
		
		
		
		 
	if (PresetBtnLockFlag == 1)
		{
			PresetBtnLock();		
		}
		
			switch (GetPreset())
			{
					
			case 0:
			break;
			
			case 1:
			PORTB |= 0x01;						//Set Preset led
			Preset =1 ;							//Set current preset
			PORTB &= 0xf9;
			CurrentBankLed = NewBankLed;
						
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
			Preset = 2;
			PORTB &= 0xfA;
			CurrentBankLed = NewBankLed;
									
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
			Preset = 3;
			PORTB &= 0xfC;
			CurrentBankLed = NewBankLed;
			BankPresVal = Preset+BankOffsetVal;	//Calculate Actual value for relay output
			PresDiff=PresetDiference(PrevPreset,BankPresVal);
			
			if (PrevPreset != BankPresVal)
				{
					RelayOut(PresDiff);				//OUtput to relay						
				}
			
			PrevPreset = BankPresVal;
			break;
			}		
		
	_delay_loop_2(30000);
	//PORTB |= NewBankLed;
	//_delay_loop_2(10000);
 
}
}
