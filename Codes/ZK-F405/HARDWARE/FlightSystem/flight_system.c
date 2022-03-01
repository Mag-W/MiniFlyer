#include "stdio.h"

#include "sbus.h"
#include "flight_system.h" 
#include "mpu6050_iic.h"
#include "myMath.h"


PidObject pidRateX; //�ڻ�PID����
PidObject pidRateY;
PidObject pidRateZ;

PidObject pidPitch; //�⻷PID����
PidObject pidRoll;
PidObject pidYaw;

int16_t motor_PWM_Value[4];//

#define MOTOR1 motor_PWM_Value[0] 
#define MOTOR2 motor_PWM_Value[1] 
#define MOTOR3 motor_PWM_Value[2] 
#define MOTOR4 motor_PWM_Value[3] 
uint16_t low_thr_cnt;

PidObject *(pPidObject[])={&pidRateX,&pidRateY,&pidRateZ,&pidRoll,&pidPitch,&pidYaw   //�ṹ�����飬��ÿһ�������һ��pid�ṹ�壬�����Ϳ���������������PID��������  �������ʱ������λpid��������
};

//PID�ڴ˴��޸�
void pid_param_Init(void)//PID������ʼ��
{
	pidRateX.kp = 5.0f;//�ڻ�P�������ƫ����ٶȾ������������ٶ�
	pidRateY.kp = 5.0f;
	pidRateZ.kp = 0.6f;
	
	pidRateX.ki = 5.0f;//�ڻ�I�������ٶȿ��ƾ���
	pidRateY.ki = 5.0f;
	pidRateZ.ki = 0.6f;	
	
	pidRateX.kd = 0.07f;//�ڻ�D����ϵͳ�˶�,��ƫ��ոճ���ʱ�����ܴ�Ŀ������ã��ӿ�ϵͳ��Ӧ�ٶȣ����ٵ���ʱ�䣬�Ӷ�����ϵͳ�����ԣ����������ڼ�С�������˷��񵴣��Ӷ����ϵͳ�ȶ��ԣ�������������̬ƫ��
	pidRateY.kd = 0.07f;
	pidRateZ.kd = 0.00225f;	
	

	pidRoll.kp = 0.5f;
	pidPitch.kp = 0.5f;//�⻷P�������ƫ��ǶȾ����������Ƕ�
	pidYaw.kp = 0.1f;	


	pidRoll.ki = 0.0f;
	pidPitch.ki = 0.0f;//�⻷I�����Ƕȿ��ƾ���
	pidYaw.ki = 0.0f;	
	
	
	pidRateX.IntegLimitHigh  = PID_IMAX;
	pidRateY.IntegLimitHigh = PID_IMAX;
	pidRateZ.IntegLimitHigh= PID_IMAX;
	
	pidRateX.OutLimitHigh  = PIDMAX;
	pidRateY.OutLimitHigh = PIDMAX;
	pidRateZ.OutLimitHigh= PIDMAX;
	
	pidPitch.IntegLimitHigh  = PID_IMAX;
	pidRoll.IntegLimitHigh = PID_IMAX;
	pidYaw.IntegLimitHigh= PID_IMAX;
	
	pidPitch.OutLimitHigh  = PIDMAX;
	pidRoll.OutLimitHigh = PIDMAX;
	pidYaw.OutLimitHigh= PIDMAX;
	
	pidRateX.IntegLimitLow  = PID_IMIN;
	pidRateY.IntegLimitLow = PID_IMIN;
	pidRateZ.IntegLimitLow= PID_IMIN;
	
	pidRateX.OutLimitLow  = PIDMIN;
	pidRateY.OutLimitLow = PIDMIN;
	pidRateZ.OutLimitLow= PIDMIN;
	
	pidPitch.IntegLimitLow  = PID_IMIN;
	pidRoll.IntegLimitLow = PID_IMIN;
	pidYaw.IntegLimitLow= PID_IMIN;
	
	pidPitch.OutLimitLow  = PIDMIN;
	pidRoll.OutLimitLow = PIDMIN;
	pidYaw.OutLimitLow= PIDMIN;
	
	pidRest(pPidObject,6); //������λPID���ݣ���ֹ�ϴ�����������Ӱ�챾�ο���

	Angle.yaw = pidYaw.desired =  pidYaw.measured = 0;   //����ƫ����
	
	
}



/**************************************************************
 *������λPID����
 * @param[in] 
 * @param[out] 
 * @return     
 ***************************************************************/	
void pidRest(PidObject **pid,const uint8_t len)
{
	uint8_t i;
	for(i=0;i<len;i++)
	{
		pid[i]->integ = 0;
		pid[i]->prevError = 0;
		pid[i]->out = 0;
		pid[i]->offset = 0;
	}
}


/**************************************************************
 * Update the PID parameters.
 *
 * @param[in] pid         A pointer to the pid object.
 * @param[in] measured    The measured value
 * @param[in] updateError Set to TRUE if error should be calculated.
 *                        Set to False if pidSetError() has been used.
 * @return PID algorithm output
 ***************************************************************/	
void pidUpdate(PidObject* pid,const float dt)
{
	float error;
	float deriv;
	
	error = pid->desired - pid->measured; //��ǰ�Ƕ���ʵ�ʽǶȵ����

	pid->integ += error * dt;	 //�������ۼ�ֵ

	pid->integ = LIMIT(pid->integ,pid->IntegLimitLow,pid->IntegLimitHigh); //���л����޷�

	deriv = (error - pid->prevError)/dt;  //ǰ�����������΢��

	pid->out = pid->kp * error + pid->ki * pid->integ + pid->kd * deriv;//PID���

	pid->out = LIMIT(pid->out,pid->OutLimitLow,pid->OutLimitHigh); //����޷�
	
	pid->prevError = error;  //�����ϴε����
		
}


/**************************************************************
 *  CascadePID
 * @param[in] 
 * @param[out] 
 * @return     
 ***************************************************************/	
void CascadePID(PidObject* pidRate,PidObject* pidAngE,const float dt)  //����PID
{	 
	pidUpdate(pidAngE,dt);    //�ȼ����⻷
	pidRate->desired = pidAngE->out;
	pidUpdate(pidRate,dt);    //�ټ����ڻ�	
}

/**************************************************************
 *  state_control
 * @param[in] �˲�������������ݡ��ںϺ��ŷ���ǡ����м��
 * @param[out] 
 * @return     
 ***************************************************************/	
void state_control(float dt)
{
	u16 Throttle=0;
	Throttle=(ESC_MAX-ESC_MIN)*(CH[2]-RC_L1MIN)/RC_RANGE+ESC_MIN;//��ȡ��������ֵ
	MOTOR1 = MOTOR2 = MOTOR3 = MOTOR4 = LIMIT(Throttle,ESC_MIN,ESC_MAX-PIDMAX);//������̬����һ����
	
	pidPitch.desired=(ANGLE_MAX-ANGLE_MIN)*(CH[1]-RC_R1MIN)/RC_RANGE+ANGLE_MIN;//�������Ƿ�Χ��ң�����趨�Ƕ�
	pidRoll.desired=-((ANGLE_MAX-ANGLE_MIN)*(CH[0]-RC_R1MIN)/RC_RANGE+ANGLE_MIN);//�������Ƿ�Χ��ң�����趨�Ƕ�
	
	if(pidPitch.desired>-2&&pidPitch.desired<2)
	{
		pidPitch.desired=0;
	}
	if(pidRoll.desired>-2&&pidRoll.desired<2)
	{
		pidRoll.desired=0;
	}
	
	printf("desire pitch is %f\ndesire roll is %f\n",pidPitch.desired,pidRoll.desired);
	
	if(CH[5]>=1780)//����ch6�²�����PID����
	{
		pidRateX.measured = MPU6050.gyroX * Gyro_G; //�ڻ�����ֵ �Ƕ�/��
		pidRateY.measured = MPU6050.gyroY * Gyro_G;
		pidRateZ.measured = MPU6050.gyroZ * Gyro_G;

		pidPitch.measured = Angle.pitch; //�⻷����ֵ ��λ���Ƕ�
		pidRoll.measured = Angle.roll;
		pidYaw.measured = Angle.yaw;
		
		pidUpdate(&pidRoll,dt);    //����PID�������������⻷	�����PID		
		pidRateX.desired = pidRoll.out; //���⻷��PID�����Ϊ�ڻ�PID������ֵ��Ϊ����PID
		pidUpdate(&pidRateX,dt);  //�ٵ����ڻ�

		pidUpdate(&pidPitch,dt);    //����PID�������������⻷	������PID	
		pidRateY.desired = pidPitch.out;  
		pidUpdate(&pidRateY,dt); //�ٵ����ڻ�

		CascadePID(&pidRateZ,&pidYaw,dt);	//ֱ�ӵ��ô���PID����������

		if(0)printf("PIDX��%f\tPIDY��%f\tPIDZ��%f\t\n",pidRateX.out,pidRateY.out,pidRateZ.out);
		
		MOTOR1 +=    + pidRateX.out - pidRateY.out - pidRateZ.out;// ��̬����������������Ŀ�����
		MOTOR2 +=    - pidRateX.out - pidRateY.out + pidRateZ.out ;//
		MOTOR3 +=    - pidRateX.out + pidRateY.out - pidRateZ.out;
		MOTOR4 +=    + pidRateX.out + pidRateY.out + pidRateZ.out;//
	}
	else
	{
		pidRest(pPidObject,6);
	}



	
	TIM_SetCompare1(TIM3,LIMIT(MOTOR1,ESC_MIN,ESC_MAX));
	TIM_SetCompare2(TIM3,LIMIT(MOTOR2,ESC_MIN,ESC_MAX));
	TIM_SetCompare3(TIM3,LIMIT(MOTOR3,ESC_MIN,ESC_MAX));
	TIM_SetCompare4(TIM3,LIMIT(MOTOR4,ESC_MIN,ESC_MAX));
	if(1)printf("1--%d\t2--%d\t3--%d\t4--%d\t\n",LIMIT(MOTOR1,ESC_MIN,ESC_MAX),LIMIT(MOTOR2,ESC_MIN,ESC_MAX),LIMIT(MOTOR3,ESC_MIN,ESC_MAX),LIMIT(MOTOR4,ESC_MIN,ESC_MAX));

}


