/*
    我想你在移植之前 应该知道SPL HAL库非常不同 真的
    一定要注意 在这两个库中间 USART_IT_* 宏 虽然名字一样 但是指的东西绝对不一样
    这些都是痛苦的教训 :/
    错误的写法: CDC_UART->CR1 &= ~(USART_IT_TXE | USART_IT_RXNE);
    正确的: USART_ITConfig(CDC_UART, USART_IT_TXE | USART_IT_RXNE, DISABLE);

    在SPL下 还是尽量用库函数吧 不会错的
*/
#include "string.h"

#include "air32f10x.h"
#include "air32f10x_gpio.h"
#include "air32f10x_usart.h"
#include "uart.h"
#include "gpio.h"
#include "util.h"
#include "circ_buf.h"
#include "IO_Config.h"


// For usart
#define CDC_UART                     USART2
#define CDC_UART_ENABLE()            RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2,ENABLE);
#define CDC_UART_DISABLE()           RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2,DISABLE);
#define CDC_UART_IRQn                USART2_IRQn
#define CDC_UART_IRQn_Handler        USART2_IRQHandler

#define UART_PINS_PORT_ENABLE()      RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);
#define UART_PINS_PORT_DISABLE()     RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,DISABLE);

#define UART_TX_PORT                 GPIOA
#define UART_TX_PIN                  GPIO_Pin_2

#define UART_RX_PORT                 GPIOA
#define UART_RX_PIN                  GPIO_Pin_3

#define UART_CTS_PORT                GPIOA
#define UART_CTS_PIN                 GPIO_Pin_0

#define UART_RTS_PORT                GPIOA
#define UART_RTS_PIN                 GPIO_Pin_1


#define RX_OVRF_MSG         "<DAPLink:Overflow>\n"
#define RX_OVRF_MSG_SIZE    (sizeof(RX_OVRF_MSG) - 1)
#define BUFFER_SIZE         (512)

circ_buf_t write_buffer;
uint8_t write_buffer_data[BUFFER_SIZE];
circ_buf_t read_buffer;
uint8_t read_buffer_data[BUFFER_SIZE];

static UART_Configuration configuration = {
    .Baudrate = 9600,
    .DataBits = UART_DATA_BITS_8,
    .Parity = UART_PARITY_NONE,
    .StopBits = UART_STOP_BITS_1,
    .FlowControl = UART_FLOW_CONTROL_NONE,
};

extern uint32_t SystemCoreClock;



static void clear_buffers(void)
{
    circ_buf_init(&write_buffer, write_buffer_data, sizeof(write_buffer_data));
    circ_buf_init(&read_buffer, read_buffer_data, sizeof(read_buffer_data));
}

int32_t uart_initialize(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    USART_ITConfig(CDC_UART, USART_IT_TXE | USART_IT_RXNE, DISABLE);
    clear_buffers();

    CDC_UART_ENABLE();
    UART_PINS_PORT_ENABLE();

    //TX pin
    GPIO_InitStructure.GPIO_Pin = UART_TX_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(UART_TX_PORT, &GPIO_InitStructure);
    //RX pin
    GPIO_InitStructure.GPIO_Pin = UART_RX_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(UART_RX_PORT, &GPIO_InitStructure);

    NVIC_EnableIRQ(CDC_UART_IRQn);

    return 1;
}

int32_t uart_uninitialize(void)
{
    //CDC_UART->CR1 &= ~(USART_IT_TXE | USART_IT_RXNE);
    USART_ITConfig(CDC_UART, USART_IT_TXE | USART_IT_RXNE, DISABLE);
    clear_buffers();
    return 1;
}

int32_t uart_reset(void)
{
    //const uint32_t cr1 = CDC_UART->CR1;
    //CDC_UART->CR1 = cr1 & ~(USART_IT_TXE | USART_IT_RXNE);
    USART_ITConfig(CDC_UART, USART_IT_TXE | USART_IT_RXNE, DISABLE);
    clear_buffers();
    //CDC_UART->CR1 = cr1 & ~USART_IT_TXE;
    USART_ITConfig(CDC_UART, USART_IT_TXE, DISABLE);
    return 1;
}

int32_t uart_set_configuration(UART_Configuration *config)
{
    USART_InitTypeDef uart_conf;
    memset(&uart_conf, 0, sizeof(uart_conf));
	
    // Parity confuguration
    configuration.Parity = config->Parity;
    if(config->Parity == UART_PARITY_ODD) {
        uart_conf.USART_Parity = USART_Parity_Odd;
    } else if(config->Parity == UART_PARITY_EVEN) {
        uart_conf.USART_Parity = USART_Parity_Even;
    } else if(config->Parity == UART_PARITY_NONE) {
        uart_conf.USART_Parity = USART_Parity_No;
    } else {   //Other not support
        uart_conf.USART_Parity = USART_Parity_No;
        configuration.Parity = UART_PARITY_NONE;
    }

    // Stop bit confuguration
    configuration.StopBits = config->StopBits;
    if(config->StopBits == UART_STOP_BITS_2) {
        uart_conf.USART_StopBits = USART_StopBits_2;
    } else if(config->StopBits == UART_STOP_BITS_1_5) {
        uart_conf.USART_StopBits = USART_StopBits_2;
        configuration.StopBits = UART_STOP_BITS_2;
    } else if(config->StopBits == UART_STOP_BITS_1) {
        uart_conf.USART_StopBits = USART_StopBits_1;
    } else {
        uart_conf.USART_StopBits = USART_StopBits_1;
        configuration.StopBits = UART_STOP_BITS_1;
    }

    //Only 8 bit support
    configuration.DataBits = UART_DATA_BITS_8;
    if(uart_conf.USART_Parity == USART_Parity_Odd ||
       uart_conf.USART_Parity == USART_Parity_Even)
    {
        uart_conf.USART_WordLength = USART_WordLength_9b;
    }
    else 
    {
        uart_conf.USART_WordLength = USART_WordLength_8b;
    }

    // No flow control
    configuration.FlowControl = UART_FLOW_CONTROL_NONE;
    uart_conf.USART_HardwareFlowControl = USART_HardwareFlowControl_None;

    // Specified baudrate
    configuration.Baudrate = config->Baudrate;
    uart_conf.USART_BaudRate = config->Baudrate;

    // TX and RX mode ENABLE
    uart_conf.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

    // Disable uart and tx/rx interrupt
    USART_ITConfig(CDC_UART, USART_IT_TXE | USART_IT_RXNE, DISABLE);

    clear_buffers();
    
	USART_DeInit(CDC_UART);
	
    USART_Init(CDC_UART, &uart_conf);
    USART_Cmd(CDC_UART, ENABLE);

    // Enable USART interrupt
    USART_ITConfig(CDC_UART, USART_IT_RXNE, ENABLE);

    return 1;
}

int32_t uart_get_configuration(UART_Configuration *config)
{
    config->Baudrate = configuration.Baudrate;
    config->DataBits = configuration.DataBits;
    config->Parity   = configuration.Parity;
    config->StopBits = configuration.StopBits;
    config->FlowControl = UART_FLOW_CONTROL_NONE;

    return 1;
}

void uart_set_control_line_state(uint16_t ctrl_bmp)
{
}

int32_t uart_write_free(void)
{
    return circ_buf_count_free(&write_buffer);
}

int32_t uart_write_data(uint8_t *data, uint16_t size)
{
	uint32_t cnt = circ_buf_write(&write_buffer, data, size);
    USART_ITConfig(CDC_UART, USART_IT_TXE, ENABLE);

    return cnt;
}

int32_t uart_read_data(uint8_t *data, uint16_t size)
{
    return circ_buf_read(&read_buffer, data, size);
}

void CDC_UART_IRQn_Handler(void)
{
    if (USART_GetITStatus(CDC_UART, USART_IT_RXNE)) {
        uint8_t dat = CDC_UART->DR;
        uint32_t free = circ_buf_count_free(&read_buffer);
        if (free > RX_OVRF_MSG_SIZE) {
            circ_buf_push(&read_buffer, dat);
        } else if (RX_OVRF_MSG_SIZE == free) {
            circ_buf_write(&read_buffer, (uint8_t*)RX_OVRF_MSG, RX_OVRF_MSG_SIZE);
        } else {
            // Drop character
        }
    }

    if (USART_GetITStatus(CDC_UART, USART_IT_TXE)) {
        if (circ_buf_count_used(&write_buffer) > 0) {
            CDC_UART->DR = circ_buf_pop(&write_buffer);
        } else {
            USART_ITConfig(CDC_UART, USART_IT_TXE, DISABLE);
        }
    }
}
