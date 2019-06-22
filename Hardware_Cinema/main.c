#include <ch.h>
#include <hal.h>

#include <chprintf.h>
#include <errno.h>


#define STEPPER_MOTOR_DIRECTION_RIGHT 1
#define STEPPER_MOTOR_DIRECTION_LEFT 2

#define STEPPER_MOTOR_DIRECTION_UP 1
#define STEPPER_MOTOR_DIRECTION_DOWN 2

#define STEPPER_MOTOR_HRZ 1
#define STEPPER_MOTOR_VRT 2

int8_t horizontal_delay_value = 0;
int8_t vertical_delay_value = 0;




#define EOK 0

/*Controll setting*/




/*Serial setting*/
static const SerialConfig sdcfg = {
    .speed = 115200, /* baudrate, directly number */
    .cr1 = 0,
    .cr2 = 0,
    .cr3 = 0};

// SerialDriver *comm_dr = &SD2;
SerialDriver *comm_dr = &SD3;

typedef struct
{
    int8_t hrz_motor_delay;
    int8_t vrt_motor_delay;

} pkg_serial_t;

void StepperMotorInit(void)
{
    palSetPadMode(GPIOA, 3, PAL_MODE_OUTPUT_PUSHPULL); // Step VRT
    palSetPadMode(GPIOC, 0, PAL_MODE_OUTPUT_PUSHPULL); // Step HRZ
    palSetPadMode(GPIOC, 3, PAL_MODE_OUTPUT_PUSHPULL); // Dir VRT
    palSetPadMode(GPIOF, 3, PAL_MODE_OUTPUT_PUSHPULL); // Dir HRZ
}

void SerialCommInit(void)
{
    palSetPadMode(GPIOD, 8, PAL_MODE_ALTERNATE(7));
    palSetPadMode(GPIOD, 9, PAL_MODE_ALTERNATE(7));

    // palSetPadMode( GPIOD, 5, PAL_MODE_ALTERNATE(7) );   // TX = PD_5
    // palSetPadMode( GPIOD, 6, PAL_MODE_ALTERNATE(7) );   // RX = PD_6

    sdStart(comm_dr, &sdcfg);
}

int SerialCommGetPkg(pkg_serial_t *p_pkg)
{
    if (p_pkg == NULL)
        return EINVAL;

    msg_t msg = sdGetTimeout(comm_dr, MS2ST(10));
    if (msg < 0)
    {
        return EIO;
    }

    char start_byte = msg;
    if (start_byte == '#')
    {
        int8_t rcv_buffer[3];
        int32_t rcv_bytes = sizeof(rcv_buffer);

        msg = sdReadTimeout(comm_dr, rcv_buffer, rcv_bytes, MS2ST(10));

        if (msg != rcv_bytes)
        {
            return EIO;
        }

        p_pkg->hrz_motor_delay = rcv_buffer[0];
        p_pkg->vrt_motor_delay = rcv_buffer[1];
        uint8_t pkg_chk = rcv_buffer[2];
        uint8_t pkg_chk_calc = (int)(p_pkg->hrz_motor_delay * 2. + p_pkg->vrt_motor_delay * 1.5);

        if(pkg_chk != pkg_chk_calc)
        {
            return EIO;
        }
  
    }

    return EOK;
}


void hrz_direction(uint8_t direction_hrz)
{
    if(direction_hrz == STEPPER_MOTOR_DIRECTION_RIGHT)
    {
        palSetPad(GPIOF, 3);
    }
    else if(direction_hrz == STEPPER_MOTOR_DIRECTION_LEFT)
    {
        palClearPad(GPIOF, 3);
    }
}



void Make_hrz_step(uint8_t delay_hrz)
{
    palSetPad(GPIOC, 0);
    chThdSleepMilliseconds(delay_hrz);
    palClearPad(GPIOC, 0);
    chThdSleepMilliseconds(delay_hrz);
}


void vrt_direction(uint8_t direction_vrt)
{
    if(direction_vrt == STEPPER_MOTOR_DIRECTION_UP)
    {
        palSetPad(GPIOC, 3);
    }
    else if(direction_vrt == STEPPER_MOTOR_DIRECTION_DOWN)
    {
        palClearPad(GPIOC, 3);
    }
}


void Make_vrt_step(uint8_t delay_vrt)
{
        palSetPad(GPIOA, 3);
        chThdSleepMilliseconds(delay_vrt);
        palClearPad(GPIOA, 3);
        chThdSleepMilliseconds(delay_vrt);
}


void Hrz_action(int8_t delay_hrz)
{

    if (delay_hrz>0)
    {
        hrz_direction(STEPPER_MOTOR_DIRECTION_RIGHT);
    }
    else
    {   
        hrz_direction(STEPPER_MOTOR_DIRECTION_LEFT);
        delay_hrz = delay_hrz * (-1);
    }

    Make_hrz_step(delay_hrz);
}


void Vrt_action(int8_t delay_vrt)
{

    if (delay_vrt>0)
    {
        vrt_direction(STEPPER_MOTOR_DIRECTION_UP);
    }
    else
    {   
        vrt_direction(STEPPER_MOTOR_DIRECTION_DOWN);
        delay_vrt = delay_vrt * (-1);
    }

    Make_vrt_step(delay_vrt);
}





static THD_WORKING_AREA(waHrz_action_n, 128);
static THD_FUNCTION(Hrz_action_n, arg)
{
    arg = arg;

    while (true)
    {
        if (horizontal_delay_value == 0)
        {
            chThdSleepMilliseconds(10);
        }
        else
        {
            Hrz_action(horizontal_delay_value);
        }
        
    }
}

static THD_WORKING_AREA(waVrt_action_n, 128);
static THD_FUNCTION(Vrt_action_n, arg)
{
    arg = arg;

    while (true)
    {
        if (vertical_delay_value == 0)
        {
            chThdSleepMilliseconds(10);
        }
        else
        {
            Vrt_action(vertical_delay_value);
        }
    }
}

int main(void)
{
    /* RT Core initialization */
    chSysInit();
    /* HAL (Hardware Abstraction Layer) initialization */
    halInit();

    chThdCreateStatic(waHrz_action_n, sizeof(waHrz_action_n), NORMALPRIO, Hrz_action_n, NULL /* arg is NULL */);

    chThdCreateStatic(waVrt_action_n, sizeof(waVrt_action_n), NORMALPRIO, Vrt_action_n, NULL /* arg is NULL */);

    StepperMotorInit();


    SerialCommInit();
    pkg_serial_t input_pkg;


    while (true)
    {
        int result = SerialCommGetPkg(&input_pkg);

        if (result == EOK)
        {
            chprintf(comm_dr, "New info | hrz_del: %d / vrt_del: %d\n",
                     input_pkg.hrz_motor_delay,
                     input_pkg.vrt_motor_delay);

            horizontal_delay_value = input_pkg.hrz_motor_delay;
            vertical_delay_value = input_pkg.vrt_motor_delay;
        }

        chThdSleepMilliseconds(10);
    }
}
