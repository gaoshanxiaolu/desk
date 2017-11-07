****** Instructions on building the Demo application with different feature sets********

To conserve memory, multiple versions of libramcust are provided to support a combination of feature sets.
The Makefile can be modified to include a particular version of the libramcust library to enable the desired feature.

Three configurations are provided in this release- 


1. libramcust.a
This is the default library.

Primary Features Supported- All standard Networking Features- SSL, HTTP etc.
Not Supported- ECC, P2P

2. libramcust_p2p.a

Primary Features Supported- a. P2P
                            b. All standard Networking Features- SSL, HTTP etc. 
Not Supported- ECC

3. libecc_demo.a

Primary Features Supported- a. ECC
                            b. All standard Networking Features- SSL, HTTP etc.
Not Supported- P2P


