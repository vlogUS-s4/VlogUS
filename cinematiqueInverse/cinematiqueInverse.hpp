#ifndef cinematiqueInverse_HPP
#define cinematqueInverse_HPP

#include <cmath>
#include <array>
#include <stdio.h>
#include <iostream>
#include <sstream>
#include <windows.h> // Windows API for serial communication
#include <iomanip>

#define SERIAL_PORT "COM7" // Remains the same, Windows uses COM ports

using namespace std;

struct angles
{
    double theta1;
    double theta2;
    double theta3;
};

struct coordonnees
{
    double x;
    double y;
    double z;
};

struct parametres
{
    double longAvantBras;
    double longBicep;
    double rayonBase;
    double rayonEffecteur;
};

struct limitesMoteurs
{
    double moteur_max;
    double moteur_min;
};

struct retourCinematiqueInverse
{
    angles angle;
    bool reachable;
};

array<double, 2> calculAngle(coordonnees position, parametres longueurs);
retourCinematiqueInverse cinematiqueInverse(coordonnees position, parametres longueurs, limitesMoteurs limites);
bool validerAngles(angles angle, limitesMoteurs limites);
bool validerPosition(coordonnees position);
bool envoiAngles(angles angle);

array<double, 2> calculAngle(coordonnees position, parametres longueurs)
{

    // result: [angle, singularity] (0: no singularity, 1: singularity)
    array<double, 2> result = {0, 0};

    position.y = position.y - longueurs.rayonEffecteur;

    double z_offset = ((pow(position.x, 2)) + (pow(position.y, 2)) + (pow(position.z, 2)) + (pow(longueurs.longBicep, 2)) - (pow(longueurs.longAvantBras, 2)) + (pow(longueurs.rayonBase, 2))) / (2 * position.z);
    double z_slope = (-longueurs.rayonBase - position.y) / (position.z);

    double reachable = -(pow((z_offset + z_slope * (-longueurs.rayonBase)), 2)) + longueurs.longBicep * (pow(z_slope, 2) * longueurs.longBicep + longueurs.longBicep);

    if (reachable < 0)
    {
        result[1] = 1;
    }
    else
    {
        double coordy = (-longueurs.rayonBase - z_offset * z_slope - sqrt(reachable)) / (pow(z_slope, 2) + 1);
        double coordz = z_offset + z_slope * coordy;
        result[0] = atan2(-coordz, -longueurs.rayonBase - coordy) * 180 / M_PI;
        result[0] = 180 - result[0];
    }

    return result;
}

bool validerAngles(angles angle, limitesMoteurs limites)
{

    bool result = 0;

    if (angle.theta1 > limites.moteur_max || angle.theta1 < limites.moteur_min ||
        angle.theta2 > limites.moteur_max || angle.theta2 < limites.moteur_min ||
        angle.theta3 > limites.moteur_max || angle.theta3 < limites.moteur_min)
    {

        printf("Mecaniquement pas possible\n");

        result = 1;
    }

    return result;
}

retourCinematiqueInverse cinematiqueInverse(coordonnees position, parametres longueurs, limitesMoteurs limites, double angleCamera)
{
    // printf("Position initiale: %f, %f, %f\n", position.x, position.y, position.z);

    // Appliquer la rotation du référentiel mobile
    coordonnees positionTransformee;
    positionTransformee.x = position.x * cos(angleCamera) - position.y * sin(angleCamera);
    positionTransformee.y = position.x * sin(angleCamera) + position.y * cos(angleCamera);
    positionTransformee.z = position.z;

    // printf("Position transformee: %f, %f, %f\n", positionTransformee.x, positionTransformee.y, positionTransformee.z);

    array<double, 2> theta1 = calculAngle(positionTransformee, longueurs);
    array<double, 2> theta2 = {0, 0};
    array<double, 2> theta3 = {0, 0};
    retourCinematiqueInverse valeurRetour;

    if (theta1[1] == 0)
    {
        coordonnees position2;
        position2.x = positionTransformee.x * cos(120 * M_PI / 180) + positionTransformee.y * sin(120 * M_PI / 180);
        position2.y = positionTransformee.y * cos(120 * M_PI / 180) - positionTransformee.x * sin(120 * M_PI / 180);
        position2.z = positionTransformee.z;
        theta2 = calculAngle(position2, longueurs);
    }

    if (theta2[1] == 0)
    {
        coordonnees position3;
        position3.x = positionTransformee.x * cos(120 * M_PI / 180) - positionTransformee.y * sin(120 * M_PI / 180);
        position3.y = positionTransformee.y * cos(120 * M_PI / 180) + positionTransformee.x * sin(120 * M_PI / 180);
        position3.z = positionTransformee.z;
        theta3 = calculAngle(position3, longueurs);
    }

    valeurRetour.angle.theta1 = theta1[0];
    valeurRetour.angle.theta2 = theta2[0];
    valeurRetour.angle.theta3 = theta3[0];

    // if(theta3[1] == 0 && validerAngles(valeurRetour.angle, limites) == 0){
    //     valeurRetour.reachable = 0;
    // }

    // else{
    //     valeurRetour.reachable = 1;
    // }

    valeurRetour.reachable = (theta3[1] == 0) ? validerAngles(valeurRetour.angle, limites) : 1;

    // printf("Reachable: %b\n", valeurRetour.reachable);

    return valeurRetour;
}

bool validerPosition(coordonnees position)
{

    // Si atteignable: 1 / pas atteignable: 0
    bool reachable = 0;

    if (position.z >= 0.2 && position.z <= 0.38)
    {

        double max_x_y = (-11343 * pow(position.z, 5) + 15741 * pow(position.z, 4) - 8654.7 * pow(position.z, 3) + 2355.5 * pow(position.z, 2) - 317.38 * position.z + 17.017);

        // printf("Max_x_y: %f\n", max_x_y);

        if (position.x <= max_x_y || position.y <= max_x_y)
        {
            reachable = 1;
            // printf("Position atteignable avec ball bearing\n");
        }
    }

    else if (position.z >= 0.14 && position.z <= 0.19)
    {

        double max_x_y = (-31250 * pow(position.z, 4) + 22060 * pow(position.z, 3) - 5782.3 * pow(position.z, 2) + 668.41 * position.z - 28.773);

        // printf("Max_x_y: %f\n", max_x_y);

        if (position.x <= max_x_y || position.y <= max_x_y)
        {
            reachable = 1;
            // printf("Position atteignable avec ball bearing\n");
        }
    }

    return reachable;
}

bool envoiAngles(angles angle, double angle_stepper)
{
    HANDLE hSerial = CreateFileA(SERIAL_PORT,
                                 GENERIC_READ | GENERIC_WRITE,
                                 0,
                                 NULL,
                                 OPEN_EXISTING,
                                 FILE_ATTRIBUTE_NORMAL,
                                 NULL);
    if (hSerial == INVALID_HANDLE_VALUE)
    {
        DWORD error = GetLastError();
        printf("Erreur: Impossible d'ouvrir le port serie %s (Code: %lu)\n", SERIAL_PORT, error);
        return true;
    }

    DCB dcbSerialParams = {0};
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    if (!GetCommState(hSerial, &dcbSerialParams))
    {
        printf("Erreur: Impossible de recuperer l'etat du port\n");
        CloseHandle(hSerial);
        return true;
    }

    dcbSerialParams.BaudRate = CBR_9600;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;
    if (!SetCommState(hSerial, &dcbSerialParams))
    {
        printf("Erreur: Impossible de configurer le port\n");
        CloseHandle(hSerial);
        return true;
    }

    COMMTIMEOUTS timeouts = {0};
    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = 50;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    timeouts.WriteTotalTimeoutConstant = 50;
    timeouts.WriteTotalTimeoutMultiplier = 10;
    if (!SetCommTimeouts(hSerial, &timeouts))
    {
        printf("Erreur: Impossible de configurer les timeouts\n");
        CloseHandle(hSerial);
        return true;
    }

    std::ostringstream msg;
    msg << std::fixed << std::setprecision(2)
        << angle.theta1 << " "
        << angle.theta2 << " "
        << angle.theta3 << " "
        << angle_stepper << "\n";
    std::string data = msg.str();

    DWORD bytesWritten;
    if (!WriteFile(hSerial, data.c_str(), data.length(), &bytesWritten, NULL))
    {
        DWORD error = GetLastError();
        printf("Erreur: Echec de l'ecriture sur le port (Code: %lu, Bytes tentes: %zu, Bytes ecrits: %lu)\n",
               error, data.length(), bytesWritten);
        CloseHandle(hSerial);
        return true;
    }

    printf("Envoyé: %s", data.c_str());
    CloseHandle(hSerial);
    return false;
}

#endif