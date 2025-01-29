#ifndef cinematiqueInverse_HPP
#define cinematqueInverse_HPP

#include <cmath>
#include <array>

using namespace std;

struct angles
{
    float theta1;
    float theta2;
    float theta3;
};

struct coordonnees
{
    float x;
    float y;
    float z;
};

struct parametres
{
    float longAvantBras;
    float longBicep;
    float rayonBase;
    float rayonEffecteur;
};

array<float, 2> calculAngle(coordonnees position, parametres longueurs);
angles cinematiqueInverse(coordonnees position, parametres longueurs);

array<float, 2> calculAngle(coordonnees position, parametres longueurs){

    //result: [angle, singularity] (0: no singularity, 1: singularity)
    array<float, 2> result = {0, 0};

    position.y = position.y - longueurs.rayonEffecteur;

    float z_offset = ((pow(position.x, 2)) + (pow(position.y, 2)) + (pow(position.z, 2)) + (pow(longueurs.longBicep, 2)) - (pow(longueurs.longAvantBras, 2)) + (pow(longueurs.rayonBase, 2)))/(2*position.z);
    float z_slope = (-longueurs.rayonBase - position.y)/(position.z);

    float reachable = -(pow((z_offset + z_slope*(-longueurs.rayonBase)), 2)) + longueurs.longBicep*(pow(z_slope, 2)*longueurs.longBicep + longueurs.longBicep);

    if (reachable < 0) {
        result[1] = 1;
    } 
    else {
        float coordy = (-longueurs.rayonBase - z_offset*z_slope - sqrt(reachable))/(pow(z_slope, 2) + 1);
        float coordz = z_offset + z_slope*coordy;
        result[0] = 180 * atan(-coordz/(-longueurs.rayonBase - coordy))/M_PI;

        if (coordy > -longueurs.rayonBase){
            result[0] = result[0] + 180;
        }
    }

    return result;

}

angles cinematiqueInverse(coordonnees position, parametres longueurs){
    
    angles result;
    array<float, 2> theta1 = {0, 0};
    array<float, 2> theta2 = {0, 0};
    array<float, 2> theta3 = {0, 0};

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

    if (theta3[1] == 0){
        result.theta1 = theta1[0];
        result.theta2 = theta2[0];
        result.theta3 = theta3[0];
    }
    else{
        result.theta1 = 0;
        result.theta2 = 0;
        result.theta3 = 0;
    }

    return result;
}

#endif