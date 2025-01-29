#include "cinematiqueInverse.hpp"
#include <iostream>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <cstring>

#define SERIAL_PORT "/dev/ttyACM0"

int main(){

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
    coordonnees position = {0, 0, 0.215};

    //selon le 3D (en m)
    parametres longueurs = {0.21, 0.093, 0.089, 0.045};

    //Angles des moteurs pour atteindre la position voulue
    angles anglesMoteurs = cinematiqueInverse(position, longueurs);

    printf("Theta1: %f\n", anglesMoteurs.theta1);
    printf("Theta2: %f\n", anglesMoteurs.theta2);
    printf("Theta3: %f\n", anglesMoteurs.theta3);

    char message[50];
    snprintf(message, sizeof(message), "%.2f,%.2f,%.2f", anglesMoteurs.theta1, anglesMoteurs.theta2, anglesMoteurs.theta3);
    int bytes_written = write(serial_fd, message, strlen(message));
    if (bytes_written < 0){
        cerr << "Erreur lors de l'ecriture sur le port serie" << endl;
    }
    else{
        cout << bytes_written << endl;
    }

    close(serial_fd);
    
    return 0;

}