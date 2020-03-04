NS3-4Gand5GEnergyComparison
This is my final year project for my Computing (BSc) degree at the University of Portsmouth.

Description and Purpose
This project aims to compare the energy efficiency of 4G and 5G cellular technologies using various case studies. The case studies will emulate everyday uses of LTE with mobile phones e.g. walking around whilst opening multiple HTTP/S web pages.

The purpose of this project is to determine if the claim that 5G will be more energ efficient than 4G for mobile devices is true and to what extent.

The NS3 Energy Model will be utilised to retrieve the power consumption of the ue (mobile phones) and the Mobility Model will be used to simulate the movement.

How to Run
Prequisites: You will need to have NS-3 installed (I am running version 3.30).

You must copy the .cc files from this project to the 'Scratch' directory.
Files are run from the 'ns-3-dev' folder with the use of the './waf --run' command, example below ./waf --run scratch/HTTPRandomWalk //Note that the file extension is not included
Command line options can be inputted at runtime and will overwrite the default options set in each file, example below ./waf --run 'scratch/HTTPRandomWalk --simTime=500' //This will run the file for 500 seconds instead of the default time set in the file.