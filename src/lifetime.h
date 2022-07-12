#ifndef __LIFETIME_H__
#define __LIFETIME_H__
#include "common.h"
#include "i2c.h"
#include "hardware.h"
#include "isr.h"
#include "global.h"

void Get_EEP_LifeTime(void);
void Update_EEP_LifeTime(void);
void ParseLifeTime(unsigned char *hourString, unsigned char *minuteString);

#endif