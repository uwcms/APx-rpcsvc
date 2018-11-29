#!/bin/sh

if [ ! -e /mnt/persistent/rpcmodules ]; then
	writemount
fi

if [ -n "$PERSIST_IS_RW" ]; then
	mkdir -p /mnt/persistent/rpcmodules
	chmod 01775 /mnt/persistent/rpcmodules
	chown root:1000 /mnt/persistent/rpcmodules
fi

cd /usr/local/rpcmodules.preinstall
for I in *; do
	if [ "$(md5sum "$I" 2>/dev/null | cut -d\  -f1)" != "$(md5sum "/mnt/persistent/rpcmodules/$I" 2>/dev/null | cut -d\  -f1)" ]; then
		writemount
		cp -p "$I" /mnt/persistent/rpcmodules/
	fi
	if [ -n "$PERSIST_IS_RW" ]; then
		chmod 0755 /mnt/persistent/rpcmodules/"$I"
	fi
done
echo Initialized /mnt/persistent/rpcmodules

if [ ! -f /mnt/persistent/config/rpcsvc.acl ]; then
	writemount
	cat >/mnt/persistent/config/rpcsvc.acl <<EOF
# This file contains the list of IP addresses permitted to access rpcsvc.
# Each line contains an IP address in CIDR format.
# Comments begin with # at any point.
# 
# If you aren't familiar with CIDR, use /32 as the prefix length.
#
# Don't remove 127.0.0.1.
127.0.0.1/32 # Local clients
192.168.0.0/16 # Lab private networks
EOF
	echo Initialized /mnt/persistent/config/rpcsvc.acl
fi
if [ -n "$PERSIST_IS_RW" ]; then
	chown 1000:1000 /mnt/persistent/config/rpcsvc.acl
	chmod 0644 /mnt/persistent/config/rpcsvc.acl
fi
