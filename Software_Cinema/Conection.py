import serial
import time
import cv2
import numpy as np
from threading import Thread
import math as m

isActive = True

class SerialTransfer(object):

    def __init__(self, num_port, speed=115200):
        # self.ser = serial.Serial(num_port, speed)
        pass

    def Send_pkg(self, num_motor, direction, value_of_step):
        pkg = bytes([ord('#'), num_motor, direction]) + \
            value_of_step.to_bytes(2, byteorder='big')
        # print(value_of_step.to_bytes(2, byteorder='big'))
        print(pkg)
        self.ser.write(pkg)

    def send_delays(self, hrz_delay, vrt_delay):
        pkg = bytes([ord('#'), 
                        np.int8(hrz_delay), 
                        np.int8(vrt_delay), 
                        np.uint8(hrz_delay * 2 + vrt_delay * 1.5)
                    ])
        print(pkg)
        # self.ser.write(pkg)

    def getDebugLine(self):
        return self.ser.readline(100)

def getDelayValue(norm_input):
    # norm_input = [-1;1]
    sign = lambda x: m.copysign(1, x)
    return sign(norm_input) * 20

if __name__ == "__main__":

    serial_disabled = False
    joystick_disabled = False

    import pygame
    pygame.init()

    hrz_value_delay = 0
    vrt_value_delay = 0
        
    try:
        joystick = pygame.joystick.Joystick(0)
        joystick.init()

        joy_vrt_axis = 4
        joy_hrz_axis = 0

    except:
        joystick_disabled = True

    pygame.display.set_caption("OpenCV camera stream on Pygame")
    screen = pygame.display.set_mode([640, 480])

    camera = cv2.VideoCapture(0)

    num_port = '/dev/ttyACM0'

    hrz_motor = 1
    vrt_motor = 2

    right_direction = 1
    left_direction = 2

    up_direction = right_direction
    down_direction = left_direction

    step_value = 10

    try:
        MessageForSTM = SerialTransfer(num_port)

        # Thread dependent
        def debug_thread(serial_obj):
            while isActive:
                debug_line = serial_obj.getDebugLine()
                print('Debug line: {}'.format(debug_line.rstrip(b'\n\r')))
        thread_debug = Thread(target=debug_thread, args=[MessageForSTM])
        thread_debug.start()
    except:
        serial_disabled = True
        MessageForSTM = None
            
    def send_thread(serial_obj):
        while isActive:
            # print('{} / {}'.format(hrz_value_delay, vrt_value_delay))
            time.sleep(0.2)
            if serial_obj is not None:
                serial_obj.send_delays(hrz_value_delay, vrt_value_delay)

    thread_send = Thread(target=send_thread, args=[MessageForSTM])
    thread_send.start()

    print('Start')
    
    try:
        while isActive:
            ret, frame = camera.read()
            # Convert frame to pygame format
            screen.fill([0, 0, 0])
            frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
            frame = np.rot90(frame)
            frame = pygame.surfarray.make_surface(frame)
            screen.blit(frame, (0, 0))
            pygame.display.update()

            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    pygame.quit()
                    isActive = False

                # keys_pressed = pygame.key.get_pressed()

                # if keys_pressed[pygame.K_DOWN]:
                #     print('Down')
                #     if not serial_disabled:
                #         MessageForSTM.Send_pkg(vrt_motor, down_direction, step_value)

                # if keys_pressed[pygame.K_UP]:
                #     print('Up')
                #     if not serial_disabled:
                #         MessageForSTM.Send_pkg(vrt_motor, up_direction, step_value)

                # if keys_pressed[pygame.K_RIGHT]:
                #     print('Right')
                #     if not serial_disabled:
                #         MessageForSTM.Send_pkg(hrz_motor, right_direction, step_value)

                # if keys_pressed[pygame.K_LEFT]:
                #     print('Left')
                #     if not serial_disabled:
                #         MessageForSTM.Send_pkg(hrz_motor, left_direction, step_value)

                if not joystick_disabled:
                    if event.type == pygame.JOYAXISMOTION:
                        if event.axis == joy_hrz_axis:
                            if abs(event.value) > 0.02: 
                                hrz_value_delay = getDelayValue(event.value)
                            else:
                                hrz_value_delay = 0

                        elif event.axis == joy_vrt_axis:
                            if abs(event.value) > 0.02:
                                vrt_value_delay = getDelayValue(-event.value)
                            else:
                                vrt_value_delay = 0
                        
                        # print(vrt_value_delay, hrz_value_delay)
                            
                    # if event.type == pygame.JOYBUTTONDOWN:
                        # print(event.button)
                    # elif event.type == pygame.JOYAXISMOTION:
                        # print(event.axis, event.value)

    finally:
        pygame.quit()
        cv2.destroyAllWindows()
        isActive = False
        
