# Multrix

Multrix is a tool developed as a part of a project pursued at the [HPC Research Group](http://hpc.nitk.ac.in/) at National Institute of Technology Karnataka.

It uses dynamic binary instrumentation techniques to automatically construct the Node Dependency Matrix(NDM) of an input binary executable.

## Usage

To create a linked library of the Multrix pintool
```bash
make obj-intel64/multrix.so
```

To run the pintool on sample test programs
```bash
pin.sh -t obj-intel64/multrix.so -- ./Test_codes/test_binaries/&lt;binary_name&gt; 
```

## Working

Details about the working of this tool can be found at this [blog](http://karthiksenthil.github.io/2015/08/18/multrix/).

Note: Pin is the Intel DBI framework which can be downloaded [here](https://software.intel.com/en-us/articles/pintool-downloads)