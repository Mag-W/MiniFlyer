
#include "stm32f4xx.h" 
#include <stdbool.h>

//#include "debug_assert.h"
#include "sbus.h"
#include "delay.h"

/* FreeRtos includes */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"


#define CPPM_TIMER                   TIM9
#define CPPM_TIMER_RCC               RCC_APB2Periph_TIM9
#define CPPM_GPIO_RCC                RCC_AHB1Periph_GPIOA
#define CPPM_GPIO_PORT               GPIOA
#define CPPM_GPIO_PIN                GPIO_Pin_3
#define CPPM_GPIO_SOURCE             GPIO_PinSource3
#define CPPM_GPIO_AF                 GPIO_AF_TIM9

#define CPPM_TIM_PRESCALER           (168-1) 


static xQueueHandle captureQueue;
static uint16_t prevCapureVal;
static bool captureFlag;
static bool isAvailible;


//cppm

static uint8_t currChannel = 0;
uint16_t rcData[CH_NUM];//����PPM��ͨ���ź�ֵ
rcLinkState_t rcLinkState;
failsafeState_t  failsafeState;


void cppmInit(void)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	TIM_ICInitTypeDef  TIM_ICInitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_AHB1PeriphClockCmd(CPPM_GPIO_RCC, ENABLE);
	RCC_APB2PeriphClockCmd(CPPM_TIMER_RCC, ENABLE);

	//����PPM�ź��������ţ�PA3��
	GPIO_StructInit(&GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Pin = CPPM_GPIO_PIN;
	GPIO_Init(CPPM_GPIO_PORT, &GPIO_InitStructure);

	GPIO_PinAFConfig(CPPM_GPIO_PORT, CPPM_GPIO_SOURCE, CPPM_GPIO_AF);

	//���ö�ʱ��1us tick
	TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);
	TIM_TimeBaseStructure.TIM_Prescaler = CPPM_TIM_PRESCALER;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(CPPM_TIMER, &TIM_TimeBaseStructure);

	//�������벶��
	TIM_ICStructInit(&TIM_ICInitStructure);
	TIM_ICInitStructure.TIM_Channel = TIM_Channel_2;
	TIM_ICInit(CPPM_TIMER, &TIM_ICInitStructure);

	NVIC_InitStructure.NVIC_IRQChannel = TIM1_BRK_TIM9_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 5;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	captureQueue = xQueueCreate(64, sizeof(uint16_t));

	TIM_ITConfig(CPPM_TIMER, TIM_IT_Update | TIM_IT_CC2, ENABLE);
	TIM_Cmd(CPPM_TIMER, ENABLE);
}


bool cppmIsAvailible(void)
{
	return isAvailible;
}

int cppmGetTimestamp(uint16_t *timestamp)
{
//	ASSERT(timestamp);

	return xQueueReceive(captureQueue, timestamp, 20);
}

void cppmClearQueue(void)
{
	xQueueReset(captureQueue);
}

uint16_t capureVal;
uint16_t capureValDiff;

void TIM1_BRK_TIM9_IRQHandler(void)
{
	portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

	if(TIM_GetITStatus(CPPM_TIMER, TIM_IT_CC2) != RESET)
	{
		if(TIM_GetFlagStatus(CPPM_TIMER, TIM_FLAG_CC2OF) != RESET)
		{
		  //TODO: Handle overflow error
		}

		capureVal = TIM_GetCapture2(CPPM_TIMER);
		capureValDiff = capureVal - prevCapureVal;
		prevCapureVal = capureVal;

		xQueueSendFromISR(captureQueue, &capureValDiff, &xHigherPriorityTaskWoken);
 
		captureFlag = true;
		TIM_ClearITPendingBit(CPPM_TIMER, TIM_IT_CC2);
	}

	if(TIM_GetITStatus(CPPM_TIMER, TIM_IT_Update) != RESET)
	{
		// Update input status
		isAvailible = (captureFlag == true);
		captureFlag = false;
		TIM_ClearITPendingBit(CPPM_TIMER, TIM_IT_Update);
	}
}


void rxInit(void)
{
	cppmInit();
}

void ppmTask(void *param)
{
	uint16_t ppm;
	uint32_t currentTick;
	while(1)
	{
		currentTick = getSysTickCnt();
		
		if(cppmGetTimestamp(&ppm) == pdTRUE)//20ms����ʽ��ȡPPM����ֵ
		{
			if (cppmIsAvailible() && ppm < 2100)//�ж�PPM֡����
			{
				if(currChannel < CH_NUM)
				{
					rcData[currChannel] = ppm;
				}
				currChannel++;
			}
			else//������һ֡����
			{
				currChannel = 0;
				rcLinkState.linkState = true;
				if (rcData[THROTTLE] < 950 || rcData[THROTTLE] > 2100)//��Ч���壬˵�����ջ������ʧ�ر�����ֵ
					rcLinkState.invalidPulse = true;
				else
					rcLinkState.invalidPulse = false;
				rcLinkState.realLinkTime = currentTick;
			}
		}
		
		if (currentTick - rcLinkState.realLinkTime > 1000)//1Sû���յ��ź�˵��ң��������ʧ��
		{
			rcLinkState.linkState = false;
		}	
	}
}

//void rxTask(void *param)
//{	
//	u32 tick = 0;
//	u32 lastWakeTime = getSysTickCnt();
//	uint32_t currentTick;
//	
//	while (1)
//	{
//		vTaskDelayUntil(&lastWakeTime, F2T(RATE_1000_HZ));//1KHz����Ƶ��
//		
//		if (RATE_DO_EXECUTE(RATE_50_HZ, tick))
//		{
//			currentTick = getSysTickCnt();
//			
//			//����ң������͸���ͨ��ģʽ�л�
//			if (rcLinkState.linkState == true)
//			{
////				processRcStickPositions();
////				processRcAUXPositions();
//			}
//			
//			//�������״̬��ң��ʧȥ����
//			if (ARMING_FLAG(ARMED))
//			{
//				if (rcLinkState.linkState == false || rcLinkState.invalidPulse == true)//ң��ʧȥ���ӻ���Ч����
//				{
//					if (failsafeState.failsafeActive == false)
//					{
//						if (currentTick > failsafeState.throttleLowPeriod )//��һ�ε�����ʱ�䣨��5�룩��˵���ɻ��ڵ��Ͽ�ֱ�ӹرյ��
//						{
//							mwDisarm();
//						}
//						else 
//						{
//							failsafeState.failsafeActive = true;
//							commanderActiveFailsafe();//����ʧ�ر����Զ�����
//						}
//					}
//				}
//				else//ң����������
//				{
//					throttleStatus_e throttleStatus = calculateThrottleStatus();
//					if (throttleStatus == THROTTLE_HIGH)
//					{
//						failsafeState.throttleLowPeriod = currentTick + 5000;//5000��ʾ��Ҫ�����ŵ�ʱ�䣨5�룩
//					}
//				}
//			}
//			else
//			{
//				failsafeState.throttleLowPeriod = 0;
//			}
//			
//		}
//		tick++;
//	}
//}

