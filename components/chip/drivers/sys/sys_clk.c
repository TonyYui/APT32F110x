/***********************************************************************//** 
 * \file  sys_clk.c
 * \brief  system clock management for cpu clock(HCLK) and peri clock(PCLK)
 * \copyright Copyright (C) 2015-2020 @ APTCHIP
 * <table>
 * <tr><th> Date  <th>Version  <th>Author  <th>Description
 * <tr><td> 2020-8-10 <td>V0.0  <td>WNN   <td>initial
 * <tr><td> 2021-5-13 <td>V0.0  <td>ZJY   <td>initial
 * </table>
 * *********************************************************************
*/
#include <stdint.h>
#include <sys_clk.h>
#include <pin.h>
#include "board_config.h"

#include <csp.h>

//extern system_clk_config_t g_tSystemClkConfig[];


static uint32_t s_wTmLoad = 0,s_wClkDivn = 0;

///to match the real div to reg setting
static const uint32_t s_wHclkDiv[] = {
	1, 1, 2, 3, 4, 5, 6, 8, 12, 16, 24, 32, 36, 64, 128, 256
};

static uint32_t apt_get_hclk(void)
{
	uint32_t tRslt;
	tRslt = tClkConfig.wSclk;
	return (tRslt);
}

/** \brief sysctem clock (HCLK) configuration
 * 
 *  To set CPU frequence according to tClkConfig
 * 
 *  \param[in] none.
 *  \return csi_error_t.
 */ 
csi_error_t csi_sysclk_config(csi_clk_config_t tClkCfg)
{	csi_error_t ret = CSI_OK;
	uint8_t byFreqIdx = 0;
	uint32_t wFreq,wTargetSclk;
	cclk_src_e eSrc;
	uint8_t byFlashLp = 0;
	wFreq = tClkCfg.wFreq;
	
	wTargetSclk = wFreq/s_wHclkDiv[tClkCfg.eSdiv];
	eSrc = tClkCfg.eClkSrc;
	csp_eflash_lpmd_enable(SYSCON, 0);					//disable Flash LP Mode					
	
	switch (eSrc)
	{
		case (SRC_ISOSC): 	
			csi_isosc_enable();
			byFlashLp = 1;
			break;
		case (SRC_IMOSC):	
			switch (wFreq) 	
			{
				case (IMOSC_5M_VALUE):   byFreqIdx = 0;
					break;
				case (IMOSC_4M_VALUE):   byFreqIdx = 1;
					break;
				case (IMOSC_2M_VALUE):   byFreqIdx = 2;
					break;
				case (IMOSC_131K_VALUE): byFreqIdx = 3;
					break;
				default: ret = CSI_ERROR;	
					break;
			}
			csi_imosc_enable(byFreqIdx);
		
			if (wFreq == IM_131K)
				byFlashLp = 1;
			break;
		case (SRC_EMOSC):	
//			csi_pin_set_mux(PA03, PA03_OSC_XI);
//			csi_pin_set_mux(PA04, PA04_OSC_XO);
			if (wFreq == EMOSC_32K_VALUE)
				csp_set_em_lfmd(SYSCON, 1);
			csi_emosc_enable(wFreq);
			break;
		case (SRC_HFOSC):	
			switch (wFreq) 	
			{
				case (HFOSC_48M_VALUE): byFreqIdx = 0;
					break;
				case (HFOSC_24M_VALUE): byFreqIdx = 1;
					break;
				case (HFOSC_12M_VALUE): byFreqIdx = 2;
					break;
				case (HFOSC_6M_VALUE):  byFreqIdx = 3;
					break;
				default: ret = CSI_ERROR;
					return ret;
					break;
			}
			csi_hfosc_enable(byFreqIdx);
			break;
		case(SRC_ESOSC):
// 			csi_pin_set_mux(PA01, PA01_OSC_SXI);
//			csi_pin_set_mux(PA02, PA02_OSC_SXO);   // Config pins before use ESOSC 
			csi_esosc_enable(wFreq);
			byFlashLp = 1;
			break;
		default: 
			break;
	}
	
	if (wTargetSclk >= 16000000) {
		IFC->CEDR = IFC_CLKEN;
		if (wTargetSclk >= 24000000)
			IFC->MR = (IFC->MR & (~PF_SPEED_MSK) & (~PF_WAIT_MSK)) | HIGH_SPEED | PF_WAIT2;
		else 
			IFC->MR = (IFC->MR & (~PF_SPEED_MSK) & (~PF_WAIT_MSK) )| HIGH_SPEED | PF_WAIT1;
		csp_set_sdiv(SYSCON, tClkCfg.eSdiv);
		csp_set_clksrc(SYSCON, eSrc);
	}
	else {
		csp_set_sdiv(SYSCON, tClkCfg.eSdiv);
		csp_set_clksrc(SYSCON, eSrc);
		IFC->CEDR = IFC_CLKEN;
		IFC->MR = ((IFC->MR & (~PF_SPEED_MSK)) & (~PF_WAIT_MSK)) |LOW_SPEED | PF_WAIT0;
	}
	
	
	csp_eflash_lpmd_enable(SYSCON, (bool)byFlashLp);
	
	csp_set_pdiv(SYSCON, tClkCfg.ePdiv);
	
	//update wSclk and wPclk in tClkConfig
	tClkConfig.wSclk = wTargetSclk;
	tClkConfig.wPclk = tClkConfig.wSclk/(0x1<<tClkCfg.ePdiv);
	return ret;
}

/** \brief Clock output configuration
 * 
 *  \param[in] eCloSrc: source to output
 *  \param[in] eCloDiv: clo divider 
 *  \return csi_error_t.
 */
csi_error_t csi_clo_config(clo_src_e eCloSrc, clo_div_e eCloDiv)
{ 	
	csi_error_t ret = CSI_OK;
	csp_set_clo_src(SYSCON, eCloSrc);
	csp_set_clo_div(SYSCON, eCloDiv);
	return ret;
}

/** \brief prei clk enable in SYSCON level
 *
 *  \param[in] wModule: module name
 *  \return none
 */
void soc_clk_enable(int32_t wModule)
{
    //TODO
	if(wModule < 32U)
		csp_pcer0_clk_en(SYSCON, (uint32_t)wModule);
	else
		csp_pcer1_clk_en(SYSCON, (uint32_t)wModule - 32U);
}

/** \brief prei clk disable in SYSCON level
 *
 *  \param[in] wModule: module name
 *  \return none
 */
void soc_clk_disable(int32_t wModule)
{
    //TODO
	if(wModule < 32U)
		csp_pder0_clk_dis(SYSCON, (uint32_t)wModule);
	else
		csp_pder1_clk_dis(SYSCON, (uint32_t)wModule - 32U);
}
/** \brief to calculate SCLK and PCLK frequence according to the current reg content
 *  tClkConfig.wSclk and tClkConfig.wPclk will be updated after excuting this function
 *  \param[in] none.
 *  \return csi_error_t.
 */
csi_error_t csi_calc_clk_freq(void)
{
	cclk_src_e eClkSrc;
	uint8_t  byHclkDiv;
	uint32_t wHfoFreq;
	uint32_t wImoFreq;
	
	//calculate sclk
	{
		eClkSrc = ((cclk_src_e) csp_get_clksrc(SYSCON));
		switch(eClkSrc)
		{ 	case (SRC_ISOSC): 	
				tClkConfig.wSclk = ISOSC_VALUE;
				break;
			case (SRC_EMOSC): 	
				tClkConfig.wSclk = EMOSC_VALUE;
				break;
			case (SRC_IMOSC):	
				wImoFreq = csp_get_imosc_fre(SYSCON);
				switch (wImoFreq)
				{
					case (0): 
						tClkConfig.wSclk = IMOSC_5M_VALUE;
						break;
					case (1): 
						tClkConfig.wSclk = IMOSC_4M_VALUE;
						break;
					case (2): 
						tClkConfig.wSclk = IMOSC_2M_VALUE;	
						break;
					case (3): 
						tClkConfig.wSclk = IMOSC_131K_VALUE;	
						break;
					default: 
						return CSI_ERROR;	
						break;
				}
				break;
			case  (SRC_HFOSC):	
				wHfoFreq =  csp_get_hfosc_fre(SYSCON);
				switch (wHfoFreq)
				{
					case (0): 
						tClkConfig.wSclk = HFOSC_48M_VALUE;
						break;
					case (1): 
						tClkConfig.wSclk = HFOSC_24M_VALUE;
						break;
					case (2): 
						tClkConfig.wSclk = HFOSC_12M_VALUE;	
						break;
					case (3): 
						tClkConfig.wSclk = HFOSC_6M_VALUE;	
						break;
					default:  
						return CSI_ERROR;	
						break;
				}
				break;
			case (SRC_ESOSC):
				tClkConfig.wSclk = ESOSC_VALUE;
				break;		
			default:
				return CSI_ERROR;
				break;
		}
		byHclkDiv = csp_get_hclk_div(SYSCON);

		tClkConfig.wSclk = tClkConfig.wSclk/s_wHclkDiv[byHclkDiv];
	}
	
	//calculate pclk
	{
		uint32_t wPdiv = 1;
		uint32_t wSclk = csi_get_sclk_freq();
		wPdiv = csp_get_pdiv(SYSCON);
		
		if(wPdiv == 0) 
			tClkConfig.wPclk = wSclk;
		else           
			tClkConfig.wPclk = wSclk/(wPdiv<<1);
	}
	
	return CSI_OK;
}

/** \brief To get SCLK frequence 
 *  \param[in] none.
 *  \return csi_error_t.
 */ 
uint32_t csi_get_sclk_freq(void)
{	
	return tClkConfig.wSclk;
}

/** \brief To get PCLK frequence 
 *  \param[in] none.
 *  \return csi_error_t.
 */ 
uint32_t csi_get_pclk_freq(void)
{
	return tClkConfig.wPclk;
}

/** \brief To get CORET frequence.
 *  Make sure to excute soc_get_cpu_freq() after clock block changing
 *  \param[in] none.
 *  \return g_wSystemClk.
 */ 
uint32_t soc_get_coret_freq(void)
{
	switch ((CORET->CTRL & 0x4) >> 2)
	{
		case 0: return tClkConfig.wSclk/8;
			break;
		case 1: return tClkConfig.wSclk;
			break;
		default:
			return tClkConfig.wSclk;
			break;
	}
}
/** \brief to set clock status in PM mode 
 *  when IWDT is enabled, trying to stop ISOSC in stop mode would be invalid
 *  refer to GCER in SYSCON chapter for detailed description
 *  \param[in] eClk: clock to be configured
 *  \param[in] bEnable: enable or disable
 *  \return none.
 */ 
//void csi_clk_pm_enable(clk_pm_e eClk, bool bEnable)
//{
//	csp_clk_pm_enable(SYSCON, eClk, bEnable);
//}

/** \brief       Soc get bt frequence.
 *  \param[in]   byIdx: id of bt
 *  \return      coret frequence
 */
uint32_t soc_get_bt_freq(uint8_t byIdx)
{
	csp_bt_t *bt_base  = NULL;
	switch(byIdx)
	{
		case 0:
			bt_base = (csp_bt_t *)APB_BT0_BASE;
			break;
		case 1:
			bt_base = (csp_bt_t *)APB_BT1_BASE;
			break;
		default:
			return csi_get_pclk_freq();
	}
	
	return csi_get_pclk_freq()/(csp_bt_get_pscr(bt_base) + 1);
}

/** \brief       Soc get rtc frequence.
 *  \param[in]   none
 *  \return      rtc frequence
 */
uint32_t soc_get_rtc_freq(void)
{
	uint32_t wFre = ISOSC_VALUE;
	uint8_t byClk = csp_rtc_get_clksrc(RTC);
	
	switch(byClk)
	{
		case RTC_ISOSC:
			wFre = ISOSC_VALUE;
			break;
		case RTC_IMOSC_DIV4:
			switch(csp_get_imsrc(SYSCON))
			{
				case 0:
					wFre = IMOSC_5M_VALUE >> 2;
					break;
				case 1:
					wFre = IMOSC_4M_VALUE >> 2;
					break;
				case 2:
					wFre = IMOSC_2M_VALUE >> 2;
					break;
				case 3:
					wFre = IMOSC_131K_VALUE >> 2;
					break;
			}
			break;
		case RTC_ESOSC:
			wFre = ESOSC_VALUE;
			break;
		case RTC_EMOSC_DIV4:
			wFre =	EMOSC_VALUE >> 2;
			break;
		default:
			wFre = ISOSC_VALUE;
			break;
	}
	
	return wFre;
}

/** \brief       timer set load times out
 *  \param[in]   wTimeOut: the timeout, unit: us, 20us < wTimeOut < 3S
 *  \param[in]   byBitMode: TIMER_16BIT_MODE  or TIMER_24BIT_MODE
 *  \return      none
*/
void apt_timer_set_load_value(uint32_t wTimesOut,uint8_t byBitMode)
{
	uint32_t wDivisor,wPrdrLoadMax; 
	
	if(byBitMode == TIMER_16BIT_MODE) 
	{
		wDivisor     = TIMER_16BIT_DIVISOR;
		wPrdrLoadMax = TIMER_16BIT_MAX;
	}
	else
	{
		wDivisor     = TIMER_24BIT_DIVISOR;
		wPrdrLoadMax = TIMER_24BIT_MAX;		
	}
	if((csi_get_pclk_freq() % 1000000) == 0)
	{
		s_wClkDivn = csi_get_pclk_freq() / 1000000 * wTimesOut / wDivisor;		//timer clk div value
		if(s_wClkDivn == 0)
			s_wClkDivn  = 1;
		s_wTmLoad = csi_get_pclk_freq() / 1000000 * wTimesOut / s_wClkDivn;	//timer prdr load value
		if(s_wTmLoad > wPrdrLoadMax)
		{
			s_wClkDivn += 1;
			s_wTmLoad = csi_get_pclk_freq() / 1000000 * wTimesOut / s_wClkDivn ;	//timer prdr load value
		}			
	}
	else if((csi_get_pclk_freq() % 4000) <= 2000)                               //最大5556000 
	{
		s_wClkDivn = csi_get_pclk_freq() / 4000 * wTimesOut / 250 / wDivisor;		//timer clk div value
		if(s_wClkDivn == 0)
			s_wClkDivn  = 1;
		s_wTmLoad = csi_get_pclk_freq() / 4000 * wTimesOut / 250 / s_wClkDivn;	//timer prdr load value
		if(s_wTmLoad > wPrdrLoadMax)
		{
			s_wClkDivn += 1;
			s_wTmLoad = csi_get_pclk_freq() / 4000 * wTimesOut / 250 / s_wClkDivn ;	//timer prdr load value
		}				
	}
	else
	{
		s_wClkDivn = csi_get_pclk_freq() / 1000 * wTimesOut / 1000 / wDivisor;		//timer clk div value
		if(s_wClkDivn == 0)
			s_wClkDivn  = 1;
		s_wTmLoad = csi_get_pclk_freq() / 1000 * wTimesOut / 1000 / s_wClkDivn;	//timer prdr load value
		if(s_wTmLoad > wPrdrLoadMax)
		{
			s_wClkDivn += 1;
			s_wTmLoad = csi_get_pclk_freq() / 1000 * wTimesOut / 1000 / s_wClkDivn ;	//timer prdr load value
		}
	}
}

/** \brief       get timer prdr load value
 *  \return      load prdr value 
*/
uint32_t apt_timer_get_prdrload_value(void)
{
	return s_wTmLoad;
}

/** \brief       get timer clk div
 *  \return      clk div 
*/
uint32_t apt_timer_get_clkdiv_value(void)
{
	return s_wClkDivn;
}