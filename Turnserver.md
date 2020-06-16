# Turn Server installation and working 
## Introduction to TURN Server
For a WebRTC based videoconference application other than the signalling server we need a special kind of server that is on charge of relaying the traffic between peers. Peer to Peer connection is not possible unless peers reside on the same local network. Thus we need this special kind of server known as TURN server, that stands for Traversal Using Relay NAT and is a protocol for relaying network traffic. Coturn is a free and open-source implementation of a TURN and STUN server for VoIP and WebRTC. Coturn evolved from [rfc5766-turn-server project](https://code.google.com/p/rfc5766-turn-server/).
## Requirements
In order to succeed with the implementation of turn server, we will need the following things:
- An Ubuntu server
- Know the public IP of our server
- Own a domain and have access to the DNS manager
- SSL Certificates for the domain. Without the secure protocol, our server implementation won't be completed and after using it on our WebRTC projects with HTTPS it won't work.

## Installation of Coturn
> sudo apt-get -y update

> sudo apt-get install coturn

After successful installation stop the service as it will be automatically started once the installation finishes.
> systemctl stop coturn

## Enable Coturn
After the installation, we will need to enable the TURN server in the configuration file of coturn. To do this, uncomment the TURNSERVER_ENABLED property in the /etc/default/coturn file.
> nano /etc/default/coturn

Be sure that the content of the file has the property uncommented, just like this:
>TURNSERVER_ENABLED=1

Save the changes and proceed with the next step.

## Required Changes in Coturn Configuration file
Now in the configuration file we need to add some changes according to our needs.
> nano /etc/turnserver.conf

And paste the following content on it:
```
listening-port=3478
listening-port=80

tls-listening-port=5349
tls-listening-port=443

listening-ip=0.0.0.0
#external ip will be public ip of server
external-ip=x.x.x.x

fingerprint

lt-cred-mech

server-name=domain name
realm=domain name
user=username:password

total-quota=100
stale-nonce=600

cert=/path_to_cert_file/cert.pem
pkey=/path_to_key_file/key.pem

cipher-list="ECDHE-RSA-AES256-GCM-SHA512:DHE-RSA-AES256-GCM-SHA512::DHE-RSA-AES256-GCM-SHA512:ECDHE-RSA-AES256-GCM-SHA384:DHE-RSA-AES256-GCM-SHA384:ECDHE-RSA-AES256-SHA384"

proc-user=turnserver
proc-group=turnserver
```
## Long term user using turnadmin
One can add a turn user in the configuration file or can use [turnadmin](https://github.com/coturn/coturn/wiki/turnadmin) for adding or removing user. Turnadmin comes included in the default package of coturn, this application is a TURN relay administration tool that allows to manage the users of turn/stun server. Clients will need credentials in order to connect to turn server, so clients can be registered like this:

>sudo turnadmin -a -u USERNAME -r REALM -p PASSWORD

## Turn coturn server on
After creating the users and configuring coturn properly, we will be able to start the service so clients can connect to it. Proceed with the initialization of the service with the following command:
> systemctl start coturn

This will start the service of coturn in the server. We can check the status of the service with the following command:
> systemctl status coturn

