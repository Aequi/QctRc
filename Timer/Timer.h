#ifndef __TIMER_H__
#define __TIMER_H__

#include "stdint.h"

typedef void (*TimerCallback)(void);

void timerInit(TimerCallback timerCb);

#endif /* __TIMER_H__ */
