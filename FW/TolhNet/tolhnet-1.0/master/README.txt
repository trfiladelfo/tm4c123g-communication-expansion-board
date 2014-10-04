This folder contains the code to be run on the master node.
It is a daemon, that reads the network configuration specified in the "tolhnet.conf" file
and listens for incoming TCP connections on port 7018.
The control protocol is textual, so it can be tested by simply telnetting to port 7018.
A somewhat vague explanation of the control protocol can be found in the "doc" folder.

This code can also generate routing tables from network topology.
Just call the executable with the "--route" option,
and instead of starting the daemon, a routing table will be generated and saved.
If you have the graphviz package, a nice representation of the network will be saved
in files under /tmp.
