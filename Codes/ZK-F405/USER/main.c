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





//ȫ�ֱ�������־λ����
FlightStatedef FlightSystemFlag;//����״̬
char RunTimeInfo[400];		//������������ʱ����Ϣ


int main() 
{

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
	delay_init(168);	/*delay��ʼ��*/
	ledInit();			/*led��ʼ��*/
	uart_init(500000);
	TIM3_PWM_Init();/*PWM��ʼ��Ϊ50Hz*/
	SBUSInit();
	IIC_Init();
	while(	MPU_Init()	);
	Bmp_Init ();
	pid_param_Init();
	OffsetInit();
	
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

		//�������������ݻ�ȡ����			
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
		TIM_SetCompare4(TIM3,ESC_MAX);
		TIM_SetCompare3(TIM3,ESC_MAX);
		TIM_SetCompare2(TIM3,ESC_MAX);
		TIM_SetCompare1(TIM3,ESC_MAX);


		vTaskDelay(4000);//�����������ʱʱ��
		TIM_SetCompare4(TIM3,ESC_MIN);
		TIM_SetCompare3(TIM3,ESC_MIN);
		TIM_SetCompare2(TIM3,ESC_MIN);
		TIM_SetCompare1(TIM3,ESC_MIN);
		//�����г�У׼�͵������
		
		xEventGroupSetBits(RCtask_Handle,Esc_Unlocked);//��־����ѽ���
		LED0_OFF;

		vTaskDelete (ESCinitTask_Handler);
	}   
}




//��������������
void sensors_task(void *pvParameters)
{
	double	BMP_Pressure;
	u8 report=0;

	u32 lastWakeTime = getSysTickCnt();
	while(1)
	{
		//5ms����һ��

		vTaskDelayUntil(&lastWakeTime, 5);

		BMP_Pressure = BMP280_Get_Pressure();
		MpuGetData(); //��ȡmpu���ݲ��˲�
		GetAngle(&MPU6050,&Angle,0.005f);//���ٶȼƺ����������ݽ���Ϊŷ����
		if(FlightSystemFlag.byte.FlightUnlock==1&&CH[2]>(RC_L1MIN-DELTA))//��������������ֵ������Сֵ������״̬����
		{
			state_control(0.005f);
		}
		if(0)//������
		{
			printf("YAW��%f\nPITCH��%f\nROLL��%f\n",Angle.yaw,Angle.pitch,Angle.roll);
		}
		if(report)mpu6050_send_data(MPU6050.accX,MPU6050.accY,MPU6050.accZ,MPU6050.gyroX,MPU6050.gyroY,MPU6050.gyroZ);//���Զ���֡���ͼ��ٶȺ����������ݣ�report�����Ƿ���
		if(report)usart1_report_imu(MPU6050.accX,MPU6050.accY,MPU6050.accZ,MPU6050.gyroX,MPU6050.gyroY,MPU6050.gyroZ,(int)(Angle.roll*100),(int)(Angle.pitch*100),(int)(Angle.yaw*10));

	}
}



//RunTimeStats����
void RunTimeStats_task(void *pvParameters)
{
	u8 report=0;
	while(1)
	{
		//�ȵ����������
		xEventGroupWaitBits(RCtask_Handle, /* �¼������� */ 
												Esc_Unlocked,/* �����������Ȥ���¼� */ 
												pdFALSE, /* �˳�ʱ������¼�λ */ 
												pdTRUE, /* �������Ȥ�������¼� */ 
												portMAX_DELAY);/* ָ����ʱʱ��,һֱ�� */ 
		vTaskDelay (500);
		if(report)
		{
			vTaskDelay (500);
			memset(RunTimeInfo,0,400);				//��Ϣ����������
			vTaskGetRunTimeStats(RunTimeInfo);		//��ȡ��������ʱ����Ϣ
			printf("������\t\t\t����ʱ��\t������ռ�ٷֱ�\r\n");
			printf("%s\r\n",RunTimeInfo);
			
		}
		if(FlightSystemFlag.byte.FlightUnlock==1)
		{
			LED0_TOGGLE;
		}
		else
		{
//			LED0_OFF;
		}
	}
}

//RC����
void RC_task(void *pvParameters)
{
	u8 t=0;
	u8 report=0;//ң����ͨ���ϱ�����
	u16 FlightUnlockCnt=0;
	u16 FlightLockCnt=0;
	u16 OffsetCnt=0;
	u32 lastWakeTime = getSysTickCnt();
	while(1)
	{
		//�ȵ����������
		xEventGroupWaitBits(RCtask_Handle, /* �¼������� */ 
												Esc_Unlocked,/* �����������Ȥ���¼� */ 
												pdFALSE, /* �˳�ʱ������¼�λ */ 
												pdTRUE, /* �������Ȥ�������¼� */ 
												portMAX_DELAY);/* ָ����ʱʱ��,һֱ�� */ 
		vTaskDelayUntil(&lastWakeTime, 5);
		Sbus_Data_Count(USART2_RX_BUF);
		if(FlightSystemFlag.byte.FlightUnlock==0)//��������ʱ
		{
			if(CH[2]<=(RC_L1MIN+DELTA)&&CH[3]>=(RC_L2MAX-DELTA))//��ҡ�������´�3s���ɽ�������
			{
				FlightUnlockCnt++;
				if(FlightUnlockCnt>=SET_TIME)
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
				if(CH[2]>=(RC_L1MAX-DELTA)&&CH[3]<=(RC_L2MIN+DELTA))//��ҡ�������ϴ�3s
				{
					OffsetCnt++;
					if(OffsetCnt>=SET_TIME)
					{
						vTaskSuspend(SENSORS_Task_Handler);
						MpuGetOffset(); //У׼
						OffsetCnt=0;
						LED0_ON;
						printf("offseted!\n");
						vTaskResume(SENSORS_Task_Handler);
					}
				}
				else
				{
					OffsetCnt=0;
				}
			}
		}
		else//���н���ʱ
		{
			if(CH[2]<=(RC_L1MIN+DELTA)&&CH[3]<=(RC_L2MIN+DELTA))//��ҡ�������´�3s������������
			{
				FlightLockCnt++;
				if(FlightLockCnt>=SET_TIME)
				{
					xEventGroupClearBits(RCtask_Handle,Flight_Unlocked);//�������־
					FlightSystemFlag.byte.FlightUnlock=0;
					TIM_SetCompare1(TIM3,499);
					TIM_SetCompare2(TIM3,499);
					TIM_SetCompare3(TIM3,499);
					TIM_SetCompare4(TIM3,499);
					LED0_OFF;
					printf("Flight locked!!\n");
					FlightLockCnt=0;
				}
			}
			else
			{
				FlightLockCnt=0;
			}
		}
		if(CH[2]<=(RC_L1OFF+DELTA))//ң����������ʱ
		{
			FlightSystemFlag.byte.RCOnline=0;
		}
		else
			if(CH[2]>=(RC_L1MIN-DELTA))
			{
				FlightSystemFlag.byte.RCOnline=1;
			}
			else
			{
				printf("unknowing state!\n");
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
















