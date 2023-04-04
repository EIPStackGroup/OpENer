[![Quality Gate Status](https://sonarcloud.io/api/project_badges/measure?project=EIPStackGroup_OpENer&metric=alert_status)](https://sonarcloud.io/summary/new_code?id=EIPStackGroup_OpENer)
[![Join the chat at https://gitter.im/EIPStackGroupOpENer/Lobby](https://badges.gitter.im/EIPStackGroupOpENer/Lobby.svg)](https://gitter.im/EIPStackGroupOpENer/Lobby?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)


OpENer Version 2.3.0
====================

Welcome to OpENer!
------------------

OpENer is an EtherNet/IP&trade; stack for I/O adapter devices; supports multiple 
I/O and explicit connections; includes objects and services to make EtherNet/IP&trade;-
compliant products defined in THE ETHERNET/IP SPECIFICATION and published by 
ODVA (http://www.odva.org).

Participate!
------------
Users and developers of OpENer can join the respective Google Groups in order to exchange experience, discuss the usage of OpENer, and to suggest new features and CIP objects, which would be useful for the community.

Developers mailing list: https://groups.google.com/forum/#!forum/eip-stack-group-opener-developers

Users mailing list: https://groups.google.com/forum/#!forum/eip-stack-group-opener-users

Requirements:
-------------
OpENer has been developed to be highly portable. The default version targets PCs
with a POSIX operating system and a BSD-socket network interface. To test this 
version we recommend a Linux PC or Windows with Cygwin (http://www.cygwin.com) 
installed. You will need to have the following installed:

* CMake
* gcc
* make
* binutils
* the development library of libcap (libcap-dev or equivalient)
 
for normal building. These should be installed on most Linux installations and
are part of the development packages of Cygwin.

If you want to run the unit tests you will also have to download CppUTest via
https://github.com/cpputest/cpputest

For configuring the project we recommend the use of a CMake GUI (e.g., the 
cmake-gui package on Linux, or the Installer for Windows available at [CMake](https://cmake.org/))

Compile for Linux/POSIX:
----------------
1. Make sure all the needed tools are available (CMake, make, gcc, binutils)
2. Change to the <OpENer main folder>/bin/posix
3. For a standard configuration invoke ``setup_posix.sh``
	1. Invoke the ``make`` command
	2. Invoking OpENer:

		``./src/ports/POSIX/OpENer <interface_name>``

		e.g. ``./src/ports/POSIX/OpENer eth1``

OpENer also now has a real-time capable POSIX startup via the OpENer_RT option, which requires that the used kernel has the full preemptive RT patches applied and activated.
If you want to use OpENer_RT, prior to step 2, execute ``sudo setcap cap_ipc_lock,cap_sys_nice+ep ./src/ports/POSIX/OpENer
`` to grant OpENEr ``CAP_SYS_NICE``, and the ``CAP_IPC_LOCK`` capabilities, which are needed for the RT mode

OpENer can also be built and installed as a library by setting the CMake flag `-DOPENER_INSTALL_AS_LIB`.  To build a shared library,
the global option `-DBUILD_SHARED_LIBS=ON` should also be set.  It has only been tested under Linux/POSIX platform.


Compile for Windows XP/7/8 via Visual Studio:
---------------------------------------------
1. Invoke setup_windows.bat or configure via CMake
2. Open Visual Studio solution OpENer.sln in bin/win32
3. Compile OpENer by chosing ``Build All`` in Visual Studio
4. For invoking OpENer type from the command line:
	1. Change to <OpENer main folder>\bin\win32\src\ports\WIN32\
	2. Depending if you chose the ``Debug`` or ``Release`` configuration in Visual Studio, your executable will either show up in the subfolder Debug or Release
	3. Invoke OpENer via

		``OpENer <interface_index>``

		e.g. ``OpENer 3``

In order to get the correct interface index enter the command ``route print`` in a command promt and search for the MAC address of your chosen network interface at the beginning of the output. The leftmost number is the corresponding interface index.
		
Compile for Windows XP/7/8/10 via Cygwin:
--------------------------------------
The POSIX setup file can be reused for Cygwin. Please note, that you cannot use RT mode and you will have to remove the code responsible for checking and getting the needed capabilities, as libcap is not available in Cygwin. The easier and more supported way to build OpENer for Windows is to either use MinGW or Visual Studio.

Compile for MinGW on Windows XP/7/8/10
-------------------------------
1. Make sure 64 bit mingw is installed. (Test with gcc --version, should show x86_64-posix-seh-rev1)
2. Make sure CMake is installed. (Test with cmake --version, should be version 3.xx)
3. Change to <opener install dir>/bin/mingw
4. Run the command `setup_mingw.bat` in a dos command line. (Not a bash shell). If tracing is desired, 
use the following (where the cmake parameter must be enclosed in quotes) or change the ./source/CMakeList.txt file.
    ```
    setup_mingw.bat "-DOpENer_TRACES:BOOL=TRUE"
    ```
5. Run the command "make" from the same directory (./bin/mingw)
6. The opener.exe is now found in <opener install dir>\bin\mingw\src\ports\MINGW
7. Start it like this: "opener 192.168.250.22", where the ip address is the local computer's address on the nettwork you want to use.
		
Directory structure:
--------------------
- bin ...  The resulting binaries and make files for different ports
- doc ...  Doxygen generated documentation (has to be generated for the SVN version) and Coding rules
- data ... EDS file for the default application
- source
	- src ... the production source code
		- cip ... the CIP layer of the stack
		- cip_objects ... additional CIP objects
		- enet_encap ... the Ethernet encapsulation layer
		- ports ... the platform specific code
		- utils ... utility functions
	- tests ... the test source code
		- enet_encap ... tests for Ethernet encapsulation layer
		- utils ... tests for utility functions

Documentation:
--------------
The documentation of the functions of OpENer is part of the source code. The source 
packages contain the generated documentation in the directory doc/api_doc. If you 
use the GIT version you will need the program Doxygen for generating the HTML 
documentation. You can generate the documentation by invoking doxygen from the 
command line in the opener main directory.


Fuzzing
--------------
### Intro
Fuzzing is an automated testing method that directs varying input data to a program in 
order to monitor output. It is a way to test for overall reliability as well as identify 
potential security bugs.

The fuzzer we are using is AFL, a fuzzer that uses runtime guided techniques to create input for the tested program. From a high-level prespective AFL works as follows:
- Forks the fuzzed process
- Genereates a new test case based on a predefined input
- Feeds the fuzzed process with the test case through STDIN
- Monitors the execution and registers which paths are reachable

![Alt text](fuzz/imgs/fuzz.png "AFL Fuzzing")

### Compile
To start fuzzing this project with AFL you'll need to compile it with AFL.
First make sure you have AFL installed:
```
sudo apt install build-essential
wget http://lcamtuf.coredump.cx/afl/releases/afl-latest.tgz
tar xzf afl-latest.tgz
cd afl*
make && sudo make install
echo "AFL is ready at: $(which afl-fuzz)"

```

Then, compile OpENer with AFL:
1. Change to the ``OpENer/bin/posix`` directory
2. Compile OpENer with AFL ``./setup_posix_fuzz_afl.sh`` 
3. Run ``make``

### Fuzz
Finally, generate some test cases and start AFL:
```
# Generate inputs
mkdir inputs
echo 630000000000000000000000000000000000000000000000 | xxd -r -p > ./inputs/enip_req_list_identity
# You can also use the inputs we prepared from OpENer/fuzz/inputs
# Finally, let's fuzz!
afl-fuzz -i inputs -o findings ./src/ports/POSIX/OpENer <interface_name>
```

### Reproduce a crash
Usually to reproduce a crash it's enough to retransmit the testcase using ``cat testcase | nc IP_ADDR 44818``
However, since CIP runs over the EtherNet/IP layer, it must first register a valid session. Therefore, we need to use a dedicated script:
`python fuzz/scripts/send_testcase.py IP testcase_path`

Running an OpENer "swarm":
--------------------------

1. Create a macvlan network for this purpose and tie it to the desired eth port.
Specify the IP range and use aux address to exclude the addresses used by other devices in the subnet such as the IP of the EIP scanner PC, network bridge, etc.:
docker network create -d macvlan --subnet=192.168.135.253/24 --ip-range=192.168.135.100/24 --aux-address="PC1=192.168.135.250" --aux-address="VM=192.168.135.252" --aux-address="BR=192.168.135.253" -o parent=eth2 mac_vlan_network

Check the network you created with: docker network inspect mac_vlan_network

The network will assign IP's to the docker containers and an external scanner will be able to communicate with them. To access the containers from inside the docker host, you will have to create a bridge.

2. Create a Dockerfile.
This uses Ubuntu as the base image. It will copy OpENer to the image root and install the required packages. Lastly run OpENer on eth0 of the image:
#Filename: Dockerfile
FROM ubuntu:20.04
ADD ./bin/posix/src/ports/POSIX/OpENer /
RUN apt-get update && apt-get install -y --no-install-recommends libcap-dev nmap
ENTRYPOINT ["./OpENer", "eth0"]

3. Create a docker-compose.yml that will let you connect the macvlan network to the containers and easily build them and tear them down:
version: "3.3"
services:
dockerimagename:
network_mode: mac_vlan_network
image: dockeruser/dockerimagename

Note that to login to a running container, you have to expose a port in the dockerfile and dockercompose files and set up a network bridge.

Docker commands to start and stop multiple instances of the OpENer containers:
Start up 128 docker image instances: docker-compose up --scale dockerimagename=128 -d
Shut down all the instances: docker-compose down

Porting OpENer:
---------------
For porting OpENer to new platforms please see the porting section in the 
Doxygen documentation.

Contributing to OpENer:
-----------------------
The easiest way is to fork the repository, then create a feature/bugfix branch.
After finishing your feature/bugfix create a pull request and explain your changes.
Also, please update and/or add doxygen comments to the provided code sections.
Please stick to the coding conventions, as defined in source/doc/coding_rules
The easiest way to conform to the indenting convertion is to set uncrustify as git filter in the OpENer repository, which can be done with the following to commands:

```
git config filter.uncrustify.clean "/path/to/uncrustify/uncrustify -c uncrustify.cfg --mtime --no-backup"

git config filter.uncrustify.smudge "cat"
```


