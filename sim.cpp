/*
* Simulation of IT service request handling using SIMLIB/C++
 *
 * This program simulates the handling of on-site repair, diagnostics, and software installation requests
 * using a discrete event simulation approach. The simulation includes different types of workers
 * and processes, and it tracks various statistics such as queue lengths and success rates.
 *
 * Author: Zhdanovich Iaroslav, xzhdan00
 * Date: 30.11.2024
 */

#include <iostream>
#include <simlib.h>
#include <random>
#include <ctime>
#include <cmath>

#define SHIFT_TIME 480.0

// Statistics
int rideRequestQueueLength = 0;
int diagnosticsRequestQueueLength = 0;
int swInstallRequestQueueLength = 0;
int rideQueueLength = 0;
int diagnosticsQueueLength = 0;
int swInstallQueueLength = 0;
int rideRequestsCount = 0;
int diagnosticsRequestsCount = 0;
int swInstallRequestsCount = 0;

// Global variables
bool isOpened = true;
bool takingDDRequests = true;
int deploymentCount = 0;
int rideRequestsMade = 0;
int diagnosticsRequestsMade = 0;
int swInstallRequestsMade = 0;

// Arguments
int apprRideRequestsCount;
int apprDiagnosticsRequestsCount;
int apprSWInstallRequestsCount;
long unsigned OW = 3;
long unsigned RW = 2;
bool PRO = 1;


// Resources
Store OfficeWorkers("Office Workers", OW); // 3 office workers
Store Riders("Ride Workers", RW); // 2 ride workers
Store Universal("Universal Workers", 5); // 5 universal workers


// Work shift
class WorkShift : public Event
{
    void Behavior() override
    {
        isOpened = false;
    }
};


// Diagnostics and Deployment request taker
class DDRequestTaker : public Process
{
    void Behavior() override
    {
        takingDDRequests = false;
    }
};

// Normal distribution function
double normalDist(double mean) {
    mean = SHIFT_TIME / mean;
    static std::mt19937 gen(time(0));
    std::normal_distribution<> dist(mean, mean/4);

    double result;
    do {
        result = dist(gen);
    } while (Time + result <= Time);

    return result;
}


// Network deployment
class NetworkDeployment : public Process
{
    void Behavior() override
    {
        Priority = 1;
        rideQueueLength++;
        if (PRO) Enter(Universal, 1); else Enter(Riders, 1);
        Wait(120.0);
        if (PRO) Universal.SetCapacity(Universal.Capacity() + 1); else Riders.SetCapacity(Riders.Capacity() + 1);
        rideQueueLength--;
        Wait(SHIFT_TIME + 90.0 - Time);
        if (PRO) Leave(Universal, 1); else Leave(Riders, 1);
    }
};


// Ride(On-Site Repair)
class Ride : public Process
{
    void Behavior() override
    {
        Priority = 0;
        rideQueueLength++;
        if (PRO) Enter(Universal, 1); else Enter(Riders, 1);
        if (isOpened) {
            Wait(Uniform(10.0, 15.0));
            Wait(Exponential(40.0));
            Wait(Uniform(10.0, 15.0));
            rideQueueLength--;
        }
        if (PRO) Leave(Universal, 1); else Leave(Riders, 1);
    }
};


// Diagnostics
class Diagnostics : public Process
{
    void Behavior() override
    {
        Priority = 0;
        diagnosticsQueueLength++;
        if (PRO) Enter(Universal, 1); else Enter(OfficeWorkers, 1);
        if (isOpened) {
            Wait(Exponential(150.0));
            diagnosticsQueueLength--;
        }
        if (PRO) Leave(Universal, 1); else Leave(OfficeWorkers, 1);
    }
};


// Software install
class SWInstall : public Process
{
    void Behavior() override
    {
        Priority = 1;
        swInstallQueueLength++;
        if (PRO) Enter(Universal, 1); else Enter(OfficeWorkers, 1);
        if (isOpened) {
            Wait(Uniform(10.0, 20.0));
            swInstallQueueLength--;
        }
        if (PRO) Leave(Universal, 1); else Leave(OfficeWorkers, 1);
    }
};


// Ride request
class RideRequest : public Process {
    void Behavior() override {
        rideRequestsCount++;
        Priority = 2;
        rideRequestQueueLength++;
        if (PRO) Enter(Universal, 1); else Enter(OfficeWorkers, 1);
        Wait(3);
        rideRequestQueueLength--;
        if (PRO) Leave(Universal, 1); else Leave(OfficeWorkers, 1);
        if (Uniform(0.0, 1.0) < 0.1 && deploymentCount < 3 && takingDDRequests)
        {
            (new NetworkDeployment)->Activate();
            deploymentCount++;
        } else {
            (new Ride)->Activate();
        }
    }
};


// Diagnostics request
class DiagnosticsRequest : public Process {
    void Behavior() override {
        diagnosticsRequestsCount++;
        Priority = 2;
        diagnosticsRequestQueueLength++;
        if (PRO) Enter(Universal, 1); else Enter(OfficeWorkers, 1);
        Wait(5);
        if (PRO) Leave(Universal, 1); else Leave(OfficeWorkers, 1);
        diagnosticsRequestQueueLength--;
        (new Diagnostics)->Activate();
    }
};


// Software install request
class SWInstallRequest : public Process {
    void Behavior() override {
        swInstallRequestsCount++;
        Priority = 2;
        swInstallRequestQueueLength++;
        if (PRO) Enter(Universal, 1); else Enter(OfficeWorkers, 1);
        Wait(3);
        swInstallRequestQueueLength--;
        if (PRO) Leave(Universal, 1); else Leave(OfficeWorkers, 1);
        (new SWInstall)->Activate();
    }
};


// Ride request generator
class RideRequestGenerator : public Event {
    void Behavior() override {
        if (isOpened && rideRequestsMade < apprRideRequestsCount) {
            (new RideRequest)->Activate();
            rideRequestsMade++;
            Activate(Time + normalDist(apprRideRequestsCount));
        }
    }
};


// Diagnostics request generator
class DiagnosticsRequestGenerator : public Event {
    void Behavior() override {
        if (isOpened && takingDDRequests && diagnosticsRequestsMade < apprDiagnosticsRequestsCount)
        {
            (new DiagnosticsRequest)->Activate();
            diagnosticsRequestsMade++;
            Activate(Time + normalDist(apprDiagnosticsRequestsCount*2));
        }
    }
};


// Software install request generator
class SWInstallRequestGenerator : public Event {
    void Behavior() override {
        if (isOpened && swInstallRequestsMade < apprSWInstallRequestsCount)
        {
            (new SWInstallRequest)->Activate();
            swInstallRequestsMade++;
            Activate(Time + normalDist(apprSWInstallRequestsCount));
        }
    }
};


// Main function
int main(int argc, char *argv[]) {
    if (argc != 7) exit(1);
    apprRideRequestsCount = atoi(argv[1]);
    apprDiagnosticsRequestsCount = atoi(argv[2]);
    apprSWInstallRequestsCount = atoi(argv[3]);
    OW = atoi(argv[4]);
    RW = atoi(argv[5]);
    PRO = atoi(argv[6]);
    if (PRO) std::cout << "PRO mode enabled.\n";
    if (Riders.Capacity() != RW) Riders.SetCapacity(RW);
    if (OfficeWorkers.Capacity() != OW) OfficeWorkers.SetCapacity(OW);
    Universal.SetCapacity(5);
    std::cout << "Simulation started: \n";
    std::cout.flush();

    // Initialize the simulation
    RandomSeed(time(NULL));
    Init(0, 1000);

    // Activate request generators
    (new RideRequestGenerator)->Activate(Time + normalDist(apprRideRequestsCount));
    (new DiagnosticsRequestGenerator)->Activate(Time + normalDist(apprDiagnosticsRequestsCount*2));
    (new SWInstallRequestGenerator)->Activate(Time + normalDist(apprSWInstallRequestsCount));

    // Activate the work shift
    (new WorkShift)->Activate(SHIFT_TIME);
    (new DDRequestTaker)->Activate(SHIFT_TIME / 2);

    // Run the simulation
    Run();

    // Output statistics after the simulation
    if (PRO) Universal.Output(); else {
        OfficeWorkers.Output();
        Riders.Output();
    }

    std::cout << "Final Ride Request Queue Length: " << rideRequestQueueLength << '\n'; // Deployment queue length statistics
    std::cout << "Final Diagnostics Request Queue Length: " << diagnosticsRequestQueueLength << '\n'; // Diagnostics queue length statistics
    std::cout << "Final SW Install Request Queue Length: " << swInstallRequestQueueLength << '\n'; // SW install queue length statistics
    std::cout << "Final Ride Queue Length: " << rideQueueLength << '\n'; // Deployment queue length statistics
    std::cout << "Final Diagnostics Queue Length: " << diagnosticsQueueLength << '\n'; // Diagnostics queue length statistics
    std::cout << "Final SW Install Queue Length: " << swInstallQueueLength << '\n'; // SW install queue length statistics
    std::cout << "RIDE/DIA/SWI: " << rideRequestsCount << "/" << diagnosticsRequestsCount << "/" << swInstallRequestsCount << " TOTAL:" << rideRequestsCount + diagnosticsRequestsCount + swInstallRequestsCount << '\n'; // Total requests statistics
    std::cout << "Success rate: " <<  ((float)((rideRequestsCount + diagnosticsRequestsCount + swInstallRequestsCount)-(rideQueueLength + diagnosticsQueueLength + swInstallQueueLength)) / (float)(rideRequestsCount + diagnosticsRequestsCount + swInstallRequestsCount))*100 << "%\n"; // Success rate statistics
    std::cout << "Simulation ended.\n";
    return 0;
}