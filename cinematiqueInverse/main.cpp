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
#include <sstream>  // Required for std::istringstream


int main()
{

    coordonnees position;


    double x = 0, y = 0, z = 0;

    // Angle de rotation de la caméra
    double angleCamera_deg = 0.0;
    double angleCamera_rad = angleCamera_deg * M_PI / 180.0;

    while(1) {
        std::string filename = "../data.txt";

        std::ifstream file(filename);
        if (file.is_open())
        {
            std::string line;

            if (std::getline(file, line))
            {
                // std::cout << "Read: " << line << std::endl;
                istringstream iss(line);

                iss >> x >> z >> y; // "Read:" est ignoré, et les trois nombres sont stockés

                std::cout << "x = " << x << ", y = " << z << ", z = " << y << std::endl;
            }
        }
        else
        {
            file.close();
            //std::cerr << "Error opening file" << std::endl;
        }

        // Coordonnees de la position voulue
        position.x = x;
        position.y = y;
        position.z = z;
        bool atteignable = validerPosition(position);

        printf("Atteignable: %d\n", atteignable);

        // selon le 3D (en m)
        parametres longueurs = {0.29, 0.122, 0.089, 0.0408};

        // limites moteurs (en degres)
        limitesMoteurs limites = {280.0, 130.0};

        // Angles des moteurs pour atteindre la position voulue
        retourCinematiqueInverse anglesMoteurs = cinematiqueInverse(position, longueurs, limites, angleCamera_rad);

        // printf("----------------------------------------\n");
        // printf("Theta1: %f\n", anglesMoteurs.angle.theta1);
        // printf("Theta2: %f\n", anglesMoteurs.angle.theta2);
        // printf("Theta3: %f\n", anglesMoteurs.angle.theta3);


        if (!anglesMoteurs.reachable && atteignable)
        {
            // printf("envoi angle");
            envoiAngles(anglesMoteurs.angle);

        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    return 0;
}