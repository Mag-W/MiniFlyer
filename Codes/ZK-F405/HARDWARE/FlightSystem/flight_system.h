#ifndef __FS_H
#define __FS_H
#include "sys.h"
#include<stdio.h>

//����״̬��־λ
typedef union
{
    char all;
    struct{
					char FlightUnlock:1; //λ�����
					char RCOnline:1;
					char bits6:6;
					}byte;
}FlightStatedef;


#endif
