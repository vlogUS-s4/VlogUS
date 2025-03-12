from gpiozero import OutputDevice
import time

class StepperMotor:
    def __init__(self, dir_pin, step_pin, enable_pin=None):
        self.dir_pin = OutputDevice(dir_pin)
        self.step_pin = OutputDevice(step_pin)
        self.enable_pin = OutputDevice(enable_pin) if enable_pin else None
        self.position = 0

    def move(self, steps, step_delay):
        if steps >= 0:
            self.dir_pin.value = 1
        else:
            self.dir_pin.value = 0
        steps = abs(steps)
        for _ in range(steps):
            self.step_pin.on()
            time.sleep(step_delay)
            self.step_pin.off()
            time.sleep(step_delay)
            if self.dir_pin.value == 1:
                self.position += 1
            else:
                self.position -= 1

    def get_position(self):
        return self.position
