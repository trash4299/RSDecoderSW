#include "xaxidma.h"
#include "xparameters.h"
#include "xdebug.h"

#if defined(XPAR_UARTNS550_0_BASEADDR)
#include "xuartns550_l.h"       /* to use uartns550 */
#endif

/******************** Constant Definitions **********************************/
#define DMA_DEV_ID		XPAR_AXIDMA_0_DEVICE_ID

#ifdef XPAR_AXI_7SDDR_0_S_AXI_BASEADDR
#define DDR_BASE_ADDR		XPAR_AXI_7SDDR_0_S_AXI_BASEADDR
#elif defined (XPAR_MIG7SERIES_0_BASEADDR)
#define DDR_BASE_ADDR	XPAR_MIG7SERIES_0_BASEADDR
#elif defined (XPAR_MIG_0_BASEADDR)
#define DDR_BASE_ADDR	XPAR_MIG_0_BASEADDR
#elif defined (XPAR_PSU_DDR_0_S_AXI_BASEADDR)
#define DDR_BASE_ADDR	XPAR_PSU_DDR_0_S_AXI_BASEADDR
#endif

#ifndef DDR_BASE_ADDR
#warning CHECK FOR THE VALID DDR ADDRESS IN XPARAMETERS.H, \
		 DEFAULT SET TO 0x80000000
#define MEM_BASE_ADDR		0x80000000
#else
#define MEM_BASE_ADDR		(DDR_BASE_ADDR + 0x1000000)
#endif

#define TX_BUFFER_BASE		(MEM_BASE_ADDR + 0x00100000)
#define RX_BUFFER_BASE		(MEM_BASE_ADDR + 0x00300000)

#define MAX_PKT_LEN		0x20

#define TEST_START_VALUE	0xC

/************************** Function Prototypes ******************************/
extern void xil_printf(const char *format, ...);

int XAxiDma_SimplePollExample(u16 DeviceId);

/************************** Variable Definitions *****************************/
XAxiDma AxiDma;
unsigned char testPacket[32] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,1,2,3,4,5,0,0,1,0,0,0};
unsigned char packetBack[32];

int main()
{
	int Status;

	xil_printf("\r\n--- Entering main() --- \r\n");

	/* Run the poll example for simple transfer */
	Status = XAxiDma_SimplePollExample(DMA_DEV_ID);

	if (Status != XST_SUCCESS) {
		xil_printf("XAxiDma_SimplePoll Example Failed\r\n");
		return XST_FAILURE;
	}

	xil_printf("Successfully ran XAxiDma_SimplePoll Example\r\n");

	xil_printf("--- Exiting main() --- \r\n");

	return XST_SUCCESS;

}

#if defined(XPAR_UARTNS550_0_BASEADDR)
static void Uart550_Setup(void)
{
	XUartNs550_SetBaud(XPAR_UARTNS550_0_BASEADDR,
			XPAR_XUARTNS550_CLOCK_HZ, 9600);

	XUartNs550_SetLineControlReg(XPAR_UARTNS550_0_BASEADDR,
			XUN_LCR_8_DATA_BITS);

}
#endif

// The example to do the simple transfer through polling. The constant
// NUMBER_OF_TRANSFERS defines how many times a simple transfer is repeated.

int XAxiDma_SimplePollExample(u16 DeviceId)
{
	XAxiDma_Config *CfgPtr;
	int Status;
//	int NumTransfers = 4;
	u8 *TxBufferPtr;
	u8 *RxBufferPtr;

	TxBufferPtr = (u8 *)TX_BUFFER_BASE ;
	RxBufferPtr = (u8 *)RX_BUFFER_BASE;

	/* Initialize the XAxiDma device.
	 */
	CfgPtr = XAxiDma_LookupConfig(DeviceId);
	if (!CfgPtr) {
		xil_printf("No config found for %d\r\n", DeviceId);
		return XST_FAILURE;
	}

	Status = XAxiDma_CfgInitialize(&AxiDma, CfgPtr);
	if (Status != XST_SUCCESS) {
		xil_printf("Initialization failed %d\r\n", Status);
		return XST_FAILURE;
	}

	if(XAxiDma_HasSg(&AxiDma)){
		xil_printf("Device configured as SG mode \r\n");
		return XST_FAILURE;
	}

	/* Disable interrupts, we use polling mode
	 */
	XAxiDma_IntrDisable(&AxiDma, XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DEVICE_TO_DMA);
	XAxiDma_IntrDisable(&AxiDma, XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DMA_TO_DEVICE);

	for(int i = 0; i < MAX_PKT_LEN; i++) {
			TxBufferPtr[i] = testPacket[i];
	}

	//for(i = 0; i < NumTransfers; i ++) {

		Status = XAxiDma_SimpleTransfer(&AxiDma,(UINTPTR) TxBufferPtr, MAX_PKT_LEN, XAXIDMA_DMA_TO_DEVICE);
		if (Status != XST_SUCCESS) {
			return XST_FAILURE;
		}
		while (XAxiDma_Busy(&AxiDma,XAXIDMA_DMA_TO_DEVICE)) {
			/* Wait */
		}

		Status = XAxiDma_SimpleTransfer(&AxiDma,(UINTPTR) RxBufferPtr, MAX_PKT_LEN, XAXIDMA_DEVICE_TO_DMA);
		if (Status != XST_SUCCESS) {
			return XST_FAILURE;
		}
		while (XAxiDma_Busy(&AxiDma,XAXIDMA_DEVICE_TO_DMA)) {
			/* Wait */
		}

		for(int i = 0; i < MAX_PKT_LEN; i++) {
			packetBack[i] = RxBufferPtr[i];
			xil_printf("%i: %c   ",i,RxBufferPtr[i]);
		}
	//}
	return XST_SUCCESS;
}
