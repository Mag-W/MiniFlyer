#include "stdio.h"

#include "sbus.h"
#include "flight_system.h" 
#include "mpu6050_iic.h"
#include "myMath.h"


PidObject pidRateX; //内环PID数据
PidObject pidRateY;
PidObject pidRateZ;

PidObject pidPitch; //外环PID数据
PidObject pidRoll;
PidObject pidYaw;

int16_t motor_PWM_Value[4];//

#define MOTOR1 motor_PWM_Value[0] 
#define MOTOR2 motor_PWM_Value[1] 
#define MOTOR3 motor_PWM_Value[2] 
#define MOTOR4 motor_PWM_Value[3] 
uint16_t low_thr_cnt;

PidObject *(pPidObject[])={&pidRateX,&pidRateY,&pidRateZ,&pidRoll,&pidPitch,&pidYaw   //结构体数组，将每一个数组放一个pid结构体，这样就可以批量操作各个PID的数据了  比如解锁时批量复位pid控制数据，新手明白这句话的作用就可以了
};

//PID在此处修改
void pid_param_Init(void)//PID参数初始化
{
	pidRateX.kp = 1.5f;
	pidRateY.kp = 1.5f;
	pidRateZ.kp = 3.0f;
	
//	pidRateX.ki = 0.05f;
//	pidRateY.ki = 0.05f;
//	pidRateZ.ki = 0.02f;	
	
	pidRateX.kd = 0.3f;
	pidRateY.kd = 0.3f;
	pidRateZ.kd = 0.3f;	
	
	pidPitch.kp = 5.0f;
	pidRoll.kp = 5.0f;
	pidYaw.kp = 4.0f;	

	
	pidRateX.IntegLimitHigh  = 30.0f;
	pidRateY.IntegLimitHigh = 30.0f;
	pidRateZ.IntegLimitHigh= 30.0f;
	
	pidRateX.OutLimitHigh  = 100.0f;
	pidRateY.OutLimitHigh = 100.0f;
	pidRateZ.OutLimitHigh= 100.0f;
	
	pidPitch.IntegLimitHigh  = 30.0f;
	pidRoll.IntegLimitHigh = 30.0f;
	pidYaw.IntegLimitHigh= 30.0f;
	
	pidPitch.OutLimitHigh  = 100.0f;
	pidRoll.OutLimitHigh = 100.0f;
	pidYaw.OutLimitHigh= 100.0f;
	
	pidRest(pPidObject,6); //批量复位PID数据，防止上次遗留的数据影响本次控制

	Angle.yaw = pidYaw.desired =  pidYaw.measured = 0;   //锁定偏航角
	
	
}



/**************************************************************
 *批量复位PID函数
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
	
	error = pid->desired - pid->measured; //当前角度与实际角度的误差

	pid->integ += error * dt;	 //误差积分累加值

//	pid->integ = LIMIT(pid->integ,pid->IntegLimitLow,pid->IntegLimitHigh); //进行积分限幅

	deriv = (error - pid->prevError)/dt;  //前后两次误差做微分

	pid->out = pid->kp * error + pid->ki * pid->integ + pid->kd * deriv;//PID输出

//	pid->out = LIMIT(pid->out,pid->OutLimitLow,pid->OutLimitHigh); //输出限幅
	
	pid->prevError = error;  //更新上次的误差
		
}


/**************************************************************
 *  CascadePID
 * @param[in] 
 * @param[out] 
 * @return     
 ***************************************************************/	
void CascadePID(PidObject* pidRate,PidObject* pidAngE,const float dt)  //串级PID
{	 
	pidUpdate(pidAngE,dt);    //先计算外环
	pidRate->desired = pidAngE->out;
	pidUpdate(pidRate,dt);    //再计算内环	
}

/**************************************************************
 *  state_control
 * @param[in] 滤波后的陀螺仪数据、融合后的欧拉角、运行间隔
 * @param[out] 
 * @return     
 ***************************************************************/	
void state_control(float gx,float gy,float gz,float pitch,float roll,float yaw,float dt)
{
	u16 Throttle=0;
	Throttle=(ESC_MAX-ESC_MIN)*(CH[2]-RC_L1MIN)/RC_RANGE+ESC_MIN;//获取基础油门值
	if(Throttle>560)//飞行最小油门，即飞机起飞时开启PID
	{
		pidRateX.measured = gx * Gyro_G; //内环测量值 角度/秒
		pidRateY.measured = gy * Gyro_G;
		pidRateZ.measured = gz * Gyro_G;

		pidPitch.measured = pitch; //外环测量值 单位：角度
		pidRoll.measured = roll;
		pidYaw.measured = yaw;
		
		pidUpdate(&pidRoll,dt);    //调用PID处理函数来处理外环	横滚角PID		
		pidRateX.desired = pidRoll.out; //将外环的PID输出作为内环PID的期望值即为串级PID
		pidUpdate(&pidRateX,dt);  //再调用内环

		pidUpdate(&pidPitch,dt);    //调用PID处理函数来处理外环	俯仰角PID	
		pidRateY.desired = pidPitch.out;  
		pidUpdate(&pidRateY,dt); //再调用内环
		
		CascadePID(&pidRateZ,&pidYaw,dt);	//直接调用串级PID函数来处理
		
		MOTOR1 = MOTOR2 = MOTOR3 = MOTOR4 = LIMIT(Throttle,499,849);//留100给姿态控制
		
		MOTOR1 +=    + pidRateX.out - pidRateY.out - pidRateZ.out;// 姿态输出分配给各个电机的控制量
		MOTOR2 +=    - pidRateX.out - pidRateY.out + pidRateZ.out ;//
		MOTOR3 +=    - pidRateX.out + pidRateY.out - pidRateZ.out;
		MOTOR4 +=    + pidRateX.out + pidRateY.out + pidRateZ.out;//
	}
	else
	{
		MOTOR1 = MOTOR2 = MOTOR3 = MOTOR4 = LIMIT(Throttle,499,849); 
	}


	
	TIM_SetCompare1(TIM3,LIMIT(MOTOR1,499,949));
	TIM_SetCompare2(TIM3,LIMIT(MOTOR2,499,949));
	TIM_SetCompare3(TIM3,LIMIT(MOTOR3,499,949));
	TIM_SetCompare4(TIM3,LIMIT(MOTOR4,499,949));
	if(0)printf("1--%d\n2--%d\n3--%d\n4--%d\n",LIMIT(MOTOR1,499,949),LIMIT(MOTOR2,499,949),LIMIT(MOTOR3,499,949),LIMIT(MOTOR4,499,949));

}


