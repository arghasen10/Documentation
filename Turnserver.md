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
```bash
sudo apt-get -y update
```

```bash
sudo apt-get install coturn
```

After successful installation stop the service as it will be automatically started once the installation finishes.
```bash
systemctl stop coturn
```

## Enable Coturn
After the installation, we will need to enable the TURN server in the configuration file of coturn. To do this, uncomment the TURNSERVER_ENABLED property in the /etc/default/coturn file.
```bash
nano /etc/default/coturn
```

Be sure that the content of the file has the property uncommented, just like this:
```
TURNSERVER_ENABLED=1
```

Save the changes and proceed with the next step.

## Required Changes in Coturn Configuration file
Now in the configuration file we need to add some changes according to our needs.
```bash
nano /etc/turnserver.conf
```

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

```bash
sudo turnadmin -a -u USERNAME -r REALM -p PASSWORD
```

## Turn coturn server on
After creating the users and configuring coturn properly, we will be able to start the service so clients can connect to it. Proceed with the initialization of the service with the following command:
```bash
systemctl start coturn
```

This will start the service of coturn in the server. We can check the status of the service with the following command:
```bash
systemctl status coturn
```

# Working
In the client javascript of our WebRTC application we can use our turnserver as shown 
```javascript
var pc_config = {
        "iceServers": [
          {
              urls: ["stun:stun.realm:80"]
          },
          {
              username: 'username',
              credential: 'password',
              urls: [
                  "turn:turn.realm:80?transport=udp",
                  "turn:turn.realm:443?transport=tcp",
                  "turns:turn.realm:443?transport=tcp",
              ]
          }
      ]
}
var pc = new RTCPeerConnection(pc_config);
```

# TURN REST API
In WebRTC, the browser obtains the TURN connection information from the web server. This information is a secure information because it contains the necessary TURN credentials. As these credentials are transmitted over the public networks, we have a potential security breach. If we have to transmit a valuable information over the public network, then this information has to have a limited lifetime. Then the guy who obtains this information without permission will be able to perform only limited damage. This is how the idea of TURN REST API - time-limited TURN credentials - appeared. This security mechanism is based upon the long-term credentials mechanism. The main idea of the REST API is that the web server provides the credentials to the client, but those credentials can be used only limited time by an application that has to create a TURN server connection.
In the TURN REST API, there is no persistent passwords for users. A user has just the username. The password is always temporary, and it is generated by the web server on-demand, when the user accesses the WebRTC page. And, actually, a temporary one-time session only, username is provided to the user too.

The temporary password is obtained as HMAC-SHA1 function over the temporary username, with shared secret as the HMAC key, and then the result is encoded:
temporary-password = base64_encode(hmac-sha1(shared-secret, temporary-username))
where temporary-username="timestamp" + ":" + "username" 
For enabling TURN REST API we need to add some changes in the turnserver.conf file
```bash
nano etc/turnserver.conf
```
After opening the turnserver.conf add following changes.
```
use-auth-secret
static-auth-secret=my_secret
```
This will configure coturn in TURN REST API mode. Note we need to share the static-auth-secret with our WebRTC application server.
node.js code for creating TURN credentials in application server:
```javascript
var crypto = require('crypto');

function getTURNCredentials(name, secret){    

    var unixTimeStamp = parseInt(Date.now()/1000) + 24*3600,   // this credential would be valid for the next 24 hours
        username = [unixTimeStamp, name].join(':'),
        password,
        hmac = crypto.createHmac('sha1', secret);
    hmac.setEncoding('base64');
    hmac.write(username);
    hmac.end();
    password = hmac.read();
    return {
        username,
        password
    };
}
```
