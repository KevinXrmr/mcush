#include "mcush.h"
#include "task_blink.h"

#define ERR_LED  0

static int _errno = 0;

int set_errno(int new)
{
    int old=_errno;
    _errno = new;
    return old;
}


int cmd_error( int argc, char *argv[] )
{
    mcush_opt_parser parser;
    mcush_opt opt;
    const mcush_opt_spec opt_spec[] = {
        { MCUSH_OPT_SWITCH, "stop", 's', 0, "stop blink", MCUSH_OPT_USAGE_REQUIRED },
        { MCUSH_OPT_ARG, "errno", 0, 0, "error number from 0 to 999", MCUSH_OPT_USAGE_REQUIRED },
        { MCUSH_OPT_NONE } };
    int new = -1;
    uint8_t stop=0;

    mcush_opt_parser_init(&parser, opt_spec, (const char **)(argv+1), argc-1 );
    while( mcush_opt_parser_next( &opt, &parser ) )
    {
        if( opt.spec )
        {
            if( strcmp( opt.spec->name, "errno" ) == 0 )
            {
                if( ! shell_eval_int(opt.value, (int*)&new) )
                    new = 0; 
            }
            else if( strcmp( opt.spec->name, "stop" ) == 0 )
                stop = 1;
        }
        else
            STOP_AT_INVALID_ARGUMENT 
    }

    if( stop )
    {
        hal_led_clr( ERR_LED );
        set_errno(-1);
        return 0;
    }

    if( new == -1 )
    {
        if( _errno < 0 )
             shell_write_line( "stop" );
        else
             shell_printf( "%d\n", _errno );
        return 0;
    }
    
    if( (new < 0) || (new > 100000000) )
        goto failed;
    set_errno(new);
    return 0;
failed:
    shell_write_line( "range 0~100000000" );
    return 1;
}

const shell_cmd_t cmd_tab_blink[] = {
{   0, 'e', "error",  cmd_error, 
    "set error number",
    "error -s <number>"  },
{   CMD_END  } };


#if !defined(MCUSH_NON_OS)

#define DELAY_A   200*configTICK_RATE_HZ/1000  /* on for 1~9 */
#define DELAY_B   500*configTICK_RATE_HZ/1000  /* off for 1~9 */
#define DELAY_C  1000*configTICK_RATE_HZ/1000  /* on for 0 */
#define DELAY_D  2000*configTICK_RATE_HZ/1000  /* delay cycle */

void blink_digit( int digit )
{
    int i;

    if( digit < 0 )
        return;

    if( digit )
    {
        for( i=digit; i; i-- )
        {
            hal_led_set(ERR_LED);
            vTaskDelay(DELAY_A);
            hal_led_clr(ERR_LED);
            vTaskDelay(DELAY_A);
        }
    }
    else
    {
        hal_led_set(ERR_LED);
        vTaskDelay(DELAY_C);
        hal_led_clr(ERR_LED);

    }
    vTaskDelay(DELAY_B);
}



TaskHandle_t  task_blink;

void task_blink_entry(void *p)
{
    int i, digit, pos, skip;

    while( 1 )
    {
        i = _errno;
        if( i < 0 )
        {
            taskYIELD();
            continue;
        }
        if( i == 0 )
            blink_digit( 0 );
        else
        {
            /* digit from 1 ~ 100000000 */
            for( pos=9, skip=1; pos; pos-- )
            {
                digit = (i / 100000000) % 10;
                i -= digit * 100000000;
                i *= 10;
                if( skip && !digit && (pos != 1) )
                    continue;
                blink_digit( digit );
                skip = 0;
            }
        }
        vTaskDelay( DELAY_D );
    }
}


void task_blink_init(void)
{
    shell_add_cmd_table( cmd_tab_blink );
    xTaskCreate(task_blink_entry, (const char *)"blinkT", 
                TASK_BLINK_STACK_SIZE / sizeof(portSTACK_TYPE),
                NULL, TASK_BLINK_PRIORITY, &task_blink);
    if( !task_blink )
        halt("create blink task");
    mcushTaskAddToRegistered((void*)task_blink);
}
#else

event_t event_blink = EVT_INIT;



void task_blink_entry(void)
{
    if( event_blink & EVT_INIT )
    {
        shell_add_cmd_table( cmd_tab_blink );
        event_blink &= ~EVT_INIT;
    }
    else if( event_blink & EVT_TIMER )
    {
        event_blink &= ~EVT_TIMER;
    //    if( _errno )
    //    {
    //        blink_errorno();
    //        vTaskDelay(2000*configTICK_RATE_HZ/1000);
    //    }
    //    else
    //    {
    //        hal_led_toggle(0);
    //        vTaskDelay(1000*configTICK_RATE_HZ/1000);
    //    }
    }
}



#endif