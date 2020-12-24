/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "i2c.h"
#include "spi.h"
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ssd1306.h"
#include "fonts.h"
#include "stdio.h"
#include "SX1278.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
#define DS18B20_PORT GPIOA
#define DS18B20_PIN  GPIO_PIN_11
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */


uint32_t cap1=0;
uint32_t cap2=0;
uint32_t diff=0;
uint8_t is_fist_capture=0;
uint32_t distance =0;
uint32_t tam;
uint8_t total = 0;
uint8_t medium=0;
int test = 45;
char t[4];
uint32_t sum_distance = 0;
SX1278_hw_t SX1278_hw;
SX1278_t SX1278;
int master;
int ret;
char buffer[64];
int message_length;
int ret_sent;
int ret;
char buffsent[64];
int get_adc;
uint8_t flag =0;
uint8_t Presence =0;
uint8_t Temp_byte1 =0;
uint8_t Temp_byte2 = 0;
uint16_t TEMP =0 ;
uint8_t Temperature =0;
float ADC =0;
uint8_t Percent_humidity =0;
uint16_t time_reset =0;
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
void delay_us (uint16_t us){
	__HAL_TIM_SET_COUNTER(&htim1,0);
	while(__HAL_TIM_GET_COUNTER(&htim1)<us);
}// end void delay_us ()

void Set_Pin_Output (GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Pin = GPIO_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOx, &GPIO_InitStruct);
}

void Set_Pin_Input (GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Pin = GPIO_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(GPIOx, &GPIO_InitStruct);
}

uint8_t DS18B20_Start (void)
{
	uint8_t Response = 0;
	Set_Pin_Output(DS18B20_PORT, DS18B20_PIN);   // set the pin as output
	HAL_GPIO_WritePin (DS18B20_PORT, DS18B20_PIN, 0);  // pull the pin low
	delay_us (480);   // delay_us according to datasheet

	Set_Pin_Input(DS18B20_PORT, DS18B20_PIN);    // set the pin as input
	delay_us (80);    // delay_us according to datasheet

	if (!(HAL_GPIO_ReadPin (DS18B20_PORT, DS18B20_PIN))) Response = 1;    // if the pin is low i.e the presence pulse is detected
	else Response = -1;

	delay_us (400); // 480 us delay_us totally.

	return Response;
}

void DS18B20_Write (uint8_t data)
{
	Set_Pin_Output(DS18B20_PORT, DS18B20_PIN);  // set as output

	for (int i=0; i<8; i++)
	{

		if ((data & (1<<i))!=0)  // if the bit is high
		{
			// write 1

			Set_Pin_Output(DS18B20_PORT, DS18B20_PIN);  // set as output
			HAL_GPIO_WritePin (DS18B20_PORT, DS18B20_PIN, 0);  // pull the pin LOW
			delay_us (1);  // wait for 1 us

			Set_Pin_Input(DS18B20_PORT, DS18B20_PIN);  // set as input
			delay_us (50);  // wait for 60 us
		}

		else  // if the bit is low
		{
			// write 0

			Set_Pin_Output(DS18B20_PORT, DS18B20_PIN);
			HAL_GPIO_WritePin (DS18B20_PORT, DS18B20_PIN, 0);  // pull the pin LOW
			delay_us (50);  // wait for 60 us

			Set_Pin_Input(DS18B20_PORT, DS18B20_PIN);
		}
	}
}

uint8_t DS18B20_Read (void)
{
	uint8_t value=0;

	Set_Pin_Input(DS18B20_PORT, DS18B20_PIN);

	for (int i=0;i<8;i++)
	{
		Set_Pin_Output(DS18B20_PORT, DS18B20_PIN);   // set as output

		HAL_GPIO_WritePin (DS18B20_PORT, DS18B20_PIN, 0);  // pull the data pin LOW
		delay_us (2);  // wait for > 1us

		Set_Pin_Input(DS18B20_PORT, DS18B20_PIN);  // set as input
		if (HAL_GPIO_ReadPin (DS18B20_PORT, DS18B20_PIN))  // if the pin is HIGH
		{
			value |= 1<<i;  // read = 1
		}
		delay_us (50);  // wait for 60 us
	}
	return value;
}

uint8_t Read_temp(){
		DS18B20_Start ();
	  HAL_Delay (1);
	  DS18B20_Write (0xCC);  // skip ROM
	  DS18B20_Write (0x44);  // convert t
	  HAL_Delay (800);

		Presence = DS18B20_Start ();
		HAL_Delay(1);
		DS18B20_Write (0xCC);  // skip ROM
		DS18B20_Write (0xBE);  // Read Scratch-pad

		Temp_byte1 = DS18B20_Read();
	  Temp_byte2 = DS18B20_Read();
	  TEMP = (Temp_byte2<<8)|Temp_byte1;
	  Temperature = (float)TEMP/16;
}
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
int _write(int file, char *ptr, int len) {
	int i;
	for (i = 0; i < len; i++) {
		ITM_SendChar(*ptr++);
	}
	return len;
}// end _write ()



//void delay_us (uint16_t us);
void read_SR04();
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM1_Init();
  MX_I2C1_Init();
  MX_SPI1_Init();
  MX_ADC1_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */
//  HAL_TIM_Base_Start(&htim1);
  HAL_TIM_IC_Start_IT(&htim1, TIM_CHANNEL_1);
	HAL_TIM_Base_Start(&htim2);
	HAL_ADC_Start(&hadc1);
  SSD1306_Init ();

  SX1278_hw.dio0.port = DIO0_GPIO_Port;
  SX1278_hw.dio0.pin = DIO0_Pin;
  SX1278_hw.nss.port = NSS_GPIO_Port;
  SX1278_hw.nss.pin = NSS_Pin;
  SX1278_hw.reset.port = RESET_GPIO_Port;
  SX1278_hw.reset.pin = RESET_Pin;
  SX1278_hw.spi = &hspi1;
  SX1278.hw = &SX1278_hw;
  SX1278_begin(&SX1278, SX1278_433MHZ, SX1278_POWER_10DBM, SX1278_LORA_SF_10,
    				SX1278_LORA_BW_250KHZ, 10);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  SX1278_LoRaEntryRx(&SX1278,16, 2000);
  while (1)
  {
		

	if(flag == 1){
    	Read_temp();
      for(int i = 0; i<5;i++){
		read_SR04();
//		sum_distance = sum_distance + distance;
		HAL_Delay(70);
      }
//      distance = sum_distance/3 ;
//      sum_distance=0;
	  if (distance < 100){
		  sprintf(t,"%d",distance);
		  t[2]=' ';
		  t[3]=' ';
	  }else if(distance >=100 && distance < 1000){
		  sprintf(t,"%d",distance);
		  t[3]=' ';
	  }else {
		  sprintf(t,"%d",distance);
	  }
		ADC = 0;
		for( int i =0; i<10;i++){
			ADC= ADC + HAL_ADC_GetValue(&hadc1);
		}
		Percent_humidity = 100 - ((float)ADC *100)/(3980*10);	
	  SSD1306_GotoXY (10,20);  // goto 10, 10
	  SSD1306_Puts ("Distance:", &Font_7x10, 1);  
	  SSD1306_Puts (t, &Font_7x10, 1);
		SSD1306_Puts ("           ", &Font_7x10, 1);
		
		char temp_str[3];
		sprintf ( temp_str,"%d",Temperature);
		SSD1306_GotoXY (10,33);  // goto 10, 10
	  SSD1306_Puts ("Temp:", &Font_7x10, 1);  
	  SSD1306_Puts (temp_str, &Font_7x10, 1);
		
		char humidity_str[3];
		if ( Percent_humidity < 100 ){
			sprintf ( humidity_str,"%d",Percent_humidity);
			humidity_str[2] =' ';
			SSD1306_GotoXY (10,46);  // goto 10, 10
			SSD1306_Puts ("Hump:", &Font_7x10, 1);  
			SSD1306_Puts (humidity_str, &Font_7x10, 1);
			SSD1306_Puts ("   ", &Font_7x10, 1);
		}else {
			sprintf ( humidity_str,"%d",Percent_humidity);
			SSD1306_GotoXY (10,46);  // goto 10, 10
			SSD1306_Puts ("Hump:", &Font_7x10, 1);  
			SSD1306_Puts (humidity_str, &Font_7x10, 1);
		}// end if
		
		
		

		SSD1306_GotoXY (100,20);  // goto 10, 10
	  SSD1306_Puts ("   ", &Font_7x10, 1);  
		
	  SSD1306_UpdateScreen();
	  message_length = sprintf(buffsent, "N1:%d:%d:%d", distance,Temperature,Percent_humidity);

	  for(int i =0; i < 3; i++){
		  ret_sent = SX1278_LoRaEntryTx(&SX1278, message_length, 1000);
		  ret_sent = SX1278_LoRaTxPacket(&SX1278, (uint8_t* )buffsent, message_length,1000);
	  }
	  SX1278_LoRaEntryRx(&SX1278,16, 1000);
	  flag = 0;
		__HAL_TIM_SetCounter(&htim2,0);
		time_reset =0;
	}// end if
	
	
	if (__HAL_TIM_GetCounter(&htim2)>=60000){
		time_reset ++;
		if (time_reset > 8000) HAL_NVIC_SystemReset();
	}// end if 
	
	
	
	


    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL8;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV4;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */




void read_SR04(){
		HAL_GPIO_WritePin(TRIG_GPIO_Port, TRIG_Pin, GPIO_PIN_RESET);
		delay_us(2);
		HAL_GPIO_WritePin(TRIG_GPIO_Port, TRIG_Pin, GPIO_PIN_SET);
		delay_us(10);
		HAL_GPIO_WritePin(TRIG_GPIO_Port, TRIG_Pin, GPIO_PIN_RESET);
		__HAL_TIM_ENABLE_IT(&htim1, TIM_IT_CC1);


}// end read_SR04



void HAL_TIM_IC_CaptureCallback (TIM_HandleTypeDef *htim){

	if(htim -> Channel == HAL_TIM_ACTIVE_CHANNEL_1 ){
		if(is_fist_capture == 0){
			cap1 = HAL_TIM_ReadCapturedValue(&htim1, TIM_CHANNEL_1);
			is_fist_capture = 1;
			__HAL_TIM_SET_CAPTUREPOLARITY(&htim1,TIM_CHANNEL_1,TIM_INPUTCHANNELPOLARITY_FALLING);
		}else if(is_fist_capture == 1){
			cap2 = HAL_TIM_ReadCapturedValue(&htim1, TIM_CHANNEL_1);
			__HAL_TIM_SET_COUNTER(&htim1, 0);
			if(cap2 > cap1){
				diff = cap2 - cap1;
			}else if (cap1 > cap2){
				diff = (0xffff - cap1) + cap2;
			}// end else if

			distance = (diff*0.343)/2 + 20;
//		 tam = diff/5.8; //(diff*0.34/2);//
//		 distance = tam + 25;
//		 if(tam < 260){
//			 distance=tam+25;
//		 }else{
//
//			 distance = tam + 130;
//		 }
		 is_fist_capture = 0;
		 __HAL_TIM_SET_CAPTUREPOLARITY(&htim1,TIM_CHANNEL_1,TIM_INPUTCHANNELPOLARITY_RISING);
		 __HAL_TIM_DISABLE_IT(&htim1,TIM_IT_CC1);
		}// end else if
	}// end if

}// end HAL_TIM_IC_CaptureCallback

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */

  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
