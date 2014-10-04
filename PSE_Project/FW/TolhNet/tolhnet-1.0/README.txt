This is the ToLHnet source code distribution.

Visit http://www.tolhnet.org/ for updates, documentation, and contact information.

The project is split into two main parts:
 - the firmware, under the "fw" folder.
 - the code for the master controller, intended to be run under a Linux OS, under the "master" folder.

The ToLHnet network layer proper is included in the "network.cpp", from which "network.c" is automatically
derived, and "network.h" files.
These have been thoroughly revised and the code is considered production quality.
The rest of the firmware is just an example, with known limitations.
The master controller code incorporates a daemon which is to be considered an alpha release, with some features still unimplemented.

Licensing:
Most of the firmware is released under the Boost Software License (BSL), except for proprietary drivers.
See individual file headers for details.
The master controller code is released under the GNU General Public License.
You can find license texts in the appropriate files in this folder.
