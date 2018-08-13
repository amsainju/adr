# adr
Data reader for the ARENA 313 modules.

## Release version
The release version of this software may be downloaded from the 'binaries' subdirectory.

## Important notes
Please ensure that your MATLAB binary folder is on your system (or user) path. For example, for a default MATLAB R2018a installation, this would be 'C:\Program Files\MATLAB\R2018a\bin\win64'.

This software assumes that there are two modes of samples, each numbered 0 and 3, respectively. *If this changes, it will require alteration of the code.*

This software also assumes that no more than 32,768 profiles will be processed at any one time. This needs to change eventually and will only require an occassional check to see if we need to reallocate for more memory. The decision was made not to pursue this at this time to keep the execution time low. This reallocation will take a relatively large amount of time.

The sample length is hardcoded at 32768 samples per profile to avoid having to parse the XML files. If you need to alter this value, the XML parse function will have to be implemented.

At this time, there is no need to have XML files in the same directory as the .dat files. All pertinent information is pulled from the radar headers themselves.

This software has only been tested on Windows 10 Enterprise (64-bit) to this point.

## Usage
In order to use this software, call this command from the Windows command line:

```
adr.exe -i infile
```
where *infile* is the input data file to begin processing.

Optionally, you may use the following parameters:
```
adr.exe -i infile -m 5
```
where *5* is the maximum number of data files to process.
```
adr.exe -h
```
to show usage information.
```
adr.exe -i infile -d
```
to show debug information while executing.

Note that you do not have to use this executable from the same directory as the data files, so long as you provide the complete path to the executable. Use double quotes if there is a space in the path.

## MATLAB import
This software stores data in MATLAB matrices in a .mat file that uses the file naming convention of the data files from whence the data came. For example, if the data came from a data file named '20180709_180156_Channel7_0000.dat', it would be named '20180709_180156.mat' and located in the same folder from which you executed the binary.

To import this data into MATLAB, you would simply use the command:
```
load('C:\PATH\TO\FILE\20180709_180156.mat')
```

At this point, four matrices should be visible in your workspace:
* Long_Chirp_PPS_Count_Values
* Long_Chirp_Profiles
* Short_Chirp_PPS_Count_Values
* Short_Chirp_Profiles

Their names should be self-explanatory.
