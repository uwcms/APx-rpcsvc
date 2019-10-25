# The Gen 1 University of Wisconsin/APx RPC Service

The APx Gen 1 RPC Service provides a remote procedure call interface,
allowing custom modules to be written to easily expose firmware-specific
functionality to higher level subsystem control or monitoring applications.

The Gen 1 APx RPC Service is derived from the Unversity of Wisconsin RPC Service in use in CTP7s in the CMS Calo Trigger Layer 1 Phase 1 upgrade.

This repository has been reformatted for packaging as a CentOS RPM.

In order to enable this service after installation, you must run:

```sh
systemctl enable rpcsvc # to start rpcsvc on boot
systemctl start rpcsvc  # to start rpcsvc now
```

You will also need to edit `/etc/rpcsvc.ipacl` to define what hosts may connect to the RPC service.  By default, only localhost can connect.
