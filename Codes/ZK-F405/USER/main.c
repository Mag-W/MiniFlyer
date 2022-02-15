#include "sys.h"
#include "delay.h"
#include "string.h"



#include "FreeRTOS.h"
#include "task.h"
#include "timer.h"
//#include "queue.h"
//#include "semphr.h"
#include "event_groups.h"
#include "timers.h"

#include "myiic.h"
#include "led.h"
#include "mpu6050_iic.h"
#include "inv_mpu.h"
#include "inv_mpu_dmp_motion_driver.h" 
#include "bmp280.h"
#include "pwm.h"
#include "Upper.h"
#include "sbus.h"
#include "flight_system.h"






//��ʼ������
TaskHandle_t startTaskHandle;
static void startTask(void *arg);



//�����ʼ��
//�������ȼ�
#define ESCinit_TASK_PRIO		6
//�����ջ��С	
#define ESCinit_STK_SIZE 		256  
//������
TaskHandle_t ESCinitTask_Handler;
//������
void escinit_task(void *pvParameters);


//���������ݻ�ȡ�ʹ���
//�������ȼ�
#define SENSORS_TASK_PRIO		5
//�����ջ��С	
#define SENSORS_STK_SIZE 		512  
//������
TaskHandle_t SENSORS_Task_Handler;
//������
void sensors_task(void *pvParameters);

//ң�����ݻ�ȡ�ʹ���
//�������ȼ�
#define RC_TASK_PRIO		4
//�����ջ��С	
#define RC_STK_SIZE 		512 
//������
TaskHandle_t RC_Task_Handler;
//������
void RC_task(void *pvParameters);

//����ʱ��ͳ������
//�������ȼ�
#define RUNTIMESTATS_TASK_PRIO	3
//�����ջ��С	
#define RUNTIMESTATS_STK_SIZE 	512  
//������
TaskHandle_t RunTimeStats_Handler;
//������
void RunTimeStats_task(void *pvParameters);















//�¼����
static EventGroupHandle_t RCtask_Handle =NULL; 

//�¼���
#define Esc_Unlocked 			(0x01<<0)
#define Flight_Unlocked 	(0x01<<1)
#define RC_Connected		 	(0x01<<2)



//������
#define ESC_MAX (1000-1)
#define ESC_MIN (500-1)

#define DELTA 5
#define RC_L1MIN (312+DELTA)
#define RC_L1MAX (1912-DELTA)
#define RC_L2MIN (608+DELTA)
#define RC_L2MAX (1408-DELTA)
#define RC_R1MIN (312+DELTA)
#define RC_R1MAX (1912-DELTA)


FlightStatedef FlightSystemFlag;//����״̬
char RunTimeInfo[400];		//������������ʱ����Ϣ

int main() 
{

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
	delay_init(168);	/*delay��ʼ��*/
	ledInit();			/*led��ʼ��*/
	uart_init(500000);
	TIM3_PWM_Init();/*PWM��ʼ��Ϊ50Hz*/
	SBUSInit();//SBUS
	IIC_Init();
	while(	MPU_Init()	);
	Bmp_Init ();
	while(mpu_dmp_init())
	{
		printf("dmp ��ʼ��ʧ�ܣ�-----%d\n",mpu_dmp_init());
	}

	//ȫ�ֱ�־��ʼ��
	FlightSystemFlag.all=0;
	
	xTaskCreate(startTask, "START_TASK", 300, NULL, 2, &startTaskHandle);	/*������ʼ����*/
	
	vTaskStartScheduler();	/*�����������*/

	while(1){};
}




/*��������*/
void startTask(void *arg)
{
	taskENTER_CRITICAL();	/*�����ٽ���*/
	
	//�����¼����
	RCtask_Handle = xEventGroupCreate(); 
	
		//���������ʼ������
	xTaskCreate((TaskFunction_t )escinit_task,     	
							(const char*    )"escinit_task",   	
							(uint16_t       )ESCinit_STK_SIZE, 
							(void*          )NULL,				
							(UBaseType_t    )ESCinit_TASK_PRIO,	
							(TaskHandle_t*  )&ESCinitTask_Handler); 

		//������������ȡ����			
	xTaskCreate((TaskFunction_t )sensors_task,     	
							(const char*    )"sensors_task",   	
							(uint16_t       )SENSORS_STK_SIZE, 
							(void*          )NULL,				
							(UBaseType_t    )SENSORS_TASK_PRIO,	
							(TaskHandle_t*  )&SENSORS_Task_Handler); 	
							
											
	//����RunTimeStats����
	xTaskCreate((TaskFunction_t )RunTimeStats_task,     
							(const char*    )"RunTimeStats_task",   
							(uint16_t       )RUNTIMESTATS_STK_SIZE,
							(void*          )NULL,
							(UBaseType_t    )RUNTIMESTATS_TASK_PRIO,
							(TaskHandle_t*  )&RunTimeStats_Handler); 							
							
	//����RC����
	xTaskCreate((TaskFunction_t )RC_task,     
							(const char*    )"RC_task",   
							(uint16_t       )RC_STK_SIZE,
							(void*          )NULL,
							(UBaseType_t    )RC_TASK_PRIO,
							(TaskHandle_t*  )&RC_Task_Handler); 							
														
							
							
							
							
							
							
							
							
							
							
	printf("Free heap: %d bytes\n", xPortGetFreeHeapSize());			/*��ӡʣ���ջ��С*/
							
	vTaskDelete(startTaskHandle);										/*ɾ����ʼ����*/

	taskEXIT_CRITICAL();	/*�˳��ٽ���*/
} 






//ESC��ʼ�������� 
void escinit_task(void *pvParameters)
{
	while(1)
	{
		LED0_ON;

		TIM_SetCompare1(TIM3,ESC_MAX);
		TIM_SetCompare2(TIM3,ESC_MAX);
		TIM_SetCompare3(TIM3,ESC_MAX);
		TIM_SetCompare4(TIM3,ESC_MAX);

		vTaskDelay(1000);
		TIM_SetCompare1(TIM3,ESC_MIN);
		TIM_SetCompare2(TIM3,ESC_MIN);
		TIM_SetCompare3(TIM3,ESC_MIN);
		TIM_SetCompare4(TIM3,ESC_MIN);
		vTaskDelay(1000);
		//�����г�У׼�͵������
		
		xEventGroupSetBits(RCtask_Handle,Esc_Unlocked);//��־����ѽ���
		LED0_OFF;
		vTaskDelete (ESCinitTask_Handler);
	}   
}




//��������������
void sensors_task(void *pvParameters)
{
	double	BMP_Pressure,BMP_Temperature;
	float pitch,roll,yaw; 		//ŷ����
	short aacx,aacy,aacz;		//���ٶȴ�����ԭʼ����
	short gyrox,gyroy,gyroz;	//������ԭʼ���� 
	u8 report=0;	
	u32 lastWakeTime = getSysTickCnt();
	while(1)
	{
		//5ms����һ��,IIC���ʽ����ҵײ���õ���while�����¸���������ռ�ýϳ�ʱ��
		vTaskDelayUntil(&lastWakeTime, 5);
		BMP_Temperature = BMP280_Get_Temperature();
		BMP_Pressure = BMP280_Get_Pressure();
		if(mpu_dmp_get_data(&pitch,&roll,&yaw)==0)//��Ҫ�����㹻����ܷ�ֹdmp�����������ȡ����Ƶ����úʹ�����DMP��������Э��
		{ 
			MPU_Get_Accelerometer(&aacx,&aacy,&aacz);	//�õ����ٶȴ���������
			MPU_Get_Gyroscope(&gyrox,&gyroy,&gyroz);	//�õ�����������
			if(report)mpu6050_send_data(aacx,aacy,aacz,gyrox,gyroy,gyroz);//���Զ���֡���ͼ��ٶȺ�������ԭʼ���ݣ�report�����Ƿ���
			if(report)usart1_report_imu(aacx,aacy,aacz,gyrox,gyroy,gyroz,(int)(roll*100),(int)(pitch*100),(int)(yaw*10));
		}
	}
}



//RunTimeStats����
void RunTimeStats_task(void *pvParameters)
{
	u8 report=0;
	while(1)
	{
		if(report)
		{
			memset(RunTimeInfo,0,400);				//��Ϣ����������
			vTaskGetRunTimeStats(RunTimeInfo);		//��ȡ��������ʱ����Ϣ
			printf("������\t\t\t����ʱ��\t������ռ�ٷֱ�\r\n");
			printf("%s\r\n",RunTimeInfo);
			vTaskDelay (2000);
		}
	}
}

//RC����
void RC_task(void *pvParameters)
{
	u8 t=0;
	u8 report=0;
	u16 FlightUnlockCnt=0;
	u16 FlightLockCnt=0;
	u32 lastWakeTime = getSysTickCnt();
	while(1)
	{
		//�ȵ����������
		xEventGroupWaitBits(RCtask_Handle, /* �¼������� */ 
												Esc_Unlocked,/* �����������Ȥ���¼� */ 
												pdFALSE, /* �˳�ʱ������¼�λ */ 
												pdTRUE, /* �������Ȥ�������¼� */ 
												portMAX_DELAY);/* ָ����ʱʱ��,һֱ�� */ 
		vTaskDelayUntil(&lastWakeTime, 50);
		Sbus_Data_Count(USART2_RX_BUF);
		
		if(FlightSystemFlag.byte.FlightUnlock==0)//��������ʱ
		{
			if(CH[2]<=RC_L1MIN&&CH[3]>=RC_L2MAX)//��ҡ�������´�3s���ɽ�������
			{
				FlightUnlockCnt++;
				if(FlightUnlockCnt>=60)
				{
					xEventGroupSetBits(RCtask_Handle,Flight_Unlocked);//��־�����ѽ���
					FlightSystemFlag.byte.FlightUnlock=1;
					printf("Flight unlocked!!\n");
					FlightUnlockCnt=0;
				}
			}
			else
			{
				FlightUnlockCnt=0;
			}
		}
		else//���н���ʱ
		{
			if(CH[2]<=RC_L1MIN&&CH[3]<=RC_L2MIN)//��ҡ�������´�3s������������
			{
				FlightLockCnt++;
				if(FlightLockCnt>=60)
				{
					xEventGroupClearBits(RCtask_Handle,Flight_Unlocked);//�������־
					FlightSystemFlag.byte.FlightUnlock=0;
					printf("Flight locked!!\n");
					FlightLockCnt=0;
				}
			}
			else
			{
				FlightLockCnt=0;
			}
		}

		
		//ͨ�������ϴ�
		if(report)
		{
			for(t=0;t<18;t++)
			{
				printf("%d--%d ",t,CH[t]);
			}
			printf("\n");
		}
	}
}






























void vApplicationIdleHook( void )
{

}
















