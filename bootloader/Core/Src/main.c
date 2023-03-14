#include "main.h"
#include "bootloader_functions.h"

/*
 * ----------------------------------
 * |								|
 * |	BOOTLOADER(0X08000000)		| 	SECTOR 0 , 16KB
 * |                                |
 * ----------------------------------
 * |	EMPTY=	0X08004000			|	SECTOR 1 , 16KB
 * ----------------------------------
 * |								|
 * |	   APP-1(0X08008000)		|	SECTOR 2 , 16KB
 * |                                |
 * ----------------------------------
 * ----------------------------------
 * |								|
 * |		APP-2(0X0800C000)		|	SECTOR 3 , 16KB
 * |                                |
 * ----------------------------------*/


/**************** Define **************************/
#define BL_DEBUG_MSG_EN
#define BL_RX_LEN	200


/**************** Variables **************************/

uint8_t bootloader_rx_buffer[BL_RX_LEN];
volatile uint32_t received_data=0;


CRC_HandleTypeDef hcrc;
UART_HandleTypeDef huart3;
bootloader myBootloader;

/**********************FUNCTIONS PROTOTYPE***************************/

void SystemClock_Config(void);
void SelectedMenu(uint8_t selectedMenu);
void host_menu(void);
void Error_Handler(void);


static void MX_USART3_UART_Init(void);
static void MX_GPIO_Init(void);
static void MX_CRC_Init(void);


uint8_t menuOpenSelectionMonitor(void);
bool isUserWantToSeeMenu(uint8_t rxData);


/********************* MENU OPEN SELECTION MONITOR ***********************/

uint8_t menuOpenSelectionMonitor(void)
	    {
	    	static uint8_t press_info[] = "-PRESS M or m TO SHOW MENU\n";
	    	static uint8_t menu='\0';
	    	memset(bootloader_rx_buffer,0,200);
	    	HAL_UART_Transmit(&huart3, press_info, sizeof(press_info),100);
	    	HAL_UART_Receive(&huart3, &menu, sizeof(menu),HAL_MAX_DELAY);
	    	while(!menu);//wait for the users response
	    	return menu;
	    }

/********************* CONTROL IF THE USER WANTS TO SEE MENU ******************/

bool isUserWantToSeeMenu(uint8_t rxData)
	    {
	    	if(rxData == 'M' || rxData == 'm')
	    	{
	    		myBootloader.mainMenuActive = yes;
	    		myBootloader.mainMenuState = Idle;
	    		return true;
	    	}
	    	myBootloader.mainMenuActive = no;
	    	return false;
	    }

/********************* STATE MACHINE **********************************/

void SelectedMenu(uint8_t selectedMenu)
	    {
	    	uint8_t uploadModeRx;
	    	if( selectedMenu != 'E' && myBootloader.mainMenuActive == yes && myBootloader.mainMenuState == Idle)
	    	{
	    		switch(selectedMenu)
	    		{
	    			case '0':
	    				printmsg("BL DEBUG MESG: 0 is pressed and going to default boot mode\r\n");
	    				myBootloader.mainMenuState=Idle;
	    				bootloader_default_mode();
	    				break;
	    			case '1':
	    				printmsg("BL DEBUG MESG: 1 is pressed and going to user APP-1\r\n");
	    				myBootloader.mainMenuState = jumpApp1;
	    				bootloader_jump_to_app1();
	    				break;
	    			case '2':
	    				printmsg("BL DEBUG MESG: 2 is pressed and going to user APP-2\r\n");
	    				myBootloader.mainMenuState = jumpApp2;
	    				bootloader_jump_to_app2();
	    				break;
	    			case '3':
	    				printmsg("BL DEBUG MESG: 3 is pressed and going to delete APP-1\r\n");
	    				myBootloader.mainMenuState = deleteApp1;
	    				//app 1 flash erase function
	    				bootloader_erase_flash_sector(2,1);
	    				printmsg("BL DEBUG MESG: APP-1 is deleted\r\n");
	    				break;
	    			case '4':
	    				printmsg("BL DEBUG MESG: 4 is pressed and going to delete APP-2\r\n");
	    				myBootloader.mainMenuState = deleteApp2;
	    				//app 2 flash erase function
	    				bootloader_erase_flash_sector(3,1);
	    				printmsg("BL DEBUG MESG: APP-2 is deleted\r\n");
	    				break;
	    			case '5':
	    				printmsg("BL DEBUG MESG: 5 is pressed and now you can add bin file\r\n");
	    				printmsg("BL DEBUG MESG: Please choose where you want to upload this file\r\n");
	    				printmsg("BL DEBUG MESG: Click 6 for APP-1\r\n");
	    				printmsg("BL DEBUG MESG: Click 7 for APP-2\r\n");
	    				myBootloader.mainMenuState = uploadModeEnable;
	    				myBootloader.UploadMode = enable;
	    				HAL_UART_Receive(&huart3, &uploadModeRx, sizeof(uploadModeRx), HAL_MAX_DELAY);
	    				while(!uploadModeRx);//wait for the user response

	    				if(uploadModeRx == '6' && myBootloader.UploadMode == enable)
	    				{
	    					myBootloader.UploadMode = app1;
	    					printmsg("BL DEBUG MESG: Now you can add your bin file for APP-1\r\n");

	    					upload_New_App();
	    					//seçilen app in yerine yüklenen uygulamayı flasha yazan fonksiyon
	    						//1.uart receive interrupt
	    						//2.receive edilen her byte volatile uint32_t ReceiveDataArr[14336]; ile rame kaydolur
	    						//3.ramdeki veriler bir volatile uint32_t TempDataArr[14336]; değikenine kopyalanır
	    						//4.volatile uint32_t TempDataArr[14336]; içerisindeki veriler flasha yazılır
	    						//5.chip kendine reset atar ve default olarak app1 i çalıştırır.
	    					printmsg("BL DEBUG MESG: APP-1 is uploaded\r\n");
	    				}
	    				else if(uploadModeRx == '7' && myBootloader.UploadMode == enable)
	    				{
	    					myBootloader.UploadMode = app2;
	    					printmsg("BL DEBUG MESG: Now you can add your bin file for APP-2\r\n");
	    					//seçilen app in yerine yüklenen uygulamayı flasha yazan fonksiyon
	    					upload_New_App();
	    						//1.uart receive interrupt
	    						//2.receive edilen her byte volatile uint32_t ReceiveDataArr[14336]; ile rame kaydolur
	    						//3.ramdeki veriler bir volatile uint32_t TempDataArr[14336]; değikenine kopyalanır
	    						//4.volatile uint32_t TempDataArr[14336]; içerisindeki veriler flasha yazılır
	    						//5.chip kendine reset atar ve default olarak app1 i çalıştırır.
	    					printmsg("BL DEBUG MESG: APP-2 is uploaded\r\n");
	    				}
	    				else
	    				{
	    					printmsg("BL DEBUG MESG: APP upload process is stopped!! \r\n");
	    					break;
	    				}
	    				break;
	    			case '6':
	    				if(myBootloader.UploadMode!=enable )
	    					break;
	    				else
	    				{
							printmsg("BL DEBUG MESG:6 Is Pressed and Here is Your MCU Chip ID\n\r" );
							myBootloader.mainMenuState = readIDNumberMCU;
							char buf[15];
							uint16_t id=bootloader_get_mcu_chip_id();
	    				    HAL_UART_Transmit(&huart3, (uint8_t*)buf,sprintf(buf,"MCU CHIP ID: %hu\r\n",id), HAL_MAX_DELAY);

	    				}
	    				break;

	    			case '7':
	    				if(myBootloader.UploadMode!=enable)
	    					break;
	    				else
	    				{
							printmsg("BL DEBUG MESG:7 Is Pressed and Here is Read Sector Protection Status\n\r" );
							myBootloader.mainMenuState = readSectorProtectionStatus;
							char buf[50];
							uint16_t status=bootloader_handle_read_sector_protection_status();
							HAL_UART_Transmit(&huart3, (uint8_t*)buf,sprintf(buf,"Sector Protection Status: %hu\r\n",status), HAL_MAX_DELAY);
	    				}
	    				break;
	    			case '8':
	    				//eklenecek
	    			default:
	    				printmsg("BL DEBUG MESG: PLEASE RESET THE CHIP AND SELECT THE MENU WHAT YOU WANT --> SELECTION ERROR DETECTED!!!!!\r\n");
	    				break;
	    		}
	    	}
	    	else
	    		printmsg("MENU SELECTION ERROR PLEASE RESET !!");

 }

/***************************************** HOST MENU ***************************************************/
void host_menu(void)
	    {
	    	static uint8_t menu_title[] 		= "******* PRESS 0 BOOTLOADER MENU *******\r\n";
	    	static uint8_t menu_press1[] 		= "******* PRESS 1 FOR JUMP TO APP1 *******\r\n";
	    	static uint8_t menu_press2[] 		= "******* PRESS 2 FOR JUMP TO APP2 *******\r\n";
	    	static uint8_t menu_press3[] 		= "******* PRESS 3 FOR DELETE APP1 *******\r\n";
	    	static uint8_t menu_press4[] 		= "******* PRESS 4 FOR DELETE APP2 *******\r\n";
	    	static uint8_t menu_press5[] 		= "******* PRESS 5 FOR UPLOAD APP *******\r\n";
	    	static uint8_t menu_press6[] 		= "******* PRESS 6 FOR READ IDENTIFICATION NUMBER YOUR MCU *******\r\n";
	    	static uint8_t menu_press7[] 		= "******* PRESS 7 FOR READ SECTOR PROTECTION STATUS *******\r\n";
	    	static uint8_t menu_press8[] 		= "******* PRESS 8 FOR GO TO ADDRESS  *******\r\n";
	    	static uint8_t menu_terminator[] 	= "******* ------------------ *******\r\n";
	    	static uint8_t menu_info[] 			= "******* PLEASE SELECT ONE OF THESE OPTION OR PRESS E TO EXIT *******\r\n";
	    	uint8_t menu_rx=0;

	    	if(myBootloader.mainMenuActive == yes && myBootloader.mainMenuState == Idle){
	    		HAL_UART_Transmit(&huart3,menu_title,sizeof(menu_title), 50);
	    		HAL_UART_Transmit(&huart3,menu_press1,sizeof(menu_press1), 50);
	    		HAL_UART_Transmit(&huart3,menu_press2,sizeof(menu_press2), 50);
	    		HAL_UART_Transmit(&huart3,menu_press3,sizeof(menu_press3), 50);
	    		HAL_UART_Transmit(&huart3,menu_press4,sizeof(menu_press4), 50);
	    		HAL_UART_Transmit(&huart3,menu_press5,sizeof(menu_press5), 50);
	    		HAL_UART_Transmit(&huart3,menu_press6,sizeof(menu_press6), 50);
	    		HAL_UART_Transmit(&huart3,menu_press7,sizeof(menu_press7), 50);
	    		HAL_UART_Transmit(&huart3,menu_press8,sizeof(menu_press8), 50);
	    		HAL_UART_Transmit(&huart3,menu_terminator,sizeof(menu_terminator), 50);
	    		HAL_UART_Transmit(&huart3,menu_info,sizeof(menu_info), 50);
	    		received_data=0;
	    	}
	    	HAL_UART_Receive(&huart3, &menu_rx, sizeof(menu_rx), HAL_MAX_DELAY);
	    	while(!menu_rx);//wait for the until menu_rx=1
	    	SelectedMenu(menu_rx);
 }




int main(void)
{
	  HAL_Init();
	  SystemClock_Config();
	  MX_GPIO_Init();
	  MX_USART3_UART_Init();
	  MX_CRC_Init();


	  /******************Read The User Button For Pass To Bootloader******************/

	  if(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0))
	    {
	  	  printmsg("BL_DEBUG_MSG: BUTTON IS PRESSED.. GOING TO BOOTLOADER MODE\n\r");
	  	  if(isUserWantToSeeMenu(menuOpenSelectionMonitor()) && myBootloader.mainMenuState == Idle)
	  	  {
	  		  host_menu();
	  	  }
	    }

	    else
	    {
	  	  printmsg("BL_DEBUG_MSG: BUTTON IS NOT PRESSED.. EXECUTE DEFAULT APP\n\r");
	  	  while(1)
	  	  {
	  		HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_12);
	  		HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_13);
	  		HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_14);
	  		HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_15);
	  		HAL_Delay(150);
	  	  }
	    }


	while(1)
	{

	}
}


/**************************SYSTEM CLOCK INIT*************************/

void SystemClock_Config(void)
{
	  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
	  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

	  __HAL_RCC_PWR_CLK_ENABLE();
	  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

	  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
	  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
	  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
	  RCC_OscInitStruct.PLL.PLLM = 8;
	  RCC_OscInitStruct.PLL.PLLN = 84;
	  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
	  RCC_OscInitStruct.PLL.PLLQ = 4;
	  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
	  {
	    Error_Handler();
	  }

	  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
	                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
	  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
	  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

	  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
	  {
	    Error_Handler();
	  }



	 HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);					 //Configure the Systick interrupt time

	 HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);				//Configure the Systick

	 HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);							// SysTick_IRQn interrupt configuration
}

/**************************USART COMMUNICATION INIT*************************/

static void MX_USART3_UART_Init(void)
{
	  huart3.Instance = USART3;
	  huart3.Init.BaudRate = 115200;
	  huart3.Init.WordLength = UART_WORDLENGTH_8B;
	  huart3.Init.StopBits = UART_STOPBITS_1;
	  huart3.Init.Parity = UART_PARITY_NONE;
	  huart3.Init.Mode = UART_MODE_TX_RX;
	  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
	  if (HAL_UART_Init(&huart3) != HAL_OK)
	  {
	    Error_Handler();
	  }
}

/**************************GPIO INIT*************************/

static void MX_GPIO_Init(void)
{
	  GPIO_InitTypeDef GPIO_InitStruct = {0};
	  __HAL_RCC_GPIOA_CLK_ENABLE();
	  __HAL_RCC_GPIOD_CLK_ENABLE();
	  __HAL_RCC_GPIOC_CLK_ENABLE();


	  GPIO_InitStruct.Pin = GPIO_PIN_0;
	  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
	  GPIO_InitStruct.Pull = GPIO_NOPULL;
	  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	  /*Configure GPIO pin : PD12 */
	  GPIO_InitStruct.Pin = GPIO_PIN_12;
	  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	  GPIO_InitStruct.Pull = GPIO_NOPULL;
	  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
	  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_RESET);

	  /*Configure GPIO pin : PD13 */
	  GPIO_InitStruct.Pin = GPIO_PIN_13;
	  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	  GPIO_InitStruct.Pull = GPIO_NOPULL;
	  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
	  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, GPIO_PIN_RESET);

	  /*Configure GPIO pin : PD13 */
	  GPIO_InitStruct.Pin = GPIO_PIN_14;
	  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	  GPIO_InitStruct.Pull = GPIO_NOPULL;
	  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
	  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, GPIO_PIN_RESET);

	  /*Configure GPIO pin : PD13 */
	  GPIO_InitStruct.Pin = GPIO_PIN_15;
	  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	  GPIO_InitStruct.Pull = GPIO_NOPULL;
	  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
	  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, GPIO_PIN_RESET);

}




/**************************CRC INIT*************************/

static void MX_CRC_Init(void)
{
	  hcrc.Instance = CRC;
	  if (HAL_CRC_Init(&hcrc) != HAL_OK)
	  {
	    Error_Handler();
	  }
}


/**************************ERROR HANDLER*************************/
void Error_Handler(void)
{
	  __disable_irq();
	  while (1)
	  {
	  }
}
