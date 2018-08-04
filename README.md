# adr
Data reader for the ARENA 313 modules.

## Important notes
This software assumes that there are two modes of samples, each numbered 0 and 3, respectively. If this changes, it will require alteration of the code.

This software also assumes that no more than 32,768 profiles will be processed at any one time. This needs to change eventually and will only require an occassional check to see if we need to reallocate for more memory. The decision was made not to pursue this at this time to keep the execution time low. This reallocation will take a relatively large amount of time.
