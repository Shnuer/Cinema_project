#include <ch.h>
#include <hal.h>

#include <chprintf.h>
#include <errno.h>

typedef enum
{
    STEPPER_MOTOR_HRZ = 1,
    STEPPER_MOTOR_VRT = 2
} stepper_motor_idx_t;

typedef enum
{
    STEPPER_MOTOR_DIRECTION_RIGTH = 1,
    STEPPER_MOTOR_DIRECTION_LEFT = 2
} stepper_motor_dir_t;

#define EOK 0

/*Controll setting*/
typedef struct
{
    stepper_motor_idx_t motor_index;
    stepper_motor_dir_t motor_direction;

    uint16_t value_of_step;
} motor_action_t;

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
    uint8_t motor_index;
    uint8_t motor_direction;

    uint16_t value_of_step;
} pkg_serial;

void StepperMotorInit(void)
{
    palSetPadMode(GPIOA, 3, PAL_MODE_OUTPUT_PUSHPULL); // Step VRT
    palSetPadMode(GPIOC, 0, PAL_MODE_OUTPUT_PUSHPULL); // Step HRZ
    palSetPadMode(GPIOC, 3, PAL_MODE_OUTPUT_PUSHPULL); // Dir VRT
    palSetPadMode(GPIOF, 3, PAL_MODE_OUTPUT_PUSHPULL); // Dir HRZ
}

void SerialCommInit()
{
    palSetPadMode(GPIOD, 8, PAL_MODE_ALTERNATE(7));
    palSetPadMode(GPIOD, 9, PAL_MODE_ALTERNATE(7));

    // palSetPadMode( GPIOD, 5, PAL_MODE_ALTERNATE(7) );   // TX = PD_5
    // palSetPadMode( GPIOD, 6, PAL_MODE_ALTERNATE(7) );   // RX = PD_6

    sdStart(comm_dr, &sdcfg);
}

int SerialCommGetPkg(pkg_serial *p_pkg)
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
        uint8_t rcv_buffer[4];
        int32_t rcv_bytes = sizeof(rcv_buffer);

        msg = sdReadTimeout(comm_dr, rcv_buffer, rcv_bytes, MS2ST(10));

        if (msg != rcv_bytes)
        {
            return EIO;
        }

        p_pkg->motor_index = rcv_buffer[0];
        p_pkg->motor_direction = rcv_buffer[1];
        p_pkg->value_of_step = (rcv_buffer[2] << 8) | (rcv_buffer[3]);
    }

    return EOK;
}

int vrt_delay = 20;
int hrz_delay = 20;

void makeStep(stepper_motor_idx_t motor_idx)
{
    if (motor_idx == STEPPER_MOTOR_HRZ)
    {
        palSetPad(GPIOC, 0);
        chThdSleepMilliseconds(hrz_delay);
        palClearPad(GPIOC, 0);
        chThdSleepMilliseconds(hrz_delay);
    } else if ( motor_idx == STEPPER_MOTOR_VRT ) {
        palSetPad(GPIOA, 3);
        chThdSleepMilliseconds(vrt_delay);
        palClearPad(GPIOA, 3);
        chThdSleepMilliseconds(vrt_delay);
    }
}

void makeNSteps(motor_action_t *action)
{
    for (int i = 0; i < action->value_of_step; ++i)
    {
        makeStep(action->motor_index);
    }


}

void SetTheMotorValueHorizontally(motor_action_t *action)
{
    if (action->motor_direction == STEPPER_MOTOR_DIRECTION_RIGTH)
    {
        palSetPad(GPIOF, 3);
    }
    else if (action->motor_direction == STEPPER_MOTOR_DIRECTION_LEFT)
    {
        palClearPad(GPIOF, 3);
    }

    makeNSteps(action);
}

void SetTheMotorValueVertically(motor_action_t *action)
{
    if (action->motor_direction == STEPPER_MOTOR_DIRECTION_RIGTH)
    {
        palSetPad(GPIOC, 3);
    }
    else if (action->motor_direction == STEPPER_MOTOR_DIRECTION_LEFT)
    {
        palClearPad(GPIOC, 3);
    }

    makeNSteps(action);
}

void UpdateStateMotor(motor_action_t *action)
{
    palToggleLine(LINE_LED1);

    if (action->motor_index == STEPPER_MOTOR_HRZ)
    {
        SetTheMotorValueHorizontally(action);
    }
    else if (action->motor_index == STEPPER_MOTOR_VRT)
    {
        SetTheMotorValueVertically(action);
    }
}

static THD_WORKING_AREA(waThread, 128);
static THD_FUNCTION(Thread, arg)
{
    arg = arg;

    while (true)
    {
        chThdSleepSeconds(1);
    }
}

int main(void)
{
    /* RT Core initialization */
    chSysInit();
    /* HAL (Hardware Abstraction Layer) initialization */
    halInit();

    chThdCreateStatic(waThread, sizeof(waThread), NORMALPRIO, Thread, NULL /* arg is NULL */);

    StepperMotorInit();
    motor_action_t new_action;

    SerialCommInit();
    pkg_serial input_pkg;

    while (true)
    {
        int result = SerialCommGetPkg(&input_pkg);

        if (result == EOK)
        {
            chprintf(comm_dr, "New info | idx: %d / dir: %d / val: %d\n",
                     input_pkg.motor_index,
                     input_pkg.motor_direction,
                     input_pkg.value_of_step);

            new_action.motor_index = input_pkg.motor_index;
            new_action.motor_direction = input_pkg.motor_direction;
            new_action.value_of_step = input_pkg.value_of_step;

            UpdateStateMotor( &new_action );
        }

        chThdSleepMilliseconds(10);
    }
}
