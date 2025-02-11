#include "cinematiqueInverse.hpp"
#include <iostream>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <cstring>
#include <fstream>
#include <string>
#include <thread>
#include <chrono>

#define SERIAL_PORT "/dev/ttyACM0"

int main(){
/* 
    int serial_fd = open(SERIAL_PORT, O_RDWR | O_NOCTTY | O_NDELAY);
    if(serial_fd == -1){
        cerr<< "Impossible d'ouvrir le port serie!" << endl;
        return -1;
    }

    struct termios options;
    tcgetattr(serial_fd, &options);
    cfsetispeed(&options, B9600);
    cfsetospeed(&options, B9600);
    options.c_cflag = CS8 | CLOCAL | CREAD;
    options.c_iflag = IGNPAR;
    options.c_oflag = 0;
    options.c_lflag = 0;
    tcflush(serial_fd, TCIFLUSH);
    tcsetattr(serial_fd, TCSANOW, &options);

    //Coordonnees de la position voulue
    coordonnees position = {0.1, 0.0, 0.16};

    //selon le 3D (en m)
    parametres longueurs = {0.21, 0.093, 0.089, 0.045};

    //limites moteurs (en degres)
    limitesMoteurs limites = {270.0, 135.0};

    // Angle de rotation de la caméra (exemple : 30 degrés en radians)
    double angleCamera_deg = 45.0;
    double angleCamera_rad = angleCamera_deg * M_PI / 180.0;

    //Angles des moteurs pour atteindre la position voulue
    retour anglesMoteurs = cinematiqueInverse(position, longueurs, limites, angleCamera_rad);

    printf("Theta1: %f\n", anglesMoteurs.angle.theta1);
    printf("Theta2: %f\n", anglesMoteurs.angle.theta2);
    printf("Theta3: %f\n", anglesMoteurs.angle.theta3);

    if(!anglesMoteurs.reachable){

        char message[50];
        snprintf(message, sizeof(message), "%.2f,%.2f,%.2f", anglesMoteurs.angle.theta1, anglesMoteurs.angle.theta2, anglesMoteurs.angle.theta3);
        int bytes_written = write(serial_fd, message, strlen(message));
        if (bytes_written < 0){
            cerr << "Erreur lors de l'ecriture sur le port serie" << endl;
        }

        close(serial_fd);
    
    } */

   std::string filename = "../data.txt";
    
    while (true) {
        std::ifstream file(filename);
        if (file.is_open()) {
            std::string line;
            if (std::getline(file, line)) {
                // std::cout << "Read: " << line << std::endl;
                   std::istringstream iss(line);

                    float x, y, z;

                    iss >> x >> y >> z;  // "Read:" est ignoré, et les trois nombres sont stockés

                    std::cout << "x = " << x << ", y = " << y << ", z = " << z << std::endl;
            }
            file.close();
        } else {
            std::cerr << "Error opening file" << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Adjust polling rate
    }

 

    return 0;

}