#define GLOBAL_VAR
#include "GlobalVar.h"
void global_var_init(void)
{
    /**
     * initialize event flag
     */
    debug_flag = 0;
    cal_event = RT_NULL;
    motor_speed_event = RT_NULL;
    com_event = RT_NULL;
    com_data = (Com_Data){0};
    command = (Command){0};
}
