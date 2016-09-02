/*
 * support.h
 *
 *  Created on: Aug 30, 2016
 *      Author: andres
 */


#include "xscugic.h"
#include "ff.h"
#include "xdevcfg.h"
#include "my_cordic.h"
#define INTC_DEVICE_ID XPAR_SCUGIC_0_DEVICE_ID
#define INTC_DEVICE_INT_ID 0x3D
#define DCFG_INTR_ID		XPAR_XDCFG_0_INTR
#define DCFG_DEVICE_ID		XPAR_XDCFG_0_DEVICE_ID

#define BIT_STREAM_LOCATION	0x00121178	/* Bitstream location */
#define BIT_STREAM_SIZE_WORDS	68364		/* Size in Words (32 bit)*/
#define TIMER_DEVICE_ID		XPAR_XSCUTIMER_0_DEVICE_ID
#define TIMER_LOAD_VALUE	0xFFFFFFFF
/*
 * SLCR registers
 */
#define SLCR_LOCK	0xF8000004 /**< SLCR Write Protection Lock */
#define SLCR_UNLOCK	0xF8000008 /**< SLCR Write Protection Unlock */
#define SLCR_LVL_SHFTR_EN 0xF8000900 /**< SLCR Level Shifters Enable */
#define SLCR_PCAP_CLK_CTRL XPAR_PS7_SLCR_0_S_AXI_BASEADDR + 0x168 /**< SLCR
					* PCAP clock control register address
					*/
#define SLCR_PCAP_CLK_CTRL_EN_MASK 0x1
#define SLCR_LOCK_VAL	0x767B
#define SLCR_UNLOCK_VAL	0xDF0D

/**** External variable declaration ***///
extern volatile int DmaDone;
extern volatile int DmaPcapDone;
extern volatile int FpgaProgrammed;
extern int current_partial;
extern volatile u32 xout_data,yout_data,zout_data,data2;
extern volatile int Interrupt_flag;

/**** Driver's structure definition ***///
XDcfg DcfgInstance;
XScuGic InterruptController;
//static XScuGic_Config *GicConfig;
XScuGic_Config *GicConfig;
/****ISRs ***///
void DeviceDriverHandler(void *CallbackRef);
static void DcfgIntrHandler(void *CallBackRef, u32 IntrStatus);

/**** Function prototypes ***///
int SetUpInterruptSystem(XScuGic *XScuGicInstancePtr);
int ReadBitstream(char *file_name,  char *buffer, int verbose);
int ScuGic_initialization(u16 DeviceID);

int XDcfg_Setup_Transfer(XScuGic *IntcInstPtr, XDcfg * DcfgInstPtr,
				u16 DeviceId, u16 DcfgIntrId, u32 *bitstream);
int Do_DPR2(int config, char *bitstream_1, char *bitstream_2, char *bitstream_3);
