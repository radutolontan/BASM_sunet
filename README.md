# BASM_sunet

### Submodule Dependencies

This repository uses a submodule to process the spectral composition of the sound input on the Master MCU.

- **Repository**: [Realtime_PyAudio_FFT](https://github.com/aiXander/Realtime_PyAudio_FFT/tree/08db53f57fb4e2b04b113fb772d6fc1e0bd17256)
- **Author**: [Xander Steenbrugge](https://github.com/aiXander)
- **Installation**: After cloning BASM_sunet, run the following bash command inside the main directory
```bash
submodule update --init --recursive
```

### Structure of a frame

To communicate between the Master MCU and Monitor (or Slave) MCUs, the repository uses a serial protocol broadcast over an RS485 Bus. The protocol uses two types of frames, handshake and command.

1. **Hanshake [HS]** messages are meant to configure the Slave Boards at run-time, and establish communications to the Master Board.
- These are identified by a two-byte header [HEAD] {0xFF , 0xFE}
- Each byte in the data field represents a parameter that the Master sends to all Slaves to configure at run-time
    - Byte 1 - LED BRIGHTNESS ; 8-bit Integer [0-255]
- If the Acknowledge [ACK] message is enabled, the Slave will respond after recieving the configuration message from the Master by sending the 0xCA byte twice.
2. **Command [CMD]** messages are sent by the Master Board to each Slave Board to establish command values to the lightbars controlled by each Slave.
- These are identified by a two-byte header [HEAD] {0xDE , 0xAD}
- Each byte in the data field represents a command to a specific lightbar on a Slave board
    - in frame `0xDE 0xAD 0x05 0x09 0x02 0x04 0x06 0x08 0xAA`
        - `0x02` is the level requested from Bar 1 on Slave 5
        - `0x04` is the level requested from Bar 2 on Slave 5
        - `0xAA` is just a placeholder for the Checksum
    - Commands (or levels requested) need to be integers scaled between 0 and 255, such that they can be packed into a BYTE!

Together, the Header and Address are referenced within the repository as "Sync Sequence".
Each frame is protected by a checksum, ensuring a frame with a malformed header (e.g. with a corrupted length field) or a corrupted
payload is rejected.

```
,------+------+------+- - - -+-----------,
| HEAD | ADDR | LEN  | DATA  | CHECKSUM  |
|   2  |   1  |  1   |  ...  | 1         | <- size (bytes)
'------+------+------+- - - -+-----------'

HEAD ........ 2-byte message Header [see Note above]
ADDR  ....... 1-byte address of Slave Board [between 0x00 and 0x08]
LEN ......... number of bytes in the frame [including HEAD, ADDR, LEN, DATA, CHECKSUM]
DATA ........ LEN-5 bytes of data
DATA_CKSUM .. data checksum [computed using Modulo-256]
```