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


int main(){

    //Coordonnees de la position voulue
    coordonnees position = {0.00, 0.00, 0.20};
    bool atteignable = validerPosition(position);

    //selon le 3D (en m)
    parametres longueurs = {0.29, 0.122, 0.089, 0.0408};

    //limites moteurs (en degres)
    limitesMoteurs limites = {280.0, 130.0};

    // Angle de rotation de la caméra
    double angleCamera_deg = 0.0;
    double angleCamera_rad = angleCamera_deg * M_PI / 180.0;

    //Angles des moteurs pour atteindre la position voulue
    retourCinematiqueInverse anglesMoteurs = cinematiqueInverse(position, longueurs, limites, angleCamera_rad);

    printf("Theta1: %f\n", anglesMoteurs.angle.theta1);
    printf("Theta2: %f\n", anglesMoteurs.angle.theta2);
    printf("Theta3: %f\n", anglesMoteurs.angle.theta3);

        envoiAngles(anglesMoteurs.angle);
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
        } else {
            file.close();
            std::cerr << "Error opening file" << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Adjust polling rate
    if(!anglesMoteurs.reachable && atteignable){
    }

 

    return 0;

}