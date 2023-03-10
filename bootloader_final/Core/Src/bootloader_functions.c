

/**************************INCLUDES**********************************/

#include "main.h"
#include "bootloader_functions.h"


/**************************DEFINES*************************************/:)

#define FLASH_APP_1_START		((uint32_t)0x08008000)
#define FLASH_APP_2_START 		((uint32_t)0x0800C000)
#define FLASH_APP1_END 			((uint32_t)0x0800BFFF)
#define FLASH_APP2_END 			((uint32_t)0x0800FFFF)
#define FLASH_BOOT 		 		((uint32_t)0x08000000)

#define PAGE_SIZE        		14336
// 1024 byte for write to FLASH, if we write 16kb full of sector then we should write 16 times

/*CRC will upload */
#define VERIFY_CRC_FAIL    	1
#define VERIFY_CRC_SUCCESS 	0

#define ADDR_VALID 0x00
#define ADDR_INVALID 0x01

#define INVALID_SECTOR 0x04

#define SRAM1_SIZE            112*1024     // STM32F407 has 112KB of SRAM1
#define SRAM1_END             (SRAM1_BASE + SRAM1_SIZE)
#define SRAM2_SIZE            16*1024     // STM32F407 has 16KB of SRAM2
#define SRAM2_END             (SRAM2_BASE + SRAM2_SIZE)
#define FLASH_SIZE             1024*1024     // stm32F407 has 1mb of Flash Size
#define BKPSRAM_SIZE           4*1024     // STM32F407 has 4KB of Backup SRAM2
#define BKPSRAM_END            (BKPSRAM_BASE + BKPSRAM_SIZE)



/************************* Definitions ***************************/

extern UART_HandleTypeDef huart3;
extern bootloader myBootloader;
volatile uint32_t *RX_Uploading_Data[PAGE_SIZE];
uint32_t *Temp_RX_Data[PAGE_SIZE];
volatile uint32_t bytecount=0;





/*************************Print Message Functions**************************/

void printmsg(char *format,...)
{
	char str[80];
	va_list args;
	va_start(args, format);
	vsprintf(str, format,args);
	HAL_UART_Transmit(&huart3,(uint8_t *)str, strlen(str),HAL_MAX_DELAY);
	va_end(args);
}

/*************************Bootloader Default Mode *************************/

void bootloader_default_mode(void)							//default mode blinky
{
	if( myBootloader.mainMenuState == Idle
		&& myBootloader.mainMenuActive == yes){
		printmsg("BL BOOTLOADER MSG: Hello from the default mode\r\n");
		while(1){

			HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_15);
			HAL_Delay(100);
		}
	}
}

/*************************Bootloader Jump to APP1 *************************/

void bootloader_jump_to_app1(void)
{
	// Check if user application is valid by verifying stack pointer

	if ((*(__IO uint32_t*)FLASH_APP_1_START) != 0xFFFFFFFF && myBootloader.mainMenuState == app1
			&& myBootloader.mainMenuActive == yes)
	{
		printmsg("BL BOOTLOADER MSG: Hello from the Jump APP1 \r\n");

		typedef void (*pFunction)(void);								//1. holding to reset handler address
		pFunction jump_to_app1;

		/* Jump to user application */
		uint32_t reset_handler_addr=*(__IO uint32_t*) (FLASH_APP_1_START + 4);				//2. Reset Handler = msp+4

		jump_to_app1=(pFunction)reset_handler_addr;							//3. new operation with reset handler

		/* Initialize user application's stack pointer */
		__set_MSP(*(__IO uint32_t*) FLASH_APP_1_START);							//4. flash jump adress = msp value

		/* Jump to user application */
		jump_to_app1();
	}
	else{
		printmsg("APP1 Error");
	}
}


/*************************Bootloader Jump to APP2 *************************/

void bootloader_jump_to_app2(void)
{

	// Check if user application is valid by verifying stack pointer

	if ((*(__IO uint32_t*)FLASH_APP_2_START) != 0xFFFFFFFF && myBootloader.mainMenuState == app2
			&& myBootloader.mainMenuActive == yes)
	{
		printmsg("BL BOOTLOADER MSG: Hello from the Jump APP2 \r\n");

		typedef void (*pFunction)(void);								//1. holding to reset handler address
		pFunction jump_to_app2;

		/* Jump to user application */
		uint32_t reset_handler_addr=*(__IO uint32_t*) (FLASH_APP_2_START + 4);				//2. Reset Handler = msp+4

		jump_to_app2=(pFunction)reset_handler_addr;							//3. new operation with reset handler

		/* Initialize user application's stack pointer */
		__set_MSP(*(__IO uint32_t*) FLASH_APP_2_START);							//4. flash jump adress = msp value

		/* Jump to user application */
		jump_to_app2();
	}
	else{
		printmsg("APP2 Error");
	}
}



/*************************Bootloader Execute Flash Erase *************************/

uint8_t execute_flash_erase(uint8_t sector_number,uint8_t number_of_sector)
{


		FLASH_EraseInitTypeDef flashErase_handle;
		uint32_t sectorError;								        //STM32F407 has 11 sectors [0 to 11]
													//number_of_sector has to be in the range of 0 to 11
													// if sector_number = 0xff , that means mass erase
		HAL_StatusTypeDef status;

		if(number_of_sector>11)
			return INVALID_SECTOR;
	if((sector_number==0xFF)||sector_number<=11)
	{
		if(sector_number==(uint8_t)0xFF)
		 {
			flashErase_handle.TypeErase=FLASH_TYPEERASE_MASSERASE;
		 }
		else
		 {

			uint8_t remaining_sector = 11 - sector_number;					/*calculating how many sectors needs to erased */

            if(number_of_sector>remaining_sector)
              {
        	    number_of_sector=remaining_sector;
              }

            flashErase_handle.TypeErase = FLASH_TYPEERASE_SECTORS;
            flashErase_handle.Sector = sector_number; 							// this is the initial sector
            flashErase_handle.NbSectors = number_of_sector;						//this is the number of sector that what you want how many sector to erase
	    }
		flashErase_handle.Banks=FLASH_BANK_1;							//*Get access to touch the flash registers

		HAL_FLASH_Unlock();
		flashErase_handle.VoltageRange = FLASH_VOLTAGE_RANGE_3; 				//  MCU will work on this voltage range
		status = (uint8_t) HAL_FLASHEx_Erase(&flashErase_handle, &sectorError);
		HAL_FLASH_Lock();

		return status;

	}
	return INVALID_SECTOR;


}

/*************************Bootloader Erase Flash Sector *************************/

void bootloader_erase_flash_sector(uint8_t sector_number1,uint8_t number_of_sector1)
{

	 uint8_t erase_status=0x00;
	 printmsg("BL_DEBUG_MSG: bootloader_erase_flash_sector !!\n");
	 erase_status=execute_flash_erase(sector_number1,number_of_sector1);
	 printmsg("BL_DEBUG_MSG: flash erase status: %#x\n",erase_status);				 //HARDFAULTA DÜŞÜYOR
	 	 if(erase_status == HAL_OK)
	     {
	         printmsg("\n  Erase Status: Success  Code: Flash_HAL_OK\n");
	     }
	     else if(erase_status == HAL_ERROR)
	     {
	    	 printmsg("\n  Erase Status: Fail  Code: Flash_HAL_ERROR\n");

	     }
	     else if(erase_status == HAL_BUSY)
	     {
	    	 printmsg("\n  Erase Status: Fail  Code: Flash_HAL_BUSY\n");
	     }
	     else if(erase_status == HAL_TIMEOUT)
	     {
	    	 printmsg("\n  Erase Status: Fail  Code: Flash_HAL_TIMEOUT\n");
	     }
	      else if(erase_status == INVALID_SECTOR)
	     {
	    	  printmsg("\n  Erase Status: Fail  Code: Flash_HAL_INV_SECTOR\n");
	     }
	     else
	     {
	         printmsg("\n  Erase Status: Fail  Code: UNKNOWN_ERROR_CODE\n");
	     }

}

/*************************Bootloader Get MCU Chip ID *************************/

uint16_t bootloader_get_mcu_chip_id(void)
{


	uint16_t cid;
	cid = (uint16_t)(DBGMCU->IDCODE) & 0x0FFF;
	return  cid;

}

/*************************Bootloader Read Sector Protection Status *************************/

uint16_t bootloader_handle_read_sector_protection_status(void)
{

		FLASH_OBProgramInitTypeDef OBInit;						//This structure is given by ST Flash driver to hold the OB(Option Byte) contents .
		HAL_FLASH_OB_Unlock();								//First unlock the OB(Option Byte) memory access

		HAL_FLASHEx_OBGetConfig(&OBInit);						//get the OB configuration details

		HAL_FLASH_Lock();								//Lock back .


		return (uint16_t)OBInit.WRPSector;						// r/w protection status of the sectors.
}

/************************* Uploading BIN File to The Flash Spesific Address *************************/

void upload_New_App(void)
{

		uint32_t addr;
		uint32_t pageError;
		uint32_t *ptr=(uint32_t*)Temp_RX_Data;

		if(myBootloader.UploadMode==app1)
		{
			bootloader_erase_flash_sector(2,1); 					//flash erase before upload the new app //HARDFAULTA DÜŞÜYOR 2.HAL TRANSMİTTE
			addr=FLASH_APP_1_START;
		}
		else if(myBootloader.UploadMode==app2)
		{
			bootloader_erase_flash_sector(3,1); 					//flash erase before upload the new app
			addr=FLASH_APP_2_START;
		}


		HAL_FLASH_Unlock();
		for(int i=0;i<PAGE_SIZE/4;i++)
			{
				HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr, *ptr++);
			    addr += 4;
			}
		HAL_FLASH_Lock();
}

/************************* UART CALLBACK FUNCTION *************************/
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	 if (huart == &huart3)
	 {
		 bytecount++;

		 HAL_UART_Receive_IT(&huart3, (uint8_t*)RX_Uploading_Data, 1);


			 if (bytecount == PAGE_SIZE )
			 {
				 size_t data_size = sizeof(RX_Uploading_Data);
				 memcpy(Temp_RX_Data, RX_Uploading_Data, data_size);
				 bytecount=0;
			 }
	 }

}



