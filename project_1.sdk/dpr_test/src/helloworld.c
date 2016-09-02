/******************************************************************************
*
* Copyright (C) 2009 - 2014 Xilinx, Inc.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* Use of the Software is limited solely to applications:
* (a) running on a Xilinx device, or
* (b) that interact with a Xilinx device through a bus or interconnect.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* XILINX  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
* OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
* Except as contained in this notice, the name of the Xilinx shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Xilinx.
*
******************************************************************************/

/*
 * helloworld.c: simple test application
 *
 * This application configures UART 16550 to baud rate 9600.
 * PS7 UART (Zynq) is not initialized by this application, since
 * bootrom/bsp configures it to baud rate 115200
 *
 * ------------------------------------------------
 * | UART TYPE   BAUD RATE                        |
 * ------------------------------------------------
 *   uartns550   9600
 *   uartlite    Configurable only in HW design
 *   ps7_uart    115200 (configured by bootrom/bsp)
 */
#include "support.h"
#include <stdio.h>
#include <stdlib.h>
#include "xil_printf.h"
#include "platform.h"
#include "xil_printf.h"
#include "my_cordic.h"
#include "xil_io.h"
#include "xil_exception.h"
#include "xdevcfg.h"
#include "xil_types.h"
#include "xscugic.h"
#include "xparameters.h"
#include "xil_cache.h"
#include "xsdps.h"
#include "ff.h"

#define SCALING_P0_25 0x009B74ED
#define SCALING_P0_18 0x000136E9
#define SCALING_P0_12 0x000004DB

#define DFX_1_P0_25 0x02000000
#define DFX_1_P0_18 0x00040000
#define DFX_1_P0_12 0x00001000



/************************** Function Prototypes ******************************/

void read_inputs(u32 *buff, char *filename);
void Write_outfile(u32 *out_buff, char *filename);
void cordic_highlevel(int function, u32 *ibuffer,u32 *obuffer, int n_words, u32 scaling_factor, u32 dfx_one  );

/************************** Global Variables ******************************/
 char filename1[]= "config_1.bin\0";
 char filename2[]= "config_2.bin\0";
 char filename3[]= "config_3.bin\0";

volatile int Interrupt_flag=0;
volatile u32 xout_data,yout_data,zout_data,data2;
int current_partial=1;
volatile int DmaDone=FALSE;
volatile int DmaPcapDone=FALSE;
volatile int FpgaProgrammed=FALSE;
FATFS FatFs;


int main()
{
	char * const in_file[] = { "isin1.bin\0", "icos1.bin\0", "iatan1.bin\0", "imod1.bin\0","isin2.bin\0", "icos2.bin\0", "iatan2.bin\0", "imod2.bin\0","isin3.bin\0", "icos3.bin\0", "iatan3.bin\0", "imod3.bin\0" };
	char * const out_file[] = { "osin1.bin\0", "ocos1.bin\0", "oatan1.bin\0", "omod1.bin\0","osin2.bin\0", "ocos2.bin\0", "oatan2.bin\0", "omod2.bin\0","osin3.bin\0", "ocos3.bin\0", "oatan3.bin\0", "omod3.bin\0" };
	FRESULT myres;
	int Status;
	int lk;
	int dpr_index;
	int funct_index;
	char str1[20];


    init_platform();
    Xil_DCacheDisable();
    Xil_ICacheDisable();
    print("Hello World\n\r");


//**** Memory Allocation ***///
	char *par_bitstream_1b = (char *)malloc(300000);
	if (par_bitstream_1b==NULL ){
		xil_printf("Failed to allocate memory for par_bitstream_1 buffer");
	} else {
		xil_printf("Successfully allocated memory for par_bitstream_1 buffer 0x%08x \r\n",  par_bitstream_1b);
	}
	char *par_bitstream_2b = (char *)malloc(300000);
	if (par_bitstream_2b==NULL ){
		xil_printf("Failed to allocate memory for par_bitstream_2 buffer");
	} else {
		xil_printf("Successfully allocated memory for par_bitstream_2 buffer 0x%08x \r\n",  par_bitstream_2b);
	}
	char *par_bitstream_3b = (char *)malloc(300000);
	if (par_bitstream_3b==NULL ){
		xil_printf("Failed to allocate memory for par_bitstream_3 buffer");
	} else {
		xil_printf("Successfully allocated memory for par_bitstream_3 buffer 0x%08x \r\n",  par_bitstream_3b);
	}
	u32 *input_vec = (u32 *)malloc(sizeof(u32)*200);
	if (input_vec==NULL){
		xil_printf("Failed to allocate memory for input_vec buffer");
	} else {
		xil_printf("Successfully allocated memory for input_vec buffer 0x%08x \r\n",  input_vec);
	}
	u32 *output_vec = (u32 *)malloc(sizeof(u32)*200);
	if (output_vec==NULL){
		xil_printf("Failed to allocate memory for output_vec buffer");
	} else {
		xil_printf("Successfully allocated memory for output_vec buffer 0x%08x \r\n",  output_vec);
	}
//**** End Memory Allocation ***///



	 printf("Enter name: ");
	 scanf("%s", str1);
	//**** Drive Mount ***///
	myres= f_mount(&FatFs, "0:/", 1);
	 if (myres == FR_OK)  {
		 xil_printf("Drive was mounted successfully \r\n");
	 }else{
		 xil_printf("Fail to mount driver\r\n");
	 }
	 //**** Partial bistreamst -->> RAM ***///

    ReadBitstream( filename1,  par_bitstream_1b, 0);
    ReadBitstream( filename2,  par_bitstream_2b, 0);
    ReadBitstream( filename3,  par_bitstream_3b, 0);
    /**** Generic Interrupt Controller Intialization ***///
    Status = ScuGic_initialization(INTC_DEVICE_ID);
    if(Status!=XST_SUCCESS){
    	xil_printf("GIC Example Test Failed \r\n");
    	return XST_FAILURE;
    	}
    xil_printf("GIC Successfully Configuration\r\n");
    /**** DPR with random partial ***///
    Do_DPR2(3, par_bitstream_1b, par_bitstream_2b, par_bitstream_3b);

    /**** MAIN TESTING LOOP (3 partials, 4 function/ivector(200) for each) ***///
 for (dpr_index=1; dpr_index<4; dpr_index++){
	 Do_DPR2(dpr_index, par_bitstream_1b, par_bitstream_2b, par_bitstream_3b);

	 for (funct_index=1; funct_index<5; funct_index++){
		 read_inputs(input_vec,in_file[dpr_index*4 +funct_index-5] );
		 if (funct_index==1){
		     cordic_highlevel(1, input_vec , output_vec, 200, SCALING_P0_25, DFX_1_P0_25  );
		 } else if (funct_index==2) {
		 	cordic_highlevel(1, input_vec , output_vec, 200, SCALING_P0_18, DFX_1_P0_18  );
		 } else if (funct_index==3){
		 	cordic_highlevel(1, input_vec , output_vec, 200, SCALING_P0_12, DFX_1_P0_12  );
		 } else {
			xil_printf("Error not a valid configuration \r\n");
		 }
		 Write_outfile(output_vec,out_file[dpr_index*4 +funct_index-5]);
	 }
 }


    free(par_bitstream_1b);
    free(par_bitstream_2b);
    free(par_bitstream_3b);
    free(input_vec);
    free(output_vec);
    xil_printf("goodbye \r\n");
    cleanup_platform();
    return 0;
}

/*
 * **** cordic_highlevel **
 * This function provides high level interaction with the circular cordic IP
 *
 * INPUT PARAMETERS:
 *  1. Function
 *  2. Input data (pointer to ibuffer)
 *  3. Output data (pointer to obuffer)
 *  4. Number of words
 *  5. Scaling factor
 *
 * Function
 * 	1. Sin
 *  2. Cos
 *  3. Atan
 *  4. Module
 */

void cordic_highlevel(int function, u32 *ibuffer,u32 *obuffer, int n_words, u32 scaling_factor, u32 dfx_one  ){
	int lk;

    for (lk=0; lk<n_words; lk++){

	    	Interrupt_flag=0;


	    	data2= MY_CORDIC_mReadReg( XPAR_MY_CORDIC_0_S00_AXI_BASEADDR, MY_CORDIC_S00_AXI_SLV_REG1_OFFSET );
	    	while( ((data2&0x2)==0x2) ){ //check if cordic is busy
	        	data2= MY_CORDIC_mReadReg( XPAR_MY_CORDIC_0_S00_AXI_BASEADDR, MY_CORDIC_S00_AXI_SLV_REG1_OFFSET );
	        	printf("inside\r\n");
	    	}
	    	if (function==1){// Sine
		    	MY_CORDIC_mWriteReg(XPAR_MY_CORDIC_0_S00_AXI_BASEADDR, MY_CORDIC_S00_AXI_SLV_REG2_OFFSET,scaling_factor   );
		    	MY_CORDIC_mWriteReg(XPAR_MY_CORDIC_0_S00_AXI_BASEADDR, MY_CORDIC_S00_AXI_SLV_REG3_OFFSET,0x00000000   );
		    	MY_CORDIC_mWriteReg(XPAR_MY_CORDIC_0_S00_AXI_BASEADDR, MY_CORDIC_S00_AXI_SLV_REG4_OFFSET, *ibuffer++   );
		    	MY_CORDIC_mWriteReg(XPAR_MY_CORDIC_0_S00_AXI_BASEADDR, MY_CORDIC_S00_AXI_SLV_REG0_OFFSET,0x00000001   );

	    	} else if (function==2){ //Cos
		    	MY_CORDIC_mWriteReg(XPAR_MY_CORDIC_0_S00_AXI_BASEADDR, MY_CORDIC_S00_AXI_SLV_REG2_OFFSET,0x00000000);
		    	MY_CORDIC_mWriteReg(XPAR_MY_CORDIC_0_S00_AXI_BASEADDR, MY_CORDIC_S00_AXI_SLV_REG3_OFFSET,scaling_factor);
		    	MY_CORDIC_mWriteReg(XPAR_MY_CORDIC_0_S00_AXI_BASEADDR, MY_CORDIC_S00_AXI_SLV_REG4_OFFSET, *ibuffer++   );
		    	MY_CORDIC_mWriteReg(XPAR_MY_CORDIC_0_S00_AXI_BASEADDR, MY_CORDIC_S00_AXI_SLV_REG0_OFFSET,0x00000001   );

	    	} else if (function==3){ //Atan
	    		MY_CORDIC_mWriteReg(XPAR_MY_CORDIC_0_S00_AXI_BASEADDR, MY_CORDIC_S00_AXI_SLV_REG2_OFFSET,*ibuffer++);
	    		MY_CORDIC_mWriteReg(XPAR_MY_CORDIC_0_S00_AXI_BASEADDR, MY_CORDIC_S00_AXI_SLV_REG3_OFFSET,dfx_one);
	    		MY_CORDIC_mWriteReg(XPAR_MY_CORDIC_0_S00_AXI_BASEADDR, MY_CORDIC_S00_AXI_SLV_REG4_OFFSET, 0x00000000  );
	    		MY_CORDIC_mWriteReg(XPAR_MY_CORDIC_0_S00_AXI_BASEADDR, MY_CORDIC_S00_AXI_SLV_REG0_OFFSET,0x00000003   );
	    	} else { //Module
	    		MY_CORDIC_mWriteReg(XPAR_MY_CORDIC_0_S00_AXI_BASEADDR, MY_CORDIC_S00_AXI_SLV_REG2_OFFSET,*ibuffer);
	    		MY_CORDIC_mWriteReg(XPAR_MY_CORDIC_0_S00_AXI_BASEADDR, MY_CORDIC_S00_AXI_SLV_REG3_OFFSET,*ibuffer++);
	    		MY_CORDIC_mWriteReg(XPAR_MY_CORDIC_0_S00_AXI_BASEADDR, MY_CORDIC_S00_AXI_SLV_REG4_OFFSET, 0x00000000  );
	    		MY_CORDIC_mWriteReg(XPAR_MY_CORDIC_0_S00_AXI_BASEADDR, MY_CORDIC_S00_AXI_SLV_REG0_OFFSET,0x00000003   );


	    	}
	    	while(Interrupt_flag==0); //while for cordic's interruption
	    	if (function==1){// Sine
	    		*obuffer=(unsigned int)yout_data;
	    		printf("Word output_vec %d: 0x%08x\n",lk, (unsigned int)*obuffer++ );

	    	} else if (function==2){ //Cos
	    		*obuffer=(unsigned int)yout_data;
	    		printf("Word output_vec %d: 0x%08x\n",lk, (unsigned int)*obuffer++ );

	    	} else if (function==3){ //Atan
	    		*obuffer=(unsigned int)zout_data;
	    		printf("Word output_vec %d: 0x%08x\n",lk, (unsigned int)*obuffer++ );

	    	} else { //Module
	    		*obuffer=(unsigned int)xout_data;
	    		printf("Word output_vec %d: 0x%08x\n",lk, (unsigned int)*obuffer++ );

	    	}
    }

}



void Write_outfile(u32 *out_buff, char *filename){
	FIL fil;
	FRESULT myres;
	UINT NumBytesWritten;

		   myres = f_open(&fil, filename , FA_CREATE_ALWAYS | FA_WRITE | FA_READ);
		   	   if(myres== FR_OK){
		   		   xil_printf("file created\n\r");
		   		   myres = f_lseek(&fil, 0);
		   		   myres = f_write(&fil, (const void*)out_buff,( sizeof(*out_buff)*200),&NumBytesWritten);
		   			 	 if (myres== FR_OK) {
		   			 		 xil_printf("data written\n\r");
		   			 	 }else {
		   			 		 xil_printf("failed to write data\n\r");
		   			 	 }

		   	   } else {
		   		   xil_printf("failed to create file\n\r");
		   	   }
		    myres = f_close(&fil);
		   	if (myres == FR_OK){
		   		xil_printf("file closed\n\r");
		   	 }
}



void read_inputs(u32 *buff, char *filename) {
	unsigned int jl=0;
	FIL fil;
	char line[4];
	FRESULT myres;
	char temp1;
	u32 filesize=0;
	 FATFS *fs;
	 UINT br;
		 myres = f_open(&fil, filename, FA_READ);
		 	 if (myres == FR_OK) {
		 		 printf("f_open \r\n");
		 		 filesize= file_size(&fil);
		 		 printf("File: input_sin_conf1.txt opened successfully \r\n" );

		 		 myres = f_lseek(&fil, 0 );

		 		 for (jl=0;jl<200; jl++ ){
		 			 myres =  f_read(&fil, (void *)line, 4, &br);
		 			temp1=line[0];
		 			line[0]=line[3];
		 			line[3]=temp1;
		 			temp1=line[1];
		 			line[1]=line[2];
		 			line[2]=temp1;
		 			memcpy ( &buff[jl], &line[0], 4 );
		 			printf("Word %d: 0x%08x\n",jl, (unsigned int)buff[jl] );
		 		 }
		 	 }
	// }
	 myres = f_close(&fil);

}


