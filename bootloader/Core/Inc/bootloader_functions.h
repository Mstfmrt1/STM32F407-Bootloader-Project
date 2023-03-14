

#ifndef INC_BOOTLOADER_FUNCTIONS_H_
#define INC_BOOTLOADER_FUNCTIONS_H_



/**************************INCLUDES**********************************/

#include "main.h"


/*************************Enums, Struct *******************************/
typedef enum tag_isMainMenuActivated{
	no=0,
	yes
}mainMenuActive;


typedef enum tag_mainMenuUserInputState{
	Idle,
	jumpApp1,
	jumpApp2,
	deleteApp1,
	deleteApp2,
	uploadModeEnable,
	readIDNumberMCU,
	readSectorProtectionStatus,
	gotoAddress
}mainMenuState;

typedef enum tag_subMenuUploadApp{
	enable,
	app1,
	app2
}UploadMode;


typedef struct tag_bootloader{
	uint8_t mainMenuActive;
	uint8_t mainMenuState;
	uint8_t UploadMode;

}bootloader;


void printmsg(char *format,...);
void bootloader_default_mode(void);
void bootloader_jump_to_app1(void);
void bootloader_jump_to_app2(void);
uint8_t execute_flash_erase(uint8_t sector_number,uint8_t number_of_sector);
void bootloader_erase_flash_sector(uint8_t sector_number1,uint8_t number_of_sector1);
uint16_t bootloader_get_mcu_chip_id(void);
uint16_t bootloader_handle_read_sector_protection_status(void);
void upload_New_App(void);
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart);

#endif /* INC_BOOTLOADER_FUNCTIONS_H_ */
