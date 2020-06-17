# Turn Server Database
For the user database, the turnserver has the following options:

1) Users can be set in the command line, with multiple -u or --user options. 

2) Users can be stored in SQLite DB. The default SQLite database file is /var/db/turndb or /usr/local/var/db/turndb or /var/lib/turn/turndb.

3) Users can be stored in MySQL database, if the turnserver was compiled with MySQL support. Each time turnserver checks user credentials, it reads the database (asynchronously,
of course, so that the current flow of packets is not delayed in any way), so any change in the database content is immediately visible by the turnserver. 

4) The same is true for PostgreSQL database. The same considerations are applicable.

5) The same is true for the Redis database, but the Redis database has a different schema.

6) If a database is used, then users can be divided into multiple independent realms. Each realm can be administered separately, and each realm can have its own set of users and its own
performance options (max-bps, user-quota, total-quota).

7) If you use MongoDB, the database will be setup for you automatically.

8) Of course, the turnserver can be used in non-secure mode, when users are allowed to establish sessions anonymously. But in most cases (like WebRTC) that will not work.

This documentation will describe how to install a **MySQL database** in a Linux machine and configure turn server to use that. 

## Install MySQL
Install the MySQL server by using the Ubuntu operating system package manager:
```bash
sudo apt-get update
sudo apt-get install mysql-server
```
The installer installs MySQL and all dependencies.
## Start the MySQL service
After the installation is complete, you can start the database service by running the following command. If the service is already started, a message informs you that the service is already running:
```bash
sudo systemctl start mysql
```
## Launch at reboot
To ensure that the database server launches after a reboot, run the following command:
```bash
sudo systemctl enable mysql
```

## Configure interfaces
MySQL, by default is no longer bound to ( listening on ) any remotely accessible interfaces. Edit the “bind-address” directive in /etc/mysql/mysql.conf.d/mysqld.cnf:

```bash
bind-address		= 127.0.0.1 ( The default. )
bind-address		= XXX.XXX.XXX.XXX ( The ip address of your Public Net interface. )
bind-address		= ZZZ.ZZZ.ZZZ.ZZZ ( The ip address of your Service Net interface. )
bind-address		= 0.0.0.0 ( All ip addresses. )
```
Restart the mysql service.
```bash
sudo systemctl restart mysql
```
## Add a User and create Database
Start the mysql shell with the following command after installation
```bash
sudo mysql
```
The following mysql shell prompt should appear:
```bash
mysql>
```
Add 'turn' user with 'turn' password (for example):
```bash
> create user 'turn'@'localhost' identified by 'turn';
```
  
Create database 'turn' (for example) and grant privileges to user 'turn':
```bash
  > create database turn;
  > grant all on turn.* to 'turn'@'localhost';
  > flush privileges;
  Ctrl-D
```
Create database schema:
```bash
  $ mysql -p -u turn turn <schema.sql
  Enter password: turn
  $
```  
## Fill in users

- Shared secret for the TURN REST API:
```bash
  $ bin/turnadmin -s logen -r v.abhijitmondal.in -M "host=localhost dbname=turn user=turn password=turn"
```
  
- Long-term credentials mechanism:
```
  $ bin/turnadmin -a -M "host=localhost dbname=turn user=turn password=turn" -u gorst -r v.abhijitmondal.in -p hero
  $ bin/turnadmin -a -M "host=localhost dbname=turn user=turn password=turn" -u ninefingers -r v.abhijitmondal.in -p youhavetoberealistic
``` 
 -  Admin users:
```   
  $ bin/turnadmin -A -M "host=localhost dbname=turn user=turn password=turn" -u gorst -p hero
  $ bin/turnadmin -A -M "host=localhost dbname=turn user=turn password=turn" -u ninefingers -p youhavetoberealistic -r north.gov
```
## Configure TURN Server to use MySQL Database
We can use the TURN server database parameter --mysql-userdb. The value of this parameter is a connection string for the MySQL database. As "native" MySQL does not 
have such a feature as "connection string", the TURN server parses the connection string and converts it into MySQL database connection parameter. 

The format of the MySQL connection string is:
```bash
"host=<host> dbname=<database-name> user=<database-user> password=<database-user-password> port=<port> connect_timeout=<seconds>"
```

(all parameters are optional)

So, an example of the MySQL database parameter in the TURN server command 
line would be:
```bash
--mysql-userdb="host=localhost dbname=turn user=turn password=turn connect_timeout=30"
```
Or in the turnserver.conf file:
```bash
mysql-userdb="host=localhost dbname=turn user=turn password=turn connect_timeout=30"
```
