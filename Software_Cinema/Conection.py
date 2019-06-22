import serial
import time
import cv2
import numpy as np
from threading import Thread

isActive = True

class SerialTransfer(object):

    def __init__(self, num_port, speed=115200):
        self.ser = serial.Serial(num_port, speed)

    def Send_pkg(self, num_motor, direction, value_of_step):
        pkg = bytes([ord('#'), num_motor, direction]) + \
            value_of_step.to_bytes(2, byteorder='big')
        # print(value_of_step.to_bytes(2, byteorder='big'))
        print(pkg)
        self.ser.write(pkg)

    def getDebugLine(self):
        return self.ser.readline()


if __name__ == "__main__":

    import pygame
    pygame.init()
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

    MessageForSTM = SerialTransfer(num_port)

    # Thread dependent
    def prescript(serial_obj):
        while isActive:
            debug_line = serial_obj.getDebugLine()
            print('Debug line: {}'.format(debug_line.rstrip(b'\n\r')))

    thread_debug = Thread(target=prescript, args=[MessageForSTM])
    thread_debug.start()

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

                keys_pressed = pygame.key.get_pressed()

                if keys_pressed[pygame.K_DOWN]:
                    print('Down')
                    MessageForSTM.Send_pkg(vrt_motor, down_direction, step_value)

                if keys_pressed[pygame.K_UP]:
                    print('Up')
                    MessageForSTM.Send_pkg(vrt_motor, up_direction, step_value)

                if keys_pressed[pygame.K_RIGHT]:
                    print('Right')
                    MessageForSTM.Send_pkg(hrz_motor, right_direction, step_value)

                if keys_pressed[pygame.K_LEFT]:
                    print('Left')
                    MessageForSTM.Send_pkg(hrz_motor, left_direction, step_value)

    finally:
        pygame.quit()
        cv2.destroyAllWindows()
        isActive = False
        
