#include "main.h"
#include "adc.h"
#include "usb_device.h"
#include "gpio.h"
#include "usbd_hid.h"

extern USBD_HandleTypeDef hUsbDeviceFS;

void SystemClock_Config(void);

int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_ADC1_Init();
  MX_USB_DEVICE_Init();

  uint8_t joystick_report[14];

  ADC_ChannelConfTypeDef sConfig = {0};
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_41CYCLES_5;

  uint8_t adc_channels[6] =
  {
      ADC_CHANNEL_0,
      ADC_CHANNEL_1,
      ADC_CHANNEL_2,
      ADC_CHANNEL_3,
      ADC_CHANNEL_4,
      ADC_CHANNEL_5
  };

  while (1)
  {
      for (int i = 0; i < 6; i++)
      {
          sConfig.Channel = adc_channels[i];
          HAL_ADC_ConfigChannel(&hadc1, &sConfig);
          HAL_ADC_Start(&hadc1);
          HAL_ADC_PollForConversion(&hadc1, 10);
          uint16_t raw = HAL_ADC_GetValue(&hadc1);
          uint16_t val = raw;

          switch (i)
          {
              case 0:
              {
                  uint16_t min = 0;
                  uint16_t max = 3095;
                  if (raw <= min) val = 0;
                  else if (raw >= max) val = 4095;
                  else val = (uint32_t)(raw - min) * 4095 / (max - min);
                  val = 4095 - val;
                  break;
              }
              case 1:
              {
                  uint16_t min = 0;
                  uint16_t max = 2595;
                  if (raw <= min) val = 0;
                  else if (raw >= max) val = 4095;
                  else val = (uint32_t)(raw - min) * 4095 / (max - min);
                  val = 4095 - val;
                  break;
              }
              case 2:
              {
                  uint16_t min = 0;
                  uint16_t max = 4095;
                  if (raw <= min) val = 0;
                  else if (raw >= max) val = 4095;
                  else val = (uint32_t)(raw - min) * 4095 / (max - min);
                  val = 4095 - val;
                  break;
              }
              case 3:
              {
                  uint16_t min = 0;
                  uint16_t max = 4095;
                  if (raw <= min) val = 0;
                  else if (raw >= max) val = 4095;
                  else val = (uint32_t)(raw - min) * 4095 / (max - min);
                  val = 4095 - val;
                  break;
              }
              case 4:
              {
                  uint16_t min = 0;
                  uint16_t max = 4095;
                  if (raw <= min) val = 0;
                  else if (raw >= max) val = 4095;
                  else val = (uint32_t)(raw - min) * 4095 / (max - min);
                  val = 4095 - val;
                  break;
              }
              case 5:
              {
                  uint16_t min = 0;
                  uint16_t max = 4095;
                  if (raw <= min) val = 0;
                  else if (raw >= max) val = 4095;
                  else val = (uint32_t)(raw - min) * 4095 / (max - min);
                  break;
              }
          }

          HAL_ADC_Stop(&hadc1);
          joystick_report[i * 2] = val & 0xFF;
          joystick_report[i * 2 + 1] = (val >> 8) & 0xFF;
      }

      uint16_t buttons = 0;
      buttons |= (!HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0))  << 0;
      buttons |= (!HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_1))  << 1;
      buttons |= (!HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_3))  << 2;
      buttons |= (!HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_4))  << 3;
      buttons |= (!HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_5))  << 4;
      buttons |= (!HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_8))  << 5;
      buttons |= (!HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_9))  << 6;
      buttons |= (!HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_10)) << 7;
      buttons |= (!HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_11)) << 8;
      buttons |= (!HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_12)) << 9;
      buttons |= (!HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_13)) << 10;
      buttons |= (!HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_14)) << 11;

      joystick_report[12] = buttons & 0xFF;
      joystick_report[13] = (buttons >> 8) & 0xFF;

      USBD_HID_SendReport(&hUsbDeviceFS, joystick_report, 14);
      HAL_Delay(10);
  }
}

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) Error_Handler();
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) Error_Handler();
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC|RCC_PERIPHCLK_USB;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_PLL_DIV1_5;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) Error_Handler();
}

void Error_Handler(void)
{
  __disable_irq();
  while (1) {}
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line) {}
#endif
