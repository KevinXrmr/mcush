#include "mcush.h"
#include "usbd_def.h"
#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_cdc.h"
#include "usbd_cdc_if.h"


USBD_HandleTypeDef hUsbDeviceFS;
extern USBD_DescriptorsTypeDef FS_Desc;
extern USBD_CDC_ItfTypeDef USBD_Interface_fops_FS;

#define QUEUE_UART_RX_LEN    128
#define QUEUE_UART_TX_LEN    128

QueueHandle_t hal_queue_vcp_rx;
QueueHandle_t hal_queue_vcp_tx;


signed portBASE_TYPE hal_vcp_putc( char c )
{
    if( hUsbDeviceFS.dev_config == 0 )
        return pdFAIL;
    if( CDC_Transmit_FS( (uint8_t*)&c, 1 ) == USBD_OK )
        return pdPASS;
    else
        return pdFAIL;
}

signed portBASE_TYPE hal_vcp_write( char *buf, int len )
{
    if( hUsbDeviceFS.dev_config == 0 )
        return pdFAIL;
    if( CDC_Transmit_FS( (uint8_t*)buf, len ) == USBD_OK )
        return pdPASS;
    else
        return pdFAIL;
}


signed portBASE_TYPE hal_vcp_getc( char *c, TickType_t xBlockTime )
{
    return xQueueReceive( hal_queue_vcp_rx, c, xBlockTime );
}


void hal_vcp_reset(void)
{
    xQueueReset( hal_queue_vcp_rx );
    xQueueReset( hal_queue_vcp_tx );
}


void hal_vcp_enable(uint8_t enable)
{
}


int hal_uart_init(uint32_t baudrate)
{
    hal_queue_vcp_rx = xQueueCreate( QUEUE_UART_RX_LEN, ( unsigned portBASE_TYPE ) sizeof( signed char ) );
    hal_queue_vcp_tx = xQueueCreate( QUEUE_UART_TX_LEN, ( unsigned portBASE_TYPE ) sizeof( signed char ) );
    if( !hal_queue_vcp_rx || !hal_queue_vcp_tx )
        return 0;

    USBD_Init(&hUsbDeviceFS, &FS_Desc, DEVICE_FS);
    USBD_RegisterClass(&hUsbDeviceFS, &USBD_CDC);
    USBD_CDC_RegisterInterface(&hUsbDeviceFS, &USBD_Interface_fops_FS);
    USBD_Start(&hUsbDeviceFS);
    return 1;
}


int  shell_driver_init( void )
{
    return 1;  /* already inited */
}


void shell_driver_reset( void )
{
    hal_vcp_reset();
}


int  shell_driver_read( char *buffer, int len )
{
    return 0;  /* not supported */
}


int  shell_driver_read_char( char *c )
{
    if( hal_vcp_getc( c, portMAX_DELAY ) == pdFAIL )
        return -1;
    else
        return (int)c;
}


int  shell_driver_read_char_blocked( char *c, int block_time )
{
    if( hal_vcp_getc( c, block_time ) == pdFAIL )
        return -1;
    else
        return (int)c;
}


int  shell_driver_read_is_empty( void )
{
    return 1;
}

#define WRITE_RETRY         5
#define WRITE_TIMEOUT_MS    1000
#define WRITE_TIMEOUT_TICK  (WRITE_TIMEOUT_MS*configTICK_RATE_HZ/1000)

int  shell_driver_write( const char *buffer, int len )
{
    int written=0;
    int retry;

    while( written < len )
    {
        retry = 0;
        while( hal_vcp_write( (char*)buffer, len ) == pdFAIL )
        {
            vTaskDelay(1);
            retry++;
            if( retry >= WRITE_RETRY )
                return written;
        }
        written += len;
    }
    return written;
}


void shell_driver_write_char( char c )
{
    int retry=0;
    
    while( hal_vcp_putc( c ) == pdFAIL )
    {
        vTaskDelay(1);
        retry++;
        if( retry >= WRITE_RETRY )
            return;
    }
}

