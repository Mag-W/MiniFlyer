#include "nvic.h"
#include "led.h"

/*FreeRTOS���ͷ�ļ�*/
#include "FreeRTOS.h"		 
#include "task.h"



static u32 sysTickCnt=0;

void nvicInit(void)
{
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
}

extern void xPortSysTickHandler(void);

/********************************************************
 *SysTick_Handler()
 *�δ�ʱ���жϷ�����
*********************************************************/
void  SysTick_Handler(void)
{
	if(xTaskGetSchedulerState()!=taskSCHEDULER_NOT_STARTED)	/*ϵͳ�Ѿ�����*/
    {
        xPortSysTickHandler();	
    }else
	{
		sysTickCnt++;	/*���ȿ���֮ǰ����*/
	}
}

/********************************************************
*getSysTickCnt()
*���ȿ���֮ǰ ���� sysTickCnt
*���ȿ���֮ǰ ���� xTaskGetTickCount()
*********************************************************/
u32 getSysTickCnt(void)
{
	if(xTaskGetSchedulerState()!=taskSCHEDULER_NOT_STARTED)	/*ϵͳ�Ѿ�����*/
		return xTaskGetTickCount();
	else
		return sysTickCnt;
}


