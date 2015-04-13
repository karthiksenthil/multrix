MULTRIX
===================
----------

Compile instructions
-------
To create the linked library of the Multrix pintool
> make obj-intel64/multrix.so

Running instructions
-------

> pin.sh -t obj-intel64/multrix.so  -- ./Test_codes/test_binaries/&lt;binary_name&gt;


** Note : **
Pin is the Intel DBI framework which can be downloaded [here](https://software.intel.com/en-us/articles/pintool-downloads)  
