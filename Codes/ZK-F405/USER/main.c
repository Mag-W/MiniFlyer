#include "sys.h"
#include "delay.h"

#include "FreeRTOS.h"
#include "task.h"
//#include "queue.h"
//#include "semphr.h"
//#include "event_groups.h"
//#include "timers.h"

#include "myiic.h"

#include "led.h"
#include "mpu6050_iic.h"
#include "inv_mpu.h"
#include "inv_mpu_dmp_motion_driver.h" 
#include "bmp280.h"
#include "pwm.h"
#include "Upper.h"









TaskHandle_t startTaskHandle;
static void startTask(void *arg);



//�����ʼ��
//�������ȼ�
#define ESCinit_TASK_PRIO		10
//�����ջ��С	
#define ESCinit_STK_SIZE 		1024  
//������
TaskHandle_t ESCinitTask_Handler;
//������
void escinit_task(void *pvParameters);


//���������ݻ�ȡ�ʹ���
//�������ȼ�
#define SENSORS_TASK_PRIO		3
//�����ջ��С	
#define SENSORS_STK_SIZE 		1024  
//������
TaskHandle_t SENSORS_Task_Handler;
//������
void sensors_task(void *pvParameters);



int main() 
{

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);//����ϵͳ�ж����ȼ�����4
	delay_init(168);	/*delay��ʼ��*/
	ledInit();			/*led��ʼ��*/
	uart_init(500000);
	TIM3_PWM_Init();/*PWM��ʼ��Ϊ50Hz*/
	IIC_Init();
	while(	MPU_Init()	);
	Bmp_Init ();
		while(mpu_dmp_init())
		{
			printf("dmp ��ʼ��ʧ�ܣ�-----%d\n",mpu_dmp_init());
		}
		printf("dmp ��ʼ���ɹ���-----%d\n",mpu_dmp_init());
	
	xTaskCreate(startTask, "START_TASK", 300, NULL, 2, &startTaskHandle);	/*������ʼ����*/
	
	vTaskStartScheduler();	/*�����������*/

	while(1){};
}




/*��������*/
void startTask(void *arg)
{
	taskENTER_CRITICAL();	/*�����ٽ���*/
	
	
	
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

		printf("unlocking!\n\n");
		TIM_SetCompare1(TIM3,999);
		TIM_SetCompare2(TIM3,999);
		TIM_SetCompare3(TIM3,999);
		TIM_SetCompare4(TIM3,999);

		vTaskDelay(1000);
		TIM_SetCompare1(TIM3,499);
		TIM_SetCompare2(TIM3,499);
		TIM_SetCompare3(TIM3,499);
		TIM_SetCompare4(TIM3,499);
		vTaskDelay(1000);
		//�����г�У׼�ͽ���
		printf("unlocked!\n\n");
		LED0_OFF;
		vTaskDelete (ESCinitTask_Handler);
	}   
}





void sensors_task(void *pvParameters)
{
	double	BMP_Pressure,BMP_Temperature;
	float pitch,roll,yaw; 		//ŷ����
	short aacx,aacy,aacz;		//���ٶȴ�����ԭʼ����
	short gyrox,gyroy,gyroz;	//������ԭʼ����
//	short temp;					//�¶�	 
	u8 report=1;	
	while(1)
	{
//		vTaskDelay(200);
//		LED0_TOGGLE;
		BMP_Temperature = BMP280_Get_Temperature();
		BMP_Pressure = BMP280_Get_Pressure();
		printf("\r\nTemperature %f C\n Pressure %f Pa\n",BMP_Temperature,BMP_Pressure);
		if(mpu_dmp_get_data(&pitch,&roll,&yaw)==0)//��Ҫ�����㹻����ܷ�ֹdmp�����������ȡ����Ƶ����úʹ�����DMP��������Э��
		{ 
//			temp=MPU_Get_Temperature();	//�õ��¶�ֵ
			MPU_Get_Accelerometer(&aacx,&aacy,&aacz);	//�õ����ٶȴ���������
			MPU_Get_Gyroscope(&gyrox,&gyroy,&gyroz);	//�õ�����������
			if(report)mpu6050_send_data(aacx,aacy,aacz,gyrox,gyroy,gyroz);//���Զ���֡���ͼ��ٶȺ�������ԭʼ����
			if(report)usart1_report_imu(aacx,aacy,aacz,gyrox,gyroy,gyroz,(int)(roll*100),(int)(pitch*100),(int)(yaw*10));
//			vTaskDelay (500);
//			printf("�¶�Ϊ��%d\n",temp/100);
//			temp=pitch;
//			printf("������Ϊ��%d\n",temp);
//			temp=roll;
//			printf("��ת��Ϊ��%d\n",temp);
//			temp=yaw;
//			printf("ƫ����Ϊ��%d\n",temp);
		}
//		else
//		{
//			printf("output is %d\n",mpu_dmp_get_data(&pitch,&roll,&yaw));
//		}
	}
}




































void vApplicationIdleHook( void )
{

}
















