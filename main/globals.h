#ifndef _GLOBALS_H
#define _GLOBALS_H
#include <stdbool.h>

extern volatile float pid_kp;
extern volatile float pid_kd;
extern volatile float pid_ki;
extern volatile bool control_active;

#endif /* _GLOBALS_H */