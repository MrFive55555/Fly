/*
 * Copyright (c) 2006-2019, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-05-24                  the first version
 */

#include <rthw.h>
#include <rtthread.h>

#if defined(RT_USING_USER_MAIN) && defined(RT_USING_HEAP)
/*
 * Please modify RT_HEAP_SIZE if you enable RT_USING_HEAP
 * the RT_HEAP_SIZE max value = (sram size - ZI size), 1024 means 1024 bytes
 */
#define RT_HEAP_SIZE (5 * 1024)
static rt_uint8_t rt_heap[RT_HEAP_SIZE];

RT_WEAK void *rt_heap_begin_get(void)
{
	return rt_heap;
}

RT_WEAK void *rt_heap_end_get(void)
{
	return rt_heap + RT_HEAP_SIZE;
}
#endif

void rt_os_tick_callback(void)
{
	rt_interrupt_enter();

	rt_tick_increase();

	rt_interrupt_leave();
}

// systick handler as rtthread tick
void SysTick_Handler(void)
{
	rt_os_tick_callback();
}

// delay us
void rt_hw_us_delay(rt_uint32_t us)
{
	rt_uint32_t reload = SysTick->LOAD;
	rt_uint32_t ticks = us * reload / (1000000 / RT_TICK_PER_SECOND); // 1MHZ = 1us need 72 tick = 1us
	rt_uint32_t told, tnow, tcnt = 0;
	told = SysTick->VAL; // get now laod value
	while (1)
	{
		tnow = SysTick->VAL; // get now load value
		if (tnow < told)
		{ // the count is decrease mode
			tcnt += told - tnow;
		}
		else
		{ // becasue we don't know the tnow what value, if it restart from max value, we need judge this case.
			tcnt += reload - tnow + told;
		}
		told = tnow;
		if (tcnt >= ticks)
			break;
	}
}

/**
 * This function will initial your board.
 */
void rt_hw_board_init(void)
{
	// #error "TODO 1: OS Tick Configuration."
	/*
	 * TODO 1: OS Tick Configuration
	 * Enable the hardware timer and call the rt_os_tick_callback function
	 * periodically with the frequency RT_TICK_PER_SECOND.
	 */
	SysTick_Config(SystemCoreClock / RT_TICK_PER_SECOND);
	/* Call components board initial (use INIT_BOARD_EXPORT()) */
#ifdef RT_USING_COMPONENTS_INIT
	rt_components_board_init();
#endif

#if defined(RT_USING_USER_MAIN) && defined(RT_USING_HEAP)
	rt_system_heap_init(rt_heap_begin_get(), rt_heap_end_get());
#endif
}

#ifdef RT_USING_CONSOLE

static int uart_init(void)
{
#error "TODO 2: Enable the hardware uart and config baudrate."
	return 0;
}
INIT_BOARD_EXPORT(uart_init);

void rt_hw_console_output(const char *str)
{
#error "TODO 3: Output the string 'str' through the uart."
}

#endif
