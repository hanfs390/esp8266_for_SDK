/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2016 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on ESPRESSIF SYSTEMS ESP8266 only, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "ets_sys.h"
#include "osapi.h"
#include "driver/uart.h"
#include "user_interface.h"

os_timer_t os_timer_1; /* have to be global */
extern u8 DHT11_Data_Array[6];
/******************************************************************************
 * FunctionName : user_rf_cal_sector_set
 * Description  : SDK just reversed 4 sectors, used for rf init data and paramters.
 *                We add this function to force users to set rf cal sector, since
 *                we don't know which sector is free in user's application.
 *                sector map for last several sectors : ABCCC
 *                A : rf cal
 *                B : rf init data
 *                C : sdk parameters
 * Parameters   : none
 * Returns      : rf cal sector
*******************************************************************************/
uint32 ICACHE_FLASH_ATTR
user_rf_cal_sector_set(void)
{
    enum flash_size_map size_map = system_get_flash_size_map();
    uint32 rf_cal_sec = 0;

    switch (size_map) {
        case FLASH_SIZE_4M_MAP_256_256:
            rf_cal_sec = 128 - 5;
            break;

        case FLASH_SIZE_8M_MAP_512_512:
            rf_cal_sec = 256 - 5;
            break;

        case FLASH_SIZE_16M_MAP_512_512:
            rf_cal_sec = 512 - 5;
            break;
        case FLASH_SIZE_16M_MAP_1024_1024:
            rf_cal_sec = 512 - 5;
            break;

        case FLASH_SIZE_32M_MAP_512_512:
            rf_cal_sec = 1024 - 5;
            break;
        case FLASH_SIZE_32M_MAP_1024_1024:
            rf_cal_sec = 1024 - 5;
            break;

        case FLASH_SIZE_64M_MAP_1024_1024:
            rf_cal_sec = 2048 - 5;
            break;
        case FLASH_SIZE_128M_MAP_1024_1024:
            rf_cal_sec = 4096 - 5;
            break;
        default:
            rf_cal_sec = 0;
            break;
    }

    return rf_cal_sec;
}



void ICACHE_FLASH_ATTR
user_rf_pre_init(void)
{
}

void ICACHE_FLASH_ATTR delay_ms(u32 time)
{
	int i = time;
	for (; i > 0; i--) {
		os_delay_us(1000);
	}
}
void ICACHE_FLASH_ATTR timer1_handle(void)
{
	//os_printf("get in tiemr handle\r\n");
	int status;
	struct ip_info ip_conf;
	
	status = wifi_station_get_connect_status();
	if (status != 5)
		return;
	else {
		wifi_get_ip_info(STATION_IF, &ip_conf);
		os_printf("%d.%d.%d.%d\r\n", (ip_conf.ip.addr >> 0) & 0xff,
				(ip_conf.ip.addr >> 8) & 0xff, (ip_conf.ip.addr >> 16) & 0xff,
				(ip_conf.ip.addr >> 24) & 0xff);
	}
	if(DHT11_Read_Data_Complete() == 0)		// 读取DHT11温湿度值
	{
		//-------------------------------------------------
		// DHT11_Data_Array[0] == 湿度_整数_部分
		// DHT11_Data_Array[1] == 湿度_小数_部分
		// DHT11_Data_Array[2] == 温度_整数_部分
		// DHT11_Data_Array[3] == 温度_小数_部分
		// DHT11_Data_Array[4] == 校验字节
		// DHT11_Data_Array[5] == 【1:温度>=0】【0:温度<0】
		//-------------------------------------------------


		// 温度超过30℃，LED亮
		//----------------------------------------------------
		if(DHT11_Data_Array[5]==1 && DHT11_Data_Array[2]>=30)
			GPIO_OUTPUT_SET(GPIO_ID_PIN(4),0);		// LED亮
		else
			GPIO_OUTPUT_SET(GPIO_ID_PIN(4),1);		// LED灭


		// 串口输出温湿度
		//---------------------------------------------------------------------------------
		if(DHT11_Data_Array[5] == 1)			// 温度 >= 0℃
		{
			os_printf("\r\n humidity == %d.%d %rh\r\n",DHT11_Data_Array[0], DHT11_Data_Array[1]);
			os_printf("\r\n temperature == %d.%d %c%c\r\n", DHT11_Data_Array[2], DHT11_Data_Array[3], 0xdf, 0x43);
		}
		else // if(DHT11_Data_Array[5] == 0)	// 温度 < 0℃
		{
			os_printf("\r\n humidity == %d.%d %rh\r\n",DHT11_Data_Array[0],DHT11_Data_Array[1]);
			os_printf("\r\n temperature == -%d.%d %c%c\r\n",DHT11_Data_Array[2],DHT11_Data_Array[3], 0xdf, 0x43);
		}

		// OLED显示温湿度
		//---------------------------------------------------------------------------------
		//DHT11_NUM_Char();	// DHT11数据值转成字符串

		//OLED_ShowString(0,2,DHT11_Data_Char[0]);	// DHT11_Data_Char[0] == 【湿度字符串】
		//OLED_ShowString(0,6,DHT11_Data_Char[1]);	// DHT11_Data_Char[1] == 【温度字符串】
	}
}

static void gpio_interrupt(void)
{
	u32 all_interrupt;
	u32 gpio0_interupt;
	/* read the bit of interrupt status */
	all_interrupt = GPIO_REG_READ(GPIO_STATUS_ADDRESS);
	/* MUST: clear the bit of interrupt status */
	GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, all_interrupt);

	/* get gpio0 interrupt */
	gpio0_interupt = all_interrupt & (1 << 0);
	if (gpio0_interupt)
		os_printf("into interrupt\r\n");
}

/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR
user_init(void)
{
	int led_switch = 1;
	int time_ms = 1000;
	int time_repetitive = 1; /*0, not repetitive*/
	struct station_config conf;
	
	os_printf("***********ESP8266 start\r\n");
	os_printf("SDK version: %s\r\n",	system_get_sdk_version());
	/* start interrupte */
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0);
	GPIO_DIS_OUTPUT(GPIO_ID_PIN(0)); /* input */
	
	ETS_GPIO_INTR_DISABLE();
	ETS_GPIO_INTR_ATTACH((void *)gpio_interrupt, NULL);
	gpio_pin_intr_state_set(GPIO_ID_PIN(0), GPIO_PIN_INTR_NEGEDGE); /* set interrupt */
	ETS_GPIO_INTR_ENABLE();
	/* end interrupt */
	
	/* start soft timer */
	os_timer_disarm(&os_timer_1); /* disable it */
	os_timer_setfn(&os_timer_1, (os_timer_func_t*)timer1_handle, NULL);
	os_timer_arm(&os_timer_1, time_ms, time_repetitive);
	/* not call system_timer_reinit, time_ms is [5ms ~ 6870947ms] */
	/* call system_timer_reinit, time_ms is [100ms ~ 428496ms] */
	/* end soft timer */

	/* set wifi */
	
	wifi_set_opmode(STATION_MODE);
	memset(&conf, 0, sizeof(struct station_config));
	strncpy(conf.ssid, "hfs", 3);
	strncpy(conf.password, "12345678", 8);
	wifi_station_set_config(&conf);
#if 0
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO4_U, FUNC_GPIO4); /* GIO4 output */
	while (1) {
		/* led */
		GPIO_OUTPUT_SET(GPIO_ID_PIN(4), led_switch);
		delay_ms(1000);
		led_switch = !led_switch;
		system_soft_wdt_feed();	
	}
#endif
	os_printf("***********ESP8266 stop\r\n");
}

