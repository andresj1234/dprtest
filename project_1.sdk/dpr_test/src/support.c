/*
 * support.c
 *
 *  Created on: Aug 30, 2016
 *      Author: andres
 */

#include "support.h"

int SetUpInterruptSystem(XScuGic *XScuGicInstancePtr){

	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
			(Xil_ExceptionHandler)XScuGic_InterruptHandler,
			XScuGicInstancePtr);


	Xil_ExceptionEnable();
	return XST_SUCCESS;

}
int ReadBitstream(char *file_name,  char *buffer, int verbose){
	unsigned int jl=0;
	FIL fil;
	char line[11];
	FRESULT myres;
	DWORD fre_clust, fre_sect, tot_sect;
	char vol_str[24];
	u32 filesize=0;
	 FATFS *fs;
	 UINT br;


	// myres= f_mount(&FatFs, "0:/", 1);

	 //if (myres == FR_OK)  {
		 /*
		 if (verbose==1){
		 f_getlabel("", vol_str, 0);
		 printf("Volume Label %s \r\n", vol_str);

		  myres = f_getfree("1:", &fre_clust, &fs);

		 tot_sect = (fs->n_fatent - 2) * fs->csize;
		 fre_sect = fre_clust * fs->csize;

		 printf("%10lu KiB Total Drive Capacity.\n%10lu KiB Available.\n",
		 tot_sect / 2, fre_sect / 2);
		 }*/
		 myres = f_open(&fil, file_name, FA_READ);
		 	 if (myres == FR_OK) {
		 		 filesize= file_size(&fil);
		 		 printf("File: %s opened successfully \\r\n File Size: %d bytes \r\n", file_name,filesize );

		 		 myres = f_lseek(&fil, 0 );

		 		 for (jl=0;jl<(filesize/8); jl++ ){
		 			 myres =  f_read(&fil, (void *)line, 8, &br);
		 			 memcpy ( &buffer[jl*8], &line[0], 8 );

		 		}

	  			 xil_printf("Bitstream mapped in mem address: 0x%08x\r\n", &buffer);


		 	 } else {
		 		 printf("Fail to open file");
		 	 }


	// } else {
	//	 printf("Fail to mount media");
	// }

	 myres = f_close(&fil);

return XST_SUCCESS;
}

int ScuGic_initialization(u16 DeviceID){
	int Status;

	GicConfig = XScuGic_LookupConfig(DeviceID);
	if (NULL== GicConfig){
		return XST_FAILURE;
	}

	Status = XScuGic_CfgInitialize(&InterruptController, GicConfig,
				GicConfig->CpuBaseAddress);
	if (Status != XST_SUCCESS){
		return XST_FAILURE;
	}

	Status = XScuGic_SelfTest(&InterruptController);
	if (Status != XST_SUCCESS){
		return XST_FAILURE;
	}
	XScuGic_CPUWriteReg(&InterruptController, XSCUGIC_EOI_OFFSET, INTC_DEVICE_INT_ID);

	Status = SetUpInterruptSystem(&InterruptController);
	if (Status != XST_SUCCESS){
		return XST_FAILURE;
	}
	Status = XScuGic_Connect(&InterruptController, INTC_DEVICE_INT_ID,
			(Xil_ExceptionHandler)DeviceDriverHandler,
			(void *)&InterruptController  );

	if (Status != XST_SUCCESS){
		return XST_FAILURE;
	}
	Status = XScuGic_Connect(&InterruptController, DCFG_INTR_ID,
			(Xil_ExceptionHandler)DcfgIntrHandler,
			(void *)&InterruptController  );


	XScuGic_Enable(&InterruptController, INTC_DEVICE_INT_ID);
	return XST_SUCCESS;

}


 static void DcfgIntrHandler(void *CallBackRef, u32 IntrStatus)
{
	if (IntrStatus & XDCFG_IXR_DMA_DONE_MASK) {
		DmaDone = TRUE;
	}

	if (IntrStatus & XDCFG_IXR_D_P_DONE_MASK) {
		DmaPcapDone = TRUE;
	}

	if (IntrStatus & XDCFG_IXR_PCFG_DONE_MASK) {
		/*
		 * Disable PCFG DONE interrupt as this bit will remain set and will
		 * cause continuous interrupts.
		 */
		XDcfg_IntrDisable(&DcfgInstance, XDCFG_IXR_PCFG_DONE_MASK);
		FpgaProgrammed = TRUE;
	}
}
int XDcfg_Setup_Transfer(XScuGic *IntcInstPtr, XDcfg * DcfgInstPtr,
				u16 DeviceId, u16 DcfgIntrId, u32 *bitstream)
{
	int Status;
	u32 IntrStsReg = 0;
	u32 StatusReg;
	u32 PartialCfg = 0;

	XDcfg_Config *ConfigPtr;

	xil_printf("XDcfg_Setup_Transfer par_bitstream buffer 0x%08x \r\n",   (u32 *)bitstream);

	//
	// Initialize the Device Configuration Interface driver.
	//
	ConfigPtr = XDcfg_LookupConfig(DeviceId);

	//
	// This is where the virtual address would be used, this example
	// uses physical address.
	//
	xil_printf("1 \r\n");
	Status = XDcfg_CfgInitialize(DcfgInstPtr, ConfigPtr,
					ConfigPtr->BaseAddr);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}
	xil_printf("2 \r\n");
	Status = XDcfg_SelfTest(DcfgInstPtr);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}


	xil_printf("3 \r\n");
	XScuGic_Enable(&InterruptController, DCFG_INTR_ID);

	DmaDone = FALSE;
	DmaPcapDone = FALSE;
	FpgaProgrammed = FALSE;

	//
    // Check first time configuration or partial reconfiguration
	//
	xil_printf("4 \r\n");
	IntrStsReg = XDcfg_IntrGetStatus(DcfgInstPtr);
	if (IntrStsReg & XDCFG_IXR_DMA_DONE_MASK) {
		PartialCfg = 1;
	}
	PartialCfg = 1;
	if (PartialCfg==1){
		xil_printf("IS PARTIAL \r\n");
	} else {
		xil_printf("NOT Partial \r\n");
	}
	//
	// Enable the pcap clock.
	//
	xil_printf("5 \r\n");
	StatusReg = Xil_In32(SLCR_PCAP_CLK_CTRL);
	if (!(StatusReg & SLCR_PCAP_CLK_CTRL_EN_MASK)) {
		Xil_Out32(SLCR_UNLOCK, SLCR_UNLOCK_VAL);
		Xil_Out32(SLCR_PCAP_CLK_CTRL,
				(StatusReg | SLCR_PCAP_CLK_CTRL_EN_MASK));
		Xil_Out32(SLCR_UNLOCK, SLCR_LOCK_VAL);
	}

	//
	// Disable the level-shifters from PS to PL.
	//
	xil_printf("6 \r\n");
	if (!PartialCfg) {
		Xil_Out32(SLCR_UNLOCK, SLCR_UNLOCK_VAL);
		Xil_Out32(SLCR_LVL_SHFTR_EN, 0xA);
		Xil_Out32(SLCR_LOCK, SLCR_LOCK_VAL);
	}

	//
	// Select PCAP interface for partial reconfiguration
	//
	xil_printf("7 \r\n");
	if (PartialCfg) {
		XDcfg_EnablePCAP(DcfgInstPtr);
		XDcfg_SetControlRegister(DcfgInstPtr, XDCFG_CTRL_PCAP_PR_MASK);
	}

	//
	//Clear the interrupt status bits
	//
	xil_printf("8 \r\n");
	XDcfg_IntrClear(DcfgInstPtr, (XDCFG_IXR_PCFG_DONE_MASK |
					XDCFG_IXR_D_P_DONE_MASK |
					XDCFG_IXR_DMA_DONE_MASK));

	// Check if DMA command queue is full */
	StatusReg = XDcfg_ReadReg(DcfgInstPtr->Config.BaseAddr,
				XDCFG_STATUS_OFFSET);
	xil_printf("9 \r\n");
	if ((StatusReg & XDCFG_STATUS_DMA_CMD_Q_F_MASK) ==
			XDCFG_STATUS_DMA_CMD_Q_F_MASK) {
		return XST_FAILURE;
	}
	xil_printf("10 \r\n");
	///
	// Enable the DMA done, DMA_PCAP Done and PCFG Done interrupts.
    //
	XDcfg_IntrEnable(DcfgInstPtr, (XDCFG_IXR_DMA_DONE_MASK |
					XDCFG_IXR_D_P_DONE_MASK |
					XDCFG_IXR_PCFG_DONE_MASK));
	//
	//Download bitstream in non secure mode
	//
	xil_printf("11 \r\n");
	XDcfg_Transfer(DcfgInstPtr, (u32 *)(bitstream),
			BIT_STREAM_SIZE_WORDS,
			(u8 *)XDCFG_DMA_INVALID_ADDRESS,
			0, XDCFG_NON_SECURE_PCAP_WRITE);
	xil_printf("12 \r\n");
	while (!DmaDone);
	xil_printf("13 \r\n");
	if (PartialCfg) {
		while (!DmaPcapDone);



	} else {
		while (!FpgaProgrammed);
		//
		//  Enable the level-shifters from PS to PL.
		 //
		xil_printf("14 \r\n");
		Xil_Out32(SLCR_UNLOCK, SLCR_UNLOCK_VAL);
		Xil_Out32(SLCR_LVL_SHFTR_EN, 0xF);
		Xil_Out32(SLCR_LOCK, SLCR_LOCK_VAL);
	}

	Status = XST_SUCCESS;
	xil_printf("16 \r\n");
	XDcfg_IntrDisable(DcfgInstPtr, (XDCFG_IXR_DMA_DONE_MASK |
					XDCFG_IXR_D_P_DONE_MASK |
					XDCFG_IXR_PCFG_DONE_MASK));

	xil_printf("17 \r\n");
	XScuGic_Disable(IntcInstPtr, DcfgIntrId);
	xil_printf("18 \r\n");


	return Status;
}


int Do_DPR2(int config, char *bitstream_1, char *bitstream_2, char *bitstream_3){
	int Status=0;


	xil_printf("Do_DPR par_bitstream_1 buffer 0x%08x \r\n",   (u32 *)bitstream_1);
	xil_printf("Do_DPR par_bitstream_2 buffer 0x%08x \r\n",   (u32 *)bitstream_2);
	xil_printf("Do_DPR par_bitstream_3 buffer 0x%08x \r\n",   (u32 *)bitstream_3);

	if (config==1){
	  Status =  XDcfg_Setup_Transfer(&InterruptController, &DcfgInstance,DCFG_DEVICE_ID, DCFG_INTR_ID,   (u32 *) bitstream_1);
	  current_partial=1;
	} else if (config==2){
     Status =  XDcfg_Setup_Transfer(&InterruptController, &DcfgInstance,DCFG_DEVICE_ID, DCFG_INTR_ID,   (u32 *)bitstream_2);
     current_partial=2;
	} else if (config==3) {
     Status =  XDcfg_Setup_Transfer(&InterruptController, &DcfgInstance,DCFG_DEVICE_ID, DCFG_INTR_ID,   (u32 *) bitstream_3);
     current_partial=3;
	} else {
	xil_printf("Not conf available\r\n");
	}

	return Status;
}

void DeviceDriverHandler(void *CallbackRef){
	xil_printf("ISR:\r\n");

	xout_data = MY_CORDIC_mReadReg( XPAR_MY_CORDIC_0_S00_AXI_BASEADDR, MY_CORDIC_S00_AXI_SLV_REG1_OFFSET );

	xout_data= xout_data&(~0x1);
    MY_CORDIC_mWriteReg(XPAR_MY_CORDIC_0_S00_AXI_BASEADDR, MY_CORDIC_S00_AXI_SLV_REG1_OFFSET,xout_data   );

    xout_data = MY_CORDIC_mReadReg( XPAR_MY_CORDIC_0_S00_AXI_BASEADDR, MY_CORDIC_S00_AXI_SLV_REG1_OFFSET );
    xil_printf("ISR-Cleared- Reg1 Status: 0x%08x\n", xout_data);

    xout_data = MY_CORDIC_mReadReg( XPAR_MY_CORDIC_0_S00_AXI_BASEADDR, MY_CORDIC_S00_AXI_SLV_REG5_OFFSET );
    yout_data = MY_CORDIC_mReadReg( XPAR_MY_CORDIC_0_S00_AXI_BASEADDR, MY_CORDIC_S00_AXI_SLV_REG6_OFFSET );
    zout_data = MY_CORDIC_mReadReg( XPAR_MY_CORDIC_0_S00_AXI_BASEADDR, MY_CORDIC_S00_AXI_SLV_REG7_OFFSET );

	Interrupt_flag =1;



}
