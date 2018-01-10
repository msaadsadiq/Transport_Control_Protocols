# Transport_Control_Protocols


The following Transport Control Protocols were implemented in this project
1. Stop N Wait
2. Go Back N
3. Selective Repeat


In this project the objective was to design a simple transport protocol that provides reliable datagram
service. The protocol should be able to ensure that data is delivered in order and without duplicates. The
developed protocol was tested on an emulated unreliable network. Our project consists of two programs: a
sending program that sends a file across the network, and a receiving program that receives the file and
stores it back to the local disk. The project code was written in C on a Linux operating system. No
transport protocol libraries were used in the project. The packets were constructed and acknowledgements
by the sender and receiver programs by interpreting the incoming packets.
The following objectives were achieved in the project:
1. The sender reads a local file and uses the developed protocol to send it.
2. The file contents are transferred reliably.
3. The receiver writes the contents it receives into the local directory.
4. The program transfers small as well as large file sizes.
5. A custom designed packet uses UDP as a carrier to transmit packets.
6. The packet includes fields for packet type, ack number, data and retransmission requests.
7. A retransmission protocol was developed to deal with dropped, duplicated, and delayed packets.
8. Three popular protocols were implemented.
9. The designed protocol is fast such that it requires comparatively little time to transfer the file.
10. The requires low overhead data volume to be exchanged over the network to conduct the transfer.


2. Implementation Details
This section discusses the program implementations in detail and provide the different evaluation criteria
for the 3 different protocols implemented.
2.1 Creating a dummy file
Our program can handle file of any size. We created a dummy file in Ubuntu, using the following
command line:
base64 /dev/urandom | head -c 10000000 > file.txt //This created a file with name file.txt size of 10MB.
2.2 Emulating Unreliable Network
In order to test the developed project on an unreliable network, the code was emulated using the netem
program, which is a simple network packet loss emulation tool in Linux. In order to use netem, first the
iproute package was installed through the Ubuntu package manager and the following packet loss
configurations were tested. A sample of running the netem emulator is shown below.

2.2.1 Creating a Secret Pass key
Since networks are unreliable and unsafe, a secret pass key was also added as part of the unreliable
network defenses.
2.2.2 Time out & Window size
Timeout was set to be .5 seconds for all three types of protocols. Window size was kept to 5 for easy
tracing of the packets visually. There is no limitation on window size and the program was tested with all
different sizes ranging from 1 to the maximum possible window to contain all packets.


2.3 Simulation 1.
This simulation will be run under perfect network conditions without any delay, loss or duplication
This Configuration says
1. 0% of the packets will be delay
2. Packet loss rate is 0%
3. Packet duplication rate is 0%


2.4 Simulation 2.
Sudo tc qdisc add dev lo root netem delay 100ms 20ms 25% loss 0.5% duplicate 1% reorder 25% 50%
This configuration says:
1. Delay is 100ms ± 20ms with the next random element depending 25% on the last one.
2. Packet loss rate is 0.5%
3. Packet duplicate rate is 1%•
4. 25% of packets (with a correlation of 50%) will get sent immediately, others will be delayed.
Simulation Results of the 3 methods are shown below


2.5 Simulation 3
Sudo tc qdisc add dev lo root netem delay 100ms
This configuration says:
1. Constant Delay of 100ms is added to outgoing packets of the external interface localhost
Simulation Results of the 3 methods are shown below

2.6 Simulation 4.
Sudo tc qdisc add dev lo root netem delay 20ms 30ms 40% loss 0.5% duplicate 5% reorder 25% 50%
This configuration says:
5. Delay is 20ms ± 30ms with the next random element depending 40% on the last one.
6. Packet loss rate is 0.5%
7. Packet duplicate rate is 5%•
8. 25% of packets (with a correlation of 50%) will get sent immediately, others will be delayed.
Simulation Results of the 3 methods are shown below


2.7 Ending the simulation
Finally the simulations were ended by disabling the netem delay product on the localhost interface by
deleting the rules using the following.
Sudo tc qdisc del dev lo root netem

3. Comparing Performances of the three Protocols
Stop & Wait
The major shortcoming of the stop-and-wait protocol is that requires that the sender to have only one
outgoing frame on the sending media at any given time. The sender waits till it gets the ACK back of sent
frame before sending the next frame. This causes substantial amount of network bandwidth wastage. To
improve efficiency while providing reliability, "sliding window" protocols are a better option.
Go-Back-N vs Sliding Window
The sliding window protocols does not waste any network bandwidth as compared to the Stop-N-Wait
protocol both in normal and in congested conditions. Both Sliding window protocols GBN and SR show
better performance than Stop-N-Wait as shown in the comparison graph below.
1. It was observed that delay is the biggest contributing factor to the performance in all three
implemented protocols.
2. The Stop & Wait protocol is good for small file sizes under perfect network conditions but it
performed the worst under deteriorated network conditions.
3. Since netem emulated delays and corruption in bursts, some packets of Stop-N-Wait kept
delaying for as long as 150 cycles, whereas the other ones were quicker.
4. Selective Repeat protocol was the best performing of the three

4. Conclusion
The objective of this project was to implement several different transport protocols using C language in a
Linux based environment and emulate real transmission conditions. To achieve the desired objectives the
following three Transport Protocols were implemented
1. Stop N Wait
2. Go Back N
3. Selective Repeat
Their efficiencies were also compared using a virtual network environment created using the netem
emulator that we used. For the performance comparison of the developed protocols all three protocols
were compared in both normal conditions and real-life emulations. In the normal condition there is no
packet loss and the protocols performed fast and reliably. In the real life emulation or otherwise also
called the congested condition there were some packet loss some packet delay and duplication as
controlled by our emulator. All three protocols were compared in the given conditions and results were
reported.
