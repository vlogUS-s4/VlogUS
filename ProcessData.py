import numpy
import time

class PID:
    def __init__(self, kp: float, ki: float, kd: float, setpoint: float = 0.0):
        """
        Initialise le contrôleur PID.
        :param kp: Gain proportionnel
        :param ki: Gain intégral
        :param kd: Gain dérivé
        :param setpoint: Valeur cible
        """
        self.kp = kp
        self.ki = ki
        self.kd = kd
        self.setpoint = setpoint
        
        self.previous_error = 0.0
        self.integral = 0.0
        self.last_time = None

    def compute(self, measured_value: float, current_time: float):
        """
        Calcule la sortie du PID.
        :param measured_value: Valeur mesurée
        :param current_time: Temps actuel (en secondes)
        :return: Commande PID
        """
        error = self.setpoint - measured_value
        delta_time = 0 if self.last_time is None else (current_time - self.last_time)
        
        # Calcul de la partie intégrale
        self.integral += error * delta_time
        
        # Calcul de la partie dérivée
        derivative = 0 if delta_time == 0 else (error - self.previous_error) / delta_time
        
        # Calcul de la sortie PID
        output = self.kp * error + self.ki * self.integral + self.kd * derivative
        
        # Mise à jour des variables pour la prochaine itération
        self.previous_error = error
        self.last_time = current_time
        
        return output


class RobotController:
    def __init__(self):
        self.pidX = PID(0.3, 0, 0, 20)
        self.pidY = PID(0.3, 0, 0, 50)
        self.pidZ = PID(0.3, 0, 0, 50)
        self.pidStepper = PID(0.3, 0, 0, 50)
        self.outputX = 0
        self.outputY = 0
        self.outputZ = 0
        self.outputStepper = 0


    def process(self, faces):
        self.outputX = (self.pidX.compute(faces[3], time.time())) / 100
        self.outputY = (self.pidY.compute(faces[0], time.time())) / 100
        self.outputZ = (abs(self.pidZ.compute(faces[1], time.time())) + 14) / 100
        reachable = self.validatePosition(self.outputX, self.outputY, self.outputZ)
        if not reachable:
            self.outputStepper = (self.pidStepper.compute(faces[0], time.time()))/100


    def printData(self):
                
        with open("data.txt", "w") as file:
            output_str = f"{self.outputX:.2f}\t{self.outputZ:.2f}\t{self.outputY:.2f}"

            file.write(str(output_str))
        file.close()

    def validatePosition(x,y,z):
        reachable = False
        if z >= 0.2 & z <=0.38:
            max_x_y = (z**5+15741*z**4-8654.7*z**3+2355.5*z**2-317.38*z+17.017)
            if x <= max_x_y | y <= max_x_y:
                reachable = True
        elif z >= 0.14 & z <= 0.19:
            max_x_y = (-31250*z**4+22060*z**3-5782.3*z**2+668.41*z-28.773)
            if x <= max_x_y | y <= max_x_y:
                reachable = True
        return reachable
#Hauteur: 15cm à 38cm
