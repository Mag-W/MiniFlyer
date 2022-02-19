#include "stdio.h"

#include "sbus.h"
#include "flight_system.h" 

const float M_PI = 3.1415926535;
const float RtA = 57.2957795f;
const float AtR = 0.0174532925f;
const float Gyro_G = 0.03051756f*2;	  	//�����ǳ�ʼ������+-2000��ÿ����1 / (65536 / 4000) = 0.03051756*2		
const float Gyro_Gr = 0.0005326f*2;     //������ÿ��,ת������ÿ���� 2*0.03051756	 * 0.0174533f = 0.0005326*2


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

PidObject *(pPidObject[])={&pidRateX,&pidRateY,&pidRateZ,&pidRoll,&pidPitch,&pidYaw   //�ṹ�����飬��ÿһ�������һ��pid�ṹ�壬�����Ϳ���������������PID��������  �������ʱ������λpid�������ݣ�����������仰�����þͿ�����
};

//PID�ڴ˴��޸�
void pid_param_Init(void)//PID������ʼ��
{
	pidRateX.kp = 3.f;
	pidRateY.kp = 3.f;
	pidRateZ.kp = 1.0f;
	
	pidRateX.ki = 0.05f;
	pidRateY.ki = 0.05f;
	pidRateZ.ki = 0.02f;	
	
	pidRateX.kd = 0.3f;
	pidRateY.kd = 0.3f;
	pidRateZ.kd = 0.3f;	
	
	pidPitch.kp = 10.0f;
	pidRoll.kp = 10.0f;
	pidYaw.kp = 8.0f;	
	
	pidRest(pPidObject,6); //������λPID���ݣ���ֹ�ϴ�����������Ӱ�챾�ο���

	pidYaw.desired =  pidYaw.measured = 0;   //����ƫ����
	
	
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

	//pid->integ = LIMIT(pid->integ,pid->IntegLimitLow,pid->IntegLimitHigh); //���л����޷�

	deriv = (error - pid->prevError)/dt;  //ǰ�����������΢��

	pid->out = pid->kp * error + pid->ki * pid->integ + pid->kd * deriv;//PID���

	//pid->out = LIMIT(pid->out,pid->OutLimitLow,pid->OutLimitHigh); //����޷�
	
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
 * @param[in] �����ǳ�ʼ���ݡ��ںϺ��ŷ���ǡ����м��
 * @param[out] 
 * @return     
 ***************************************************************/	
void state_control(float gx,float gy,float gz,float pitch,float roll,float yaw,float dt)
{
	u16 Throttle=0;
	Throttle=(ESC_MAX-ESC_MIN)*(CH[2]-RC_L1MIN)/RC_RANGE+ESC_MIN;//��ȡ��������ֵ
	
	pidRateX.measured = gx * Gyro_G; //�ڻ�����ֵ �Ƕ�/��
	pidRateY.measured = gy * Gyro_G;
	pidRateZ.measured = gz * Gyro_G;

	pidPitch.measured = pitch; //�⻷����ֵ ��λ���Ƕ�
	pidRoll.measured = roll;
	pidYaw.measured = yaw;
	
	pidUpdate(&pidRoll,dt);    //����PID�������������⻷	�����PID		
	pidRateX.desired = pidRoll.out; //���⻷��PID�����Ϊ�ڻ�PID������ֵ��Ϊ����PID
	pidUpdate(&pidRateX,dt);  //�ٵ����ڻ�

	pidUpdate(&pidPitch,dt);    //����PID�������������⻷	������PID	
	pidRateY.desired = pidPitch.out;  
	pidUpdate(&pidRateY,dt); //�ٵ����ڻ�
	
	CascadePID(&pidRateZ,&pidYaw,dt);	//ֱ�ӵ��ô���PID����������
	
	MOTOR1 = MOTOR2 = MOTOR3 = MOTOR4 = LIMIT(Throttle,499,849); //��100����̬����
	
	MOTOR1 +=    + pidRateX.out - pidRateY.out + pidRateZ.out;//; ��̬����������������Ŀ�����
	MOTOR2 +=    - pidRateX.out - pidRateY.out - pidRateZ.out ;//;
	MOTOR3 +=    - pidRateX.out + pidRateY.out + pidRateZ.out;
	MOTOR4 +=    + pidRateX.out + pidRateY.out - pidRateZ.out;//;
	
//	MOTOR1 +=    + pidRateX.out - pidRateY.out;
//	MOTOR2 +=    - pidRateX.out - pidRateY.out;
//	MOTOR3 +=    - pidRateX.out + pidRateY.out;
//	MOTOR4 +=    + pidRateX.out + pidRateY.out;
	
	TIM_SetCompare1(TIM3,LIMIT(MOTOR1,499,949));
	TIM_SetCompare2(TIM3,LIMIT(MOTOR2,499,949));
	TIM_SetCompare3(TIM3,LIMIT(MOTOR3,499,949));
	TIM_SetCompare4(TIM3,LIMIT(MOTOR4,499,949));
	printf("1--%d\n2--%d\n3--%d\n4--%d\n",LIMIT(MOTOR1,499,949),LIMIT(MOTOR2,499,949),LIMIT(MOTOR3,499,949),LIMIT(MOTOR4,499,949));

}


