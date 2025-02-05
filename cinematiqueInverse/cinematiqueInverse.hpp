#ifndef cinematiqueInverse_HPP
#define cinematqueInverse_HPP

#include <cmath>
#include <array>
#include <stdio.h>

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

struct retour
{
    angles angle;
    bool reachable;
};

array<double, 2> calculAngle(coordonnees position, parametres longueurs);
retour cinematiqueInverse(coordonnees position, parametres longueurs, limitesMoteurs limites);
bool validerAngles(angles angle, limitesMoteurs limites);

array<double, 2> calculAngle(coordonnees position, parametres longueurs){

    //result: [angle, singularity] (0: no singularity, 1: singularity)
    array<double, 2> result = {0, 0};

    position.y = position.y - longueurs.rayonEffecteur;

    double z_offset = ((pow(position.x, 2)) + (pow(position.y, 2)) + (pow(position.z, 2)) + (pow(longueurs.longBicep, 2)) - (pow(longueurs.longAvantBras, 2)) + (pow(longueurs.rayonBase, 2)))/(2*position.z);
    double z_slope = (-longueurs.rayonBase - position.y)/(position.z);

    double reachable = -(pow((z_offset + z_slope*(-longueurs.rayonBase)), 2)) + longueurs.longBicep*(pow(z_slope, 2)*longueurs.longBicep + longueurs.longBicep);

    if (reachable < 0) {
        result[1] = 1;
    } 
    else {
        double coordy = (-longueurs.rayonBase - z_offset*z_slope - sqrt(reachable))/(pow(z_slope, 2) + 1);
        double coordz = z_offset + z_slope*coordy;
        result[0] = atan2(-coordz, -longueurs.rayonBase - coordy) * 180 / M_PI;
        result[0] = 180 - result[0];
    }

    return result;

}

bool validerAngles(angles angle, limitesMoteurs limites){

    bool result = 0;

    if(angle.theta1 > limites.moteur_max || angle.theta1 < limites.moteur_min ||
       angle.theta2 > limites.moteur_max || angle.theta2 < limites.moteur_min ||
       angle.theta3 > limites.moteur_max || angle.theta3 < limites.moteur_min){

        printf("Mecaniquement pas possible\n");

        result = 1;

       }

    return result;
}

retour cinematiqueInverse(coordonnees position, parametres longueurs, limitesMoteurs limites){
    
    printf("Position: %f, %f, %f\n", position.x, position.y, position.z);

    array<double, 2> theta1 = {0, 0};
    array<double, 2> theta2 = {0, 0};
    array<double, 2> theta3 = {0, 0};
    retour valeurRetour;

    theta1 = calculAngle(position, longueurs);

    if (theta1[1] == 0){
        
        coordonnees position2;
        position2.x = position.x*cos(120*M_PI/180) + position.y*sin(120*M_PI/180);
        position2.y = position.y*cos(120*M_PI/180) - position.x*sin(120*M_PI/180);
        position2.z = position.z;

        theta2 = calculAngle(position2, longueurs);
    }

    if (theta2[1] == 0){

        coordonnees position3;
        position3.x = position.x*cos(120*M_PI/180) - position.y*sin(120*M_PI/180);
        position3.y = position.y*cos(120*M_PI/180) + position.x*sin(120*M_PI/180);
        position3.z = position.z;

        theta3 = calculAngle(position3, longueurs);
    }

    valeurRetour.angle.theta1 = theta1[0];
    valeurRetour.angle.theta2 = theta2[0];
    valeurRetour.angle.theta3 = theta3[0];

    if (theta3[1] == 0){
        printf("No singularity\n");
        valeurRetour.reachable = validerAngles(valeurRetour.angle, limites);
    }

    else{
        printf("Singularity\n");
        valeurRetour.reachable = 1;
    }

    return valeurRetour;
}

#endif