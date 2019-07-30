#!/bin/sh
### BEGIN INIT INFO
# Provides:          rpcsvc
# Required-Start:
# Required-Stop:
# Default-Start:     S
# Default-Stop:
# Short-Description: rpcsvc
### END INIT INFO

#RPCUSER="$(egrep '^[^:]+:[^:]+:1000:' /etc/passwd | cut -d: -f1)"
RPCUSER=root

if [ -z "$RPCUSER" ]; then
	echo No standard mode user created.  Cannot run RPC Service.
else
	if ! [ -e /dev/shm ]; then
		mkdir /dev/shm
		chmod 01777 /dev/shm
	fi
	echo Starting rpcsvc as $RPCUSER
	/bin/mkdir -p /var/log/rpcsvc
	/bin/chown -R $RPCUSER /var/log/rpcsvc
	/bin/su -c /usr/bin/rpcsvc $RPCUSER
fi
