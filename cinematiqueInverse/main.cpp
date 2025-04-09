#include "cinematiqueInverse.hpp"
#include <iostream>
#include <windows.h>
#include <thread>
#include <chrono>
#include <stdexcept>
#include <array>
#include <numeric>

int main()
{
    constexpr double DEG_TO_RAD = M_PI / 180.0;
    constexpr int SHM_SIZE = 32; // 4 doubles = 32 bytes
    constexpr std::chrono::milliseconds LOOP_DELAY(15);

    // Moving average filter parameters
    constexpr size_t FILTER_SIZE = 5; // Adjustable size of the moving average window
    std::array<double, FILTER_SIZE> x_buffer = {0.0};
    std::array<double, FILTER_SIZE> y_buffer = {0.0};
    std::array<double, FILTER_SIZE> z_buffer = {0.0};
    std::array<double, FILTER_SIZE> stepper_buffer = {0.0};
    size_t buffer_index = 0;
    bool buffer_full = false;

    double angleCamera_deg = 0.0;
    double angleCamera_rad = angleCamera_deg * DEG_TO_RAD;
    parametres longueurs = {0.29, 0.122, 0.089, 0.0408};
    limitesMoteurs limites = {280.0, 130.0};
    coordonnees position;
    double angle_stepper;

    // Connect to existing shared memory
    HANDLE hMapFile = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, "CoordinatesSharedMemory");
    if (hMapFile == NULL)
    {
        std::cerr << "Failed to open shared memory: " << GetLastError() << std::endl;
        return 1;
    }

    void *pBuf = MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, SHM_SIZE);
    if (pBuf == NULL)
    {
        std::cerr << "Failed to map shared memory: " << GetLastError() << std::endl;
        CloseHandle(hMapFile);
        return 1;
    }

    while (true)
    {
        try
        {
            // Read raw coordinates from shared memory
            double *coords = static_cast<double *>(pBuf);
            double raw_x = coords[0];
            double raw_z = coords[1];
            double raw_y = coords[2];
            double raw_stepper = coords[3];

            if (raw_x != 0.0 || raw_z != 0.0 || raw_y != 0.0)
            {

                // Update circular buffers
                x_buffer[buffer_index] = raw_x;
                y_buffer[buffer_index] = raw_y;
                z_buffer[buffer_index] = raw_z;
                stepper_buffer[buffer_index] = raw_stepper;

                // Move to next index, wrap around if needed
                buffer_index = (buffer_index + 1) % FILTER_SIZE;
                if (buffer_index == 0)
                    buffer_full = true;

                // Calculate moving average
                size_t count = buffer_full ? FILTER_SIZE : buffer_index;
                position.x = std::accumulate(x_buffer.begin(), x_buffer.begin() + count, 0.0) / count;
                position.y = std::accumulate(y_buffer.begin(), y_buffer.begin() + count, 0.0) / count;
                position.z = std::accumulate(z_buffer.begin(), z_buffer.begin() + count, 0.0) / count;
                angle_stepper = std::accumulate(stepper_buffer.begin(), stepper_buffer.begin() + count, 0.0) / count;

                // std::cout << "Filtered coordinates - X: " << position.x << "\tZ: " << position.z << "\tY: " << position.y << "\tStepper: " << angle_stepper << std::endl;

                bool atteignable = validerPosition(position);
                // std::cout << "Atteignable: " << atteignable << std::endl;

                retourCinematiqueInverse anglesMoteurs = cinematiqueInverse(position, longueurs, limites, angleCamera_rad);

                if (!anglesMoteurs.reachable && atteignable)
                {
                    if (envoiAngles(anglesMoteurs.angle, angle_stepper))
                    {
                        std::cerr << "Failed to send angles to serial port" << std::endl;
                    }
                    else
                    {
                        /*std::cout << "Angles sent: Theta1=" << anglesMoteurs.angle.theta1
                                  << ", Theta2=" << anglesMoteurs.angle.theta2
                                  << ", Theta3=" << anglesMoteurs.angle.theta3
                                  << ", Stepper=" << angle_stepper << std::endl;*/
                    }
                }
            }
            else
            {
                std::cout << "Waiting for non-zero coordinates from Python...\n";
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error processing data: " << e.what() << std::endl;
        }

        std::this_thread::sleep_for(LOOP_DELAY);
    }

    UnmapViewOfFile(pBuf);
    CloseHandle(hMapFile);
    return 0;
}