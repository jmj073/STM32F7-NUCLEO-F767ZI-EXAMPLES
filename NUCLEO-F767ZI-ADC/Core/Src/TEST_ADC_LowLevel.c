/*
 * TEST_ADC_LowLevel.c
 *
 *  Created on: Sep 28, 2022
 *      Author: JMJ
 */
// https://www.st.com/resource/en/application_note/cd00258017-stm32s-adc-modes-and-their-applications-stmicroelectronics.pdf
// nucleo-f767zi ADC pin assignments: https://www.st.com/resource/en/user_manual/um1974-stm32-nucleo144-boards-mb1137-stmicroelectronics.pdf


#include "main.h"

// HAL_ADC_Start 함수 참고
static inline void ADC_STAB() { // stabilization for accuracy
	volatile uint32_t counter = 0;
    counter = (6 * (SystemCoreClock / 1000000));
    while (counter) counter--;
}

#if 0 /* single-channel, single conversion mode */

#include <stdio.h>
#include "usart.h"

static void ADC1_Init()
{
	__HAL_RCC_ADC1_CLK_ENABLE();

	// ADC(숫자 없음)은 common(공통)이므로 모든 ADC(1, 2, 3, ...)에 적용된다.
	ADC->CCR |= (
		0b00 << ADC_CCR_ADCPRE_Pos 	| // PCLK2 divided by 2
	0);

	ADC1->CR1 |= (
		0b00 << ADC_CR1_RES_Pos		| // resolution: 12-bit
	0);

	/* channel configuration */
	ADC1->SQR3 |= 18 << ADC_SQR3_SQ1_Pos; // Assign channel18(temperature sensor) to SQ1
	ADC1->SMPR1 |= 0b111 << ADC_SMPR1_SMP18_Pos; // sampling time: 480 ADCCLK
}

void TestFunction()
{
	ADC1_Init();

	ADC->CCR |= ADC_CCR_TSVREFE; // temperature sensor channel enable
	ADC1->CR2 |= ADC_CR2_ADON; // Enable ADC
	ADC_STAB();

	ADC1->CR2 |= ADC_CR2_SWSTART; // start conversion

	while ( !(ADC1->SR & ADC_SR_EOC) ); // waiting for End Of Conversion

	uint16_t raw_value = ADC1->DR;
	float temp = ((float)raw_value) / ((1 << 12) - 1) * 3300;
	// temperature sensor electrical characteristics 참고
	temp = (temp - 760.0) / 2.5 + 25;

	char msg[64];

	int sz = sprintf(msg, "ADC Raw: %hu\r\n", raw_value);
	HAL_UART_Transmit(&huart3, (uint8_t*)msg, sz, HAL_MAX_DELAY);

	sz = sprintf(msg, "Temperature: %f\r\n", temp);
	HAL_UART_Transmit(&huart3, (uint8_t*)msg, sz, HAL_MAX_DELAY);

	while (1);
}

#endif /* single-channel, single conversion mode */

#if 0 /* single-channel, continuous conversion mode */

#include <stdio.h>
#include "usart.h"

static void ADC1_Init()
{
	__HAL_RCC_ADC1_CLK_ENABLE();

	// ADC(숫자 없음)는 common(공통)이므로 모든 ADC(1, 2, 3, ...)에 적용된다.
	ADC->CCR |= (
		0b00 << ADC_CCR_ADCPRE_Pos 	| // PCLK2 divided by 2
	0);

	ADC1->CR1 |= (
		0b00 << ADC_CR1_RES_Pos		| // resolution: 12-bit
	0);

	ADC1->CR2 |= (
			ADC_CR2_CONT			| // continuous conversion mode
			ADC_CR2_EOCS			| // enable overrun detection
	0);

	/* channel configuration */
	ADC1->SQR3 |= 18 << ADC_SQR3_SQ1_Pos; // Assign channel18(temperature sensor) to SQ1
	ADC1->SMPR1 |= 0b111 << ADC_SMPR1_SMP18_Pos; // sampling time: 480 ADCCLK
}

void TestFunction()
{
	ADC1_Init();

	ADC->CCR |= ADC_CCR_TSVREFE; // temperature sensor channel enable
	ADC1->CR2 |= ADC_CR2_ADON; // Enable ADC
	ADC_STAB();

	ADC1->CR2 |= ADC_CR2_SWSTART; // start conversion

	uint16_t data[10];

	for (int i = 0; i < 10; i++) {
		while ( !(ADC1->SR & ADC_SR_EOC) ); // waiting for End Of Conversion

		data[i] = ADC1->DR;

		if (ADC1->SR & ADC_SR_OVR) {
			HAL_GPIO_WritePin(LD1_GPIO_Port, LD1_Pin, GPIO_PIN_SET);
			break;
		}
	}

	ADC1->CR2 &= ~ADC_CR2_CONT; // stop continuous conversion

	for (int i = 0; i < 10; i++) {
		float temp = ((float)data[i]) / ((1 << 12) - 1) * 3300;
		// temperature sensor electrical characteristics 참고
		temp = (temp - 760.0) / 2.5 + 25;

		char msg[64];

		int sz = sprintf(msg, "%d ADC Raw: %hu\r\n", i, data[i]);
		HAL_UART_Transmit(&huart3, (uint8_t*)msg, sz, HAL_MAX_DELAY);

		sz = sprintf(msg, "Temperature: %f\r\n", temp);
		HAL_UART_Transmit(&huart3, (uint8_t*)msg, sz, HAL_MAX_DELAY);
	}


	while (1);
}

#endif /* single-channel, continuous conversion mode */

#if 1 /* multi-channel(scan), single conversion mode */

#include <stdio.h>
#include "usart.h"

static void GPIO_Init()
{
	GPIO_InitTypeDef gpio = { 0 };

	__HAL_RCC_GPIOC_CLK_ENABLE();

	/* ADC123_IN12 */
	gpio.Pin = GPIO_PIN_2;
	gpio.Mode = GPIO_MODE_ANALOG;

	HAL_GPIO_Init(GPIOC, &gpio);
}

static void ADC1_Init()
{
	__HAL_RCC_ADC1_CLK_ENABLE();

	// ADC(숫자 없음)는 common(공통)이므로 모든 ADC(1, 2, 3, ...)에 적용된다.
	ADC->CCR |= (
		0b00 << ADC_CCR_ADCPRE_Pos 	| // PCLK2 divided by 2
	0);

	ADC1->CR1 |= (
		0b00 << ADC_CR1_RES_Pos		| // resolution: 12-bit
		ADC_CR1_SCAN				| // enable scan mode
	0);

	ADC1->CR2 |= (
//			ADC_CR2_CONT			| // continuous conversion mode
			ADC_CR2_EOCS			| // enable overrun detection
	0);

	/* channel configuration */
	ADC1->SQR1 |= (2 - 1) << ADC_SQR1_L_Pos; // sequence length
	/* SQ1 */
	ADC1->SQR3 |= 18 << ADC_SQR3_SQ1_Pos; // Assign channel18(temperature sensor) to SQ1
	ADC1->SMPR1 |= 0b111 << ADC_SMPR1_SMP18_Pos; // sampling time: 480 ADCCLK
	/* SQ2 */
	ADC1->SQR3 |= 12 << ADC_SQR3_SQ2_Pos;
	ADC1->SMPR1 |= 0b111 << ADC_SMPR1_SMP12_Pos; // sampling time: 480 ADCCLK
}

void TestFunction()
{
	GPIO_Init();
	ADC1_Init();

	ADC->CCR |= ADC_CCR_TSVREFE; // temperature sensor channel enable
	ADC1->CR2 |= ADC_CR2_ADON; // Enable ADC
	ADC_STAB();

	ADC1->CR2 |= ADC_CR2_SWSTART; // start conversion

	uint16_t data[2];

	for (int i = 0; i < 2; i++) {
		while ( !(ADC1->SR & ADC_SR_EOC) ); // waiting for End Of Conversion

		data[i] = ADC1->DR;

		if (ADC1->SR & ADC_SR_OVR) {
			HAL_GPIO_WritePin(LD1_GPIO_Port, LD1_Pin, GPIO_PIN_SET);
			break;
		}
	}

	ADC1->CR2 &= ~ADC_CR2_CONT; // stop continuous conversion

	for (int i = 0; i < 2; i++) {
		char msg[64];

		int sz = sprintf(msg, "v%d: %hu\r\n", i, data[i]);
		HAL_UART_Transmit(&huart3, (uint8_t*)msg, sz, HAL_MAX_DELAY);
	}


	while (1);
}

#endif /* multi-channel(scan), single conversion mode */




















