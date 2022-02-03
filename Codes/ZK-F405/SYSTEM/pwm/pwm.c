#include "sys.h"
#include "delay.h"
#include "pwm.h"






//��ʱ����pwm��ʼ��
void TIM3_PWM_Init(void)
{ 	GPIO_InitTypeDef GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	TIM_OCInitTypeDef  TIM_OCInitStructure;
	
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3,ENABLE);  
	
	TIM_DeInit(TIM3);
	
	GPIO_PinAFConfig(GPIOC,GPIO_PinSource6,GPIO_AF_TIM3);//PC6 ����ΪTIM3 CH1	MOTOR1
	GPIO_PinAFConfig(GPIOC,GPIO_PinSource7,GPIO_AF_TIM3);//PC7 ����ΪTIM3 CH2	MOTOR2
	GPIO_PinAFConfig(GPIOC,GPIO_PinSource8,GPIO_AF_TIM3);//PC7 ����ΪTIM3 CH3	MOTOR3
	GPIO_PinAFConfig(GPIOC,GPIO_PinSource9,GPIO_AF_TIM3);//PC9 ����ΪTIM3 CH4	MOTOR4
	
	GPIO_InitStructure.GPIO_Pin=GPIO_Pin_6|GPIO_Pin_7|GPIO_Pin_8|GPIO_Pin_9;	
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;		
	GPIO_InitStructure.GPIO_OType=GPIO_OType_PP;      
	GPIO_InitStructure.GPIO_PuPd=GPIO_PuPd_UP;
	GPIO_Init(GPIOC,&GPIO_InitStructure); 	
	
	TIM_TimeBaseStructure.TIM_Period = 9999;			//�Զ���װ��ֵ
	TIM_TimeBaseStructure.TIM_Prescaler = 167;		//��ʱ����Ƶ
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;		//���ϼ���ģʽ	
	TIM_TimeBaseStructure.TIM_ClockDivision=0; 						//ʱ�ӷ�Ƶ
	TIM_TimeBaseStructure.TIM_RepetitionCounter=0;					//�ظ���������
	
	TIM_TimeBaseInit(TIM3,&TIM_TimeBaseStructure);					//��ʼ��TIM3
	
	TIM_OCInitStructure.TIM_OCMode=TIM_OCMode_PWM1;				//PWMģʽ1
	TIM_OCInitStructure.TIM_OutputState=TIM_OutputState_Enable;	//ʹ�����
	TIM_OCInitStructure.TIM_Pulse=0;							//CCRx
	TIM_OCInitStructure.TIM_OCPolarity=TIM_OCPolarity_High;		//�ߵ�ƽ
	TIM_OCInitStructure.TIM_OCIdleState=TIM_OCIdleState_Set;	//���иߵ�ƽ
	
	TIM_OC1Init(TIM3, &TIM_OCInitStructure);  	//��ʼ��TIM3 CH1����Ƚ�
	TIM_OC2Init(TIM3, &TIM_OCInitStructure);  	//��ʼ��TIM3 CH2����Ƚ�
	TIM_OC3Init(TIM3, &TIM_OCInitStructure);  	//��ʼ��TIM3 CH3����Ƚ�
	TIM_OC4Init(TIM3, &TIM_OCInitStructure);  	//��ʼ��TIM3 CH4����Ƚ�
	
	TIM_OC1PreloadConfig(TIM3, TIM_OCPreload_Enable);  //ʹ��TIM3��CCR1�ϵ�Ԥװ�ؼĴ���
	TIM_OC2PreloadConfig(TIM3, TIM_OCPreload_Enable);  //ʹ��TIM3��CCR2�ϵ�Ԥװ�ؼĴ���
	TIM_OC3PreloadConfig(TIM3, TIM_OCPreload_Enable);  //ʹ��TIM3��CCR3�ϵ�Ԥװ�ؼĴ���
	TIM_OC4PreloadConfig(TIM3, TIM_OCPreload_Enable);  //ʹ��TIM3��CCR4�ϵ�Ԥװ�ؼĴ���
 
	TIM_ARRPreloadConfig(TIM3,ENABLE);	//TIM3	ARPEʹ�� 
	TIM_Cmd(TIM3, ENABLE);
 
}

