/*
  * Copyright (c) 2013 Qualcomm Atheros, Inc..
  * All Rights Reserved.
  * Qualcomm Atheros Confidential and Proprietary.
  */

#include "qcom_common.h"
#include "swat_wmiconfig_common.h"
#include "qcom_pwm_api.h"
#include "qcom_nvram.h"

void
swat_wmiconfig_power_mode_set(A_UINT8 device_id, A_UINT32 powerMode)
{
	#if 0/*merge from CL1147958*/
	#ifndef FPGA
	/* FIXME: Writing to a register from application doesn't look right */
	 _qcom_reg_write(0x60018, 0x00f24400);
	#endif
	#endif
	 qcom_power_set_mode(device_id, powerMode);
}

void
swat_wmiconfig_pmparams(A_UINT8 device_id, A_UINT16 idlePeriod,
                        A_UINT16 psPollNum, A_UINT16 dtimPolicy,
                        A_UINT16 tx_wakeup_policy, A_UINT16 num_tx_to_wakeup,
                        A_UINT16 ps_fail_event_policy)
{

     qcom_power_set_parameters(device_id, idlePeriod, psPollNum, dtimPolicy, tx_wakeup_policy, 
                                 num_tx_to_wakeup, ps_fail_event_policy); 
}

void
swat_wmiconfig_pm_ebt_mac_params(A_UINT8 device_id, A_UINT8 action,
                        A_UINT8 *ebt, A_UINT8 *mac_filter)
{
    qcom_pm_set_get_ebt_mac_filter_params(device_id, action, ebt, mac_filter);
}

void
swat_wmiconfig_suspenable(A_UINT8 device_id)
{
    /* 1: enable */
    /* 0: disable */
     qcom_suspend_enable(1);
}

void
swat_wmiconfig_suspflag_get(A_UINT8 device_id, A_UINT8 *flag)
{
  qcom_suspend_restore_flag_get(flag);
}

void
swat_wmiconfig_suspstart(A_UINT32 susp_time)
{
    //notice: nvram_init will cause jtag hung.
    qcom_nvram_init(); /* sometimes this function is not called, and fini call will crash */
    qcom_nvram_fini(); //power down flash before go to suspend mode
    if(qcom_suspend_start(susp_time) == A_ERROR){
        SWAT_PTF("Store recall failed\n");
    }
}
void
swat_wmiconfig_pwm_ctrl(A_UINT8 module,A_UINT8 enable,A_UINT8 port)
{
	qcom_pwm_control((A_BOOL)module,(A_BOOL)enable,(A_UINT32)port);

}
void
swat_wmiconfig_pwm_set(A_UINT8 module,A_UINT32 freq, A_UINT32 duty_cycle,A_UINT32 phase,A_UINT8 port,A_UINT8 src_clk)
{
	if(module){
		qcom_pwm_sdm_set((A_UINT32) freq,(A_UINT32)duty_cycle,(A_UINT32)port);
	}
	else{
		qcom_pwm_port_set((A_UINT32) freq,(A_UINT32)duty_cycle,(A_UINT32)phase,(A_UINT32) port, (A_UINT32) src_clk);
				
	}
}

void
swat_wmiconfig_keep_awake_set(A_UINT32 time_ms)
{
	qcom_set_keep_awake_time(time_ms);
}

#if 0
void 
swat_wmiconfig_pwm_start(A_UINT32 flag)
{
	qcom_pwm_control(flag);
}
void
swat_wmiconfig_pwm_select(A_UINT32 pin)
{
	qcom_pwm_select_pin(pin);
}
void
swat_wmiconfig_pwm_deselect(A_UINT32 pin)
{
	qcom_pwm_deselect_pin(pin);
}
void
swat_wmiconfig_pwm_clock(A_UINT32 clock)
{
	qcom_pwm_set_clock(clock);
}
void
swat_wmiconfig_pwm_duty(A_UINT32 duty)
{
	qcom_pwm_set_duty_cycle(duty);
}
void
swat_wmiconfig_pwm_dump(void)
{
//	extern void qcom_pwm_dump(void);
	qcom_pwm_dump();
}
void
swat_wmiconfig_pwm_set(A_UINT32 freq, A_UINT32 duty)
{
	qcom_pwm_set(freq, duty);
}
void
swat_wmiconfig_pwm_sw_init(void)
{
	qcom_pwm_sw_init();
}
A_INT32
swat_wmiconfig_pwm_sw_create(A_UINT32 pin, A_UINT32 freq, A_UINT32 duty)
{
    return qcom_pwm_sw_create(pin, freq, duty);
}
void
swat_wmiconfig_pwm_sw_delete(A_INT32 id)
{
	qcom_pwm_sw_delete(id);
}
void
swat_wmiconfig_pwm_sw_config(A_INT32 id, A_UINT32 pin, A_UINT32 freq, A_UINT32 duty)
{
	qcom_pwm_sw_config(id, pin, freq, duty);
}
#endif
