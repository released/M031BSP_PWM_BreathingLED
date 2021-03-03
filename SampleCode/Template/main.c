/*_____ I N C L U D E S ____________________________________________________*/
#include <stdio.h>
#include <string.h>
#include "NuMicro.h"

#include	"project_config.h"


/*_____ D E C L A R A T I O N S ____________________________________________*/

/*_____ D E F I N I T I O N S ______________________________________________*/

/*_____ M A C R O S ________________________________________________________*/




/*
	Target : 200K Freq
	DUTY : 50%
	
	SYS_CLK : 48M
	PSC : 1

	48 000 000/200 000 = PSC * (CNR + 1)
	CNR = (SYS_CLK/FREQ)/PSC - 1 = 239

	50 /100 = CMR / (CNR + 1)
	CMR = 50 * (CNR + 1)/100
	
*/

#define SYS_CLK 									(48000000ul)
#define PWM_PSC 								(48)	
#define PWM_FREQ 								(10000)	
#define PWM_DUTY                              	(0)

#define PWM_RESOLUTION                        	(100)

#define PWM_CHANNEL                           	(1)
#define PWM_CHANNEL_MASK                     (PWM_CH_1_MASK)

//16 bit
#define PWM_CNR 								((SYS_CLK/PWM_FREQ)/PWM_PSC - 1)
#define PWM_CMR 								(PWM_DUTY * (PWM_CNR + 1)/PWM_RESOLUTION)

#define CalNewDutyCMR(pwm, u32ChannelNum, u32DutyCycle, u32CycleResolution)	(u32DutyCycle * (PWM_GET_CNR(pwm, u32ChannelNum) + 1) / u32CycleResolution)
#define CalNewDuty(pwm, u32ChannelNum, u32DutyCycle, u32CycleResolution)		(PWM_SET_CMR(pwm, u32ChannelNum, CalNewDutyCMR(pwm, u32ChannelNum, u32DutyCycle, u32CycleResolution)))



volatile uint32_t BitFlag = 0;

uint32_t counter_tick = 0;

/*_____ F U N C T I O N S __________________________________________________*/

void tick_counter(void)
{
	counter_tick++;
}

uint32_t get_tick(void)
{
	return (counter_tick);
}

void set_tick(uint32_t t)
{
	counter_tick = t;
}

void compare_buffer(uint8_t *src, uint8_t *des, int nBytes)
{
    uint16_t i = 0;	
	
    for (i = 0; i < nBytes; i++)
    {
        if (src[i] != des[i])
        {
            printf("error idx : %4d : 0x%2X , 0x%2X\r\n", i , src[i],des[i]);
			set_flag(flag_error , ENABLE);
        }
    }

	if (!is_flag_set(flag_error))
	{
    	printf("%s finish \r\n" , __FUNCTION__);	
		set_flag(flag_error , DISABLE);
	}

}

void reset_buffer(void *dest, unsigned int val, unsigned int size)
{
    uint8_t *pu8Dest;
//    unsigned int i;
    
    pu8Dest = (uint8_t *)dest;

	#if 1
	while (size-- > 0)
		*pu8Dest++ = val;
	#else
	memset(pu8Dest, val, size * (sizeof(pu8Dest[0]) ));
	#endif
	
}

void copy_buffer(void *dest, void *src, unsigned int size)
{
    uint8_t *pu8Src, *pu8Dest;
    unsigned int i;
    
    pu8Dest = (uint8_t *)dest;
    pu8Src  = (uint8_t *)src;


	#if 0
	  while (size--)
	    *pu8Dest++ = *pu8Src++;
	#else
    for (i = 0; i < size; i++)
        pu8Dest[i] = pu8Src[i];
	#endif
}

void dump_buffer(uint8_t *pucBuff, int nBytes)
{
    uint16_t i = 0;
    
    printf("dump_buffer : %2d\r\n" , nBytes);    
    for (i = 0 ; i < nBytes ; i++)
    {
        printf("0x%2X," , pucBuff[i]);
        if ((i+1)%8 ==0)
        {
            printf("\r\n");
        }            
    }
    printf("\r\n\r\n");
}

void  dump_buffer_hex(uint8_t *pucBuff, int nBytes)
{
    int     nIdx, i;

    nIdx = 0;
    while (nBytes > 0)
    {
        printf("0x%04X  ", nIdx);
        for (i = 0; i < 16; i++)
            printf("%02X ", pucBuff[nIdx + i]);
        printf("  ");
        for (i = 0; i < 16; i++)
        {
            if ((pucBuff[nIdx + i] >= 0x20) && (pucBuff[nIdx + i] < 127))
                printf("%c", pucBuff[nIdx + i]);
            else
                printf(".");
            nBytes--;
        }
        nIdx += 16;
        printf("\n");
    }
    printf("\n");
}

void delay(uint16_t dly)
{
/*
	delay(100) : 14.84 us
	delay(200) : 29.37 us
	delay(300) : 43.97 us
	delay(400) : 58.5 us	
	delay(500) : 73.13 us	
	
	delay(1500) : 0.218 ms (218 us)
	delay(2000) : 0.291 ms (291 us)	
*/

	while( dly--);
}


void delay_ms(uint16_t ms)
{
	TIMER_Delay(TIMER0, 1000*ms);
}


void PWM_Set_Duty(PWM_T *pwm,uint32_t u32ChannelNum,uint32_t u32DutyCycle,uint32_t u32CycleResolution)		// 1 ~ 1000 , 0.1 % to 100%
{
    uint32_t u32NewCMR = 0;
	u32NewCMR = CalNewDutyCMR(pwm, u32ChannelNum, u32DutyCycle, u32CycleResolution);    
	PWM_SET_CMR(pwm, u32ChannelNum, u32NewCMR);
}


void PWMx_Init(void)
{
    /*
      Configure PWM0 channel 0 init period and duty(down counter type).
      Period is PLL / (prescaler * (CNR + 1))
      Duty ratio = CMR / (CNR + 1)
      
      Period = 48 MHz / (1 * (199 + 1)) = 240000 Hz
      Duty ratio = 100 / (199 + 1) = 50%
    */
	
    /* Set PWM0 timer clock prescaler */
    PWM_SET_PRESCALER(PWM1, PWM_CHANNEL, PWM_PSC - 1);
	
    /* Set up counter type */
    PWM1->CTL1 &= ~PWM_CTL1_CNTTYPE0_Msk;

    /* Set PWM0 timer period */
    PWM_SET_CNR(PWM1, PWM_CHANNEL, PWM_CNR);
	
    /* Set PWM0 timer duty */
    PWM_SET_CMR(PWM1, PWM_CHANNEL, PWM_CMR);	
	
    /* Set output level at zero, compare up, period(center) and compare down of specified channel */
    PWM_SET_OUTPUT_LEVEL(PWM1, PWM_CHANNEL_MASK, PWM_OUTPUT_HIGH, PWM_OUTPUT_LOW, PWM_OUTPUT_NOTHING, PWM_OUTPUT_NOTHING);
	
    /* Enable output of PWM0 channel 0 */
    PWM_EnableOutput(PWM1, PWM_CHANNEL_MASK);
	
	PWM_Start(PWM1, PWM_CHANNEL_MASK);
	
	set_flag(flag_reverse , ENABLE);
}


void GPIO_Init (void)
{
//    GPIO_SetMode(PB, BIT14, GPIO_MODE_OUTPUT);	// use PWM to drive LED

    GPIO_SetMode(PB, BIT15, GPIO_MODE_OUTPUT);	
}


void TMR1_IRQHandler(void)
{
	static uint32_t LOG = 0;
	static uint16_t duty = 0;	// 1 ~ 1000 , 0.1 % to 100%
	
    if(TIMER_GetIntFlag(TIMER1) == 1)
    {
        TIMER_ClearIntFlag(TIMER1);
		tick_counter();

		if ((get_tick() % 1000) == 0)
		{
        	printf("%s : %4d\r\n",__FUNCTION__,LOG++);
//			PB14 ^= 1;
		}

		#if 1	// resolution : 100 , 15ms change duty , total duty change timing from 0 ~ 100 % (or 100% ~ 0%) will be 15*100 = 1500 ms
		if ((get_tick() % 15) == 0)		// target : 1.5 sec (duty from 0 to 100) , 1.5 sec (duty from 100 to 0)
		{

//			PWM_Set_Duty(PWM1, PWM_CHANNEL, duty, PWM_RESOLUTION);
			CalNewDuty(PWM1, PWM_CHANNEL, duty, PWM_RESOLUTION);

			if (is_flag_set(flag_reverse))
			{
				duty++;	
			}
			else
			{
				duty--;
			}

			if (duty == PWM_RESOLUTION)
			{
				set_flag(flag_reverse , DISABLE);				
			}
			else if (duty == 0)
			{
				set_flag(flag_reverse , ENABLE);
			}
		}
		#else	// resolution : 1000 , 2ms change duty , total duty change timing from 0 ~ 100 % (or 100% ~ 0%) will be 2*1000 = 2000 ms
		if ((get_tick() % 2) == 0)		// target : 2 sec (duty from 0 to 1000) , 2 sec (duty from 1000 to 0)
		{

//			PWM_Set_Duty(PWM1, PWM_CHANNEL, duty, 1000);
			CalNewDuty(PWM1, PWM_CHANNEL, duty, 1000);

			if (is_flag_set(flag_reverse))
			{
				duty++;	
			}
			else
			{
				duty--;
			}

			if (duty == 1000)
			{
				set_flag(flag_reverse , DISABLE);				
			}
			else if (duty == 0)
			{
				set_flag(flag_reverse , ENABLE);
			}
		}	


		#endif
    }
}


void TIMER1_Init(void)
{
    TIMER_Open(TIMER1, TIMER_PERIODIC_MODE, 1000);
    TIMER_EnableInt(TIMER1);
    NVIC_EnableIRQ(TMR1_IRQn);	
    TIMER_Start(TIMER1);
}

void UARTx_Process(void)
{
	uint8_t res = 0;
	res = UART_READ(UART0);

	if (res == 'x' || res == 'X')
	{
		NVIC_SystemReset();
	}

	if (res > 0x7F)
	{
		printf("invalid command\r\n");
	}
	else
	{
		switch(res)
		{
			case '1':
				break;

			case 'X':
			case 'x':
			case 'Z':
			case 'z':
				NVIC_SystemReset();		
				break;
		}
	}
}

void UART02_IRQHandler(void)
{

    if(UART_GET_INT_FLAG(UART0, UART_INTSTS_RDAINT_Msk | UART_INTSTS_RXTOINT_Msk))     /* UART receive data available flag */
    {
        while(UART_GET_RX_EMPTY(UART0) == 0)
        {
            UARTx_Process();
        }
    }

    if(UART0->FIFOSTS & (UART_FIFOSTS_BIF_Msk | UART_FIFOSTS_FEF_Msk | UART_FIFOSTS_PEF_Msk | UART_FIFOSTS_RXOVIF_Msk))
    {
        UART_ClearIntFlag(UART0, (UART_INTSTS_RLSINT_Msk| UART_INTSTS_BUFERRINT_Msk));
    }	
}

void UART0_Init(void)
{
    SYS_ResetModule(UART0_RST);

    /* Configure UART0 and set UART0 baud rate */
    UART_Open(UART0, 115200);
    UART_EnableInt(UART0, UART_INTEN_RDAIEN_Msk | UART_INTEN_RXTOIEN_Msk);
    NVIC_EnableIRQ(UART02_IRQn);
	
	#if (_debug_log_UART_ == 1)	//debug
	printf("\r\nCLK_GetCPUFreq : %8d\r\n",CLK_GetCPUFreq());
	printf("CLK_GetHXTFreq : %8d\r\n",CLK_GetHXTFreq());
	printf("CLK_GetLXTFreq : %8d\r\n",CLK_GetLXTFreq());	
	printf("CLK_GetPCLK0Freq : %8d\r\n",CLK_GetPCLK0Freq());
	printf("CLK_GetPCLK1Freq : %8d\r\n",CLK_GetPCLK1Freq());	
	#endif	

}

void SYS_Init(void)
{
    /* Unlock protected registers */
    SYS_UnlockReg();

    /* Enable HIRC clock (Internal RC 48MHz) */
    CLK_EnableXtalRC(CLK_PWRCTL_HIRCEN_Msk|CLK_PWRCTL_HXTEN_Msk);

    /* Wait for HIRC clock ready */
    CLK_WaitClockReady(CLK_STATUS_HIRCSTB_Msk|CLK_STATUS_HXTSTB_Msk);

    /* Enable HIRC clock (Internal RC 48MHz) */
    CLK_EnableXtalRC(CLK_PWRCTL_LIRCEN_Msk|CLK_PWRCTL_LXTEN_Msk);

    /* Wait for HIRC clock ready */
    CLK_WaitClockReady(CLK_STATUS_LIRCSTB_Msk|CLK_STATUS_LXTSTB_Msk);	

    /* Select HCLK clock source as HIRC and HCLK source divider as 1 */
    CLK_SetHCLK(CLK_CLKSEL0_HCLKSEL_HIRC, CLK_CLKDIV0_HCLK(1));

    CLK_EnableModuleClock(UART0_MODULE);
    CLK_SetModuleClock(UART0_MODULE, CLK_CLKSEL1_UART0SEL_HIRC, CLK_CLKDIV0_UART0(1));

    CLK_EnableModuleClock(TMR1_MODULE);
  	CLK_SetModuleClock(TMR1_MODULE, CLK_CLKSEL1_TMR1SEL_HIRC, 0);

    CLK_EnableModuleClock(PWM1_MODULE);
    CLK_SetModuleClock(PWM1_MODULE, CLK_CLKSEL2_PWM1SEL_PCLK1, 0);

    SYS->GPB_MFPH = (SYS->GPB_MFPH & (~SYS_GPB_MFPH_PB14MFP_Msk)) | SYS_GPB_MFPH_PB14MFP_PWM1_CH1;	

	
    /* Set PB multi-function pins for UART0 RXD=PB.12 and TXD=PB.13 */
    SYS->GPB_MFPH = (SYS->GPB_MFPH & ~(SYS_GPB_MFPH_PB12MFP_Msk | SYS_GPB_MFPH_PB13MFP_Msk)) |
                    (SYS_GPB_MFPH_PB12MFP_UART0_RXD | SYS_GPB_MFPH_PB13MFP_UART0_TXD);

   /* Update System Core Clock */
    SystemCoreClockUpdate();

    /* Lock protected registers */
    SYS_LockReg();
}

/*
 * This is a template project for M031 series MCU. Users could based on this project to create their
 * own application without worry about the IAR/Keil project settings.
 *
 * This template application uses external crystal as HCLK source and configures UART0 to print out
 * "Hello World", users may need to do extra system configuration based on their system design.
 */

int main()
{
    SYS_Init();

	UART0_Init();
	GPIO_Init();

	PWMx_Init();
	
	TIMER1_Init();

    /* Got no where to go, just loop forever */
    while(1)
    {


    }
}

/*** (C) COPYRIGHT 2017 Nuvoton Technology Corp. ***/
