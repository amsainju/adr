# adr
Data reader for the ARENA 313 modules.

## Current status
Linux development is underway. Contact me for more fine-grain updates.

## Release version
The release version of this software may be downloaded from the 'binaries' subdirectory.

## Important notes

This software also assumes that no more than 32,76800 profiles will be processed at any one time. This needs to change eventually and will only require an occassional check to see if we need to reallocate for more memory. The decision was made not to pursue this at this time to keep the execution time low. This reallocation will take a relatively large amount of time.

The sample length is hardcoded at 32768 samples per profile to avoid having to parse the XML files. If you need to alter this value, the XML parse function will have to be implemented.

At this time, there is no need to have XML files in the same directory as the .dat files. All pertinent information is pulled from the radar headers themselves.


## Usage
In order to use this software, call this command from the Terminal:

```
$./adr -i infile
```
where *infile* is the input data file to begin processing.

Optionally, you may use the following parameters:
```
$./adr -i infile -m 5
```
where *5* is the maximum number of data files to process.
```
$./adr -i infile -r 16384
```
where 16384 is the number of range gates (this defaults to 32768 if this parameter is not given).
```
$./adr -i infile -sm 3
```
where 3 is the short mode value (this defaults to 1 if this parameter is not given).
```
```
$./adr -i infile -pc 400000
```
where 400000 is the profile count value (this defaults to 32,76800 if this parameter is not given).
```

./adr -h
```
to show usage information.
```
./adr -i infile -d
```
to show debug information while executing.

Note that you do not have to use this executable from the same directory as the data files, so long as you provide the complete path to the executable. Use double quotes if there is a space in the path.

## MATLAB import
This software stores data in MATLAB matrices in a .mat file that uses the file naming convention of the data files from whence the data came. For example, if the data came from a data file named '20180709_180156_Channel7_0000.dat', it would be named '20180709_180156_Channel7_0000.mat' and located in the same folder from which you executed the binary.

To import this data into MATLAB, you would simply use the command:
```
load('.\PATH\TO\FILE\20180709_180156_Channel7_0000.mat')
```

At this point, four matrices should be visible in your workspace:
* Long_Chirp_PPS_Count_Values
* Long_Chirp_Profiles
* Short_Chirp_PPS_Count_Values
* Short_Chirp_Profiles

Their names should be self-explanatory.

## Compilation 
Before you run the compilation command make sure libmat.so and libmx.so  are present in the same directory as adr.c adr.h and main.c. 
```
$ export PATH=/YOUR_PATH/Matlab2018a/bin:$PATH

$ export LD_LIBRARY_PATH=/YOUR_PATH/Matlab2018a/bin/glnxa64:$LD_LIBRARY_PATH

$ gcc -L /YOUR_PATH/Matlab2018a/bin/glnxa64 -L /YOUR_PATH/Matlab2018a/sys/os/glnxa64 -I /YOUR_PATH/Matlab2018a/extern/include -o adr adr.c main.c -lm -lmat -lmx
```

