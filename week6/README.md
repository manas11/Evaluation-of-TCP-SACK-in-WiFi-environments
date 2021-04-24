## Comparsion of different TCP variants with SACK option enabled/disabled

* This directory contains results of simulation of different TCP variants including, `TCP BIC`, `TCP CUBIC`, `TCP High Speed`, `TCP HTCP`, `TCP Scalable`, `TCP Vegas` and `TCP Westwood`.
* The parameters analyzed includes, congestion window, sequence number and throughput.

The topology used is shown in the below figure.

![Wireshark output TCP Option4 SACK_PERM](../images/topology.png)

The cofiguration of the network is included in the below table.

| Payload size | Application layer datarate | Phy layer Bitrate | AP0 to STA01 distance | AP0 to STA01 distance | AP1 to STA10 distance |
|----------|------|--------|-------|-------|--------|
|1472 bytes|50Mbps|HtMcs7|120|130|140|

## Results

The throghuput related results can be found in the file [TCP_variants_Tthroughput.csv](./TCP_variants_throughput.csv).

### TCP BIC

#### Congestion window
![Wireshark output TCP Option4 SACK_PERM](./TcpBic/TcpBic-cwnd.png)

#### Transmitted Sequence number
![Wireshark output TCP Option4 SACK_PERM](./TcpBic/TcpBic-next-tx.png)

### TCP CUBIC

#### Congestion window
![Wireshark output TCP Option4 SACK_PERM](./TcpCubic/TcpCubic-cwnd.png)

#### Transmitted Sequence number
![Wireshark output TCP Option4 SACK_PERM](./TcpCubic/TcpCubic-next-tx.png)

### TCP High Speed

#### Congestion window
![Wireshark output TCP Option4 SACK_PERM](./TcpHighSpeed/TcpHighSpeed-cwnd.png)

#### Transmitted Sequence number
![Wireshark output TCP Option4 SACK_PERM](./TcpHighSpeed/TcpHighSpeed-next-tx.png)

### TCP HTCP

#### Congestion window
![Wireshark output TCP Option4 SACK_PERM](./TcpHtcp/TcpHtcp-cwnd.png)

#### Transmitted Sequence number
![Wireshark output TCP Option4 SACK_PERM](./TcpHtcp/TcpHtcp-next-tx.png)


### TCP Scalable

#### Congestion window
![Wireshark output TCP Option4 SACK_PERM](./TcpScalable/TcpScalable-cwnd.png)

#### Transmitted Sequence number
![Wireshark output TCP Option4 SACK_PERM](./TcpScalable/TcpScalable-next-tx.png)

### TCP Vegas

#### Congestion window
![Wireshark output TCP Option4 SACK_PERM](./TcpVegas/TcpVegas-cwnd.png)

#### Transmitted Sequence number
![Wireshark output TCP Option4 SACK_PERM](./TcpVegas/TcpVegas-next-tx.png)

### TCP Westwood

#### Congestion window
![Wireshark output TCP Option4 SACK_PERM](./TcpWestwood/TcpWestwood-cwnd.png)

#### Transmitted Sequence number
![Wireshark output TCP Option4 SACK_PERM](./TcpWestwood/TcpWestwood-next-tx.png)
