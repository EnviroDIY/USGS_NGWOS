# Notes on the MultiTech Conduit

## Help documents
[MultiTech Conduit AEP](https://www.thethingsindustries.com/docs/gateways/models/multitechconduit/)

### LoRa Basic Station

This is the recommended method and more secure, but I couldn't get it to work.

- [TTN Documentation](https://www.thethingsindustries.com/docs/gateways/models/multitechconduit/lbs/)
  - [Creating Keys for CUPS and LNS](https://www.thethingsindustries.com/docs/gateways/concepts/lora-basics-station/cups/)
    - Includes instructions to Configure CUPS to Send the LNS API Key
- [MultiTech Documentation](https://www.multitech.net/developer/software/lora/running-basic-station-on-conduit/)
- [A helpful forum thread](https://www.thethingsnetwork.org/forum/t/unable-to-connect-multitech-conduit-aep-to-ttn-v3/44257)

### UDP Packet Forwarder

This is less secure and not recommeneded.. but it works.

- [TTN Documentation](https://www.thethingsindustries.com/docs/gateways/models/multitechconduit/udp/)

## To Debug

Use powershell to SSH into Conduit:
`ssh LoRaAdmin@192.168.7.72`

Stop active LoRa Network Server so it can be restarted on the SSH for debugging:
`sudo /etc/init.d/lora-network-server stop`

Navigate to the active LoRa Network Server directory:
`cd /var/run/lora/1` if using a packet forwarder or basic station without persistent config
`cd /var/config/lora/1` for a basic station with persistent config enabled

> [NOTE!]
> The station can be running *either* as a basic station  *or* as a packet forwarder, not both.


> [!TIP]
> The `-l0` option sends log output to the terminal

For running as a basic station, start the server:
`sudo ./station -N -l0`

For running as a packet forwarder, start the packet forwarder
`sudo ./lora_pkt_fwd -l0`

Restart the packet forwarder serveice, either with commands or from the browser:

- AEP Version: `/etc/init.d/lora-network-server`
- mLinux Only Version:  `/etc/init.d/lora-packet-forwarder`
