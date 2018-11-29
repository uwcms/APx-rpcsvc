# RPCsvc Module Development Package

This package contains everything required to build modules for the CTP7 RPC
service.  This document covers the structure of RPC modules as well as how to
build them and install them on a CTP7.  It also includes some [important
points](#important-notes--caveats) to be aware of when using this package.

## Considerations for Module Design

1.	Each client connection runs in its own process.

	The implication of this is that it is not possible to use the pthread_mutex
	functions for locking or mutual exclusion.  Use the provided tools in
	LockTools.h instead.

2.	The [module_version_key](#module_version_key) described below should be kept
	up to date to prevent confusion or disruption when incompatible changes are
	made or modules are updated on the card.  This allows client software to
	ensure that it will always receive the expected responses from the module.

3.	While a request is being made to your module, the client application calling
	you is generally blocked.  It is generally preferable to split up long slow
	operations such as tests or data aquisition into multiple calls where
	possible, with one call doing setup and another call retrieving collected
	data.

## General Structure of a RPC module

An RPC module consists of any number of RPC methods, and an initialization
function and set of module specific data.

### RPC Methods

RPC methods are defined as functions which take a `RPCMsg` request and `RPCMsg`
response as parameters.  These provide access to input and output data for the
RPC request that the function services.  Methods such as `get_word` and
`set_word` can be used to access the data in these objects.  Please review
`RPCMsg.h` for a list of available data access methods, or reference
`rpctest.cpp` for a full test case.

Errors should be reported as a response body field which is checked for on by
the RPC client.

```cpp
void mread(const RPCMsg *request, RPCMsg *response) {
	uint32_t count = request->get_word("count");
	uint32_t addr = request->get_word("address");
	uint32_t data[count];

	if (memsvc_read(memsvc, addr, count, data) == 0) {
		response->set_word_array("data", data, count);
	}
	else {
		response->set_string("error", memsvc_get_last_error(memsvc));
		LOGGER->log_message(LogManager::INFO, stdsprintf("read memsvc error: %s", memsvc_get_last_error(memsvc)));
	}
}
```

### Logging

The RPC server provides a logging facility to be used in RPC modules, which is
configured to send their output to the standard unix syslog facility.  See
the _Network Configuration_ document for more information on how this log data
is handled.

There is a global LOGGER object accessible to modules to record log messages.
It has two primary functions that are relevant to module developers:
`log_message` and `indicate_activity`.

#### log_message
`log_message` requires a `LogLevel` and a string containing the message to
be logged:

```cpp
LOGGER->log_message(LogManager::DEBUG, message);
```

##### Log Levels

The log levels used by the rpc service are taken from RCC5424 (Syslog).  They
are listed in decreasing order of severity below.

Log Level | Meaning
----------|---------------------------------
EMERGENCY | System is unusable
ALERT     | Action must be taken immediately
CRITICAL  | Critical conditions
ERROR     | Error conditions
WARNING   | Warning conditions
NOTICE    | Normal but significant condition
INFO      | Informational messages
DEBUG     | Debug-level messages

The majority of your meaningful informational messages should be output as
`NOTICE`.

The majority of your normally insignificant informational messages should be
output as `INFO`.

The majority of debug or generally irrelevant messages should be output as
`DEBUG`.

#### indicate_activity

`indicate_activity` requires no parameters and will trigger a flash on the
front panel activity indicator LED.  The color of this flash is determined by
the module parameter `module_activity_color`.  See below for more details.

```cpp
LOGGER->indicate_activity();
```

### Module Specific Data & Initialization

Each module requires a given set of module specific data and an initialization
function which will be called at module load to initialize any necessary
resources and register all provided functions with the RPC service
infrastructure.  Note that this section must be defined as `extern "C"` for
proper linking.

```cpp
extern "C" {
	const char *module_version_key = "memory v1.0.0";
	int module_activity_color = 0;
	void module_init(ModuleManager *modmgr) {
		if (memsvc_open(&memsvc) != 0) {
			LOGGER->log_message(LogManager::ERROR, stdsprintf("Unable to connect to memory service: %s", memsvc_get_last_error(memsvc)));
			LOGGER->log_message(LogManager::ERROR, "Unable to load module");
			return; // Do not register our functions, we depend on memsvc.
		}
		modmgr->register_method("memory", "read", mread);
		modmgr->register_method("memory", "write", mwrite);
	}
}
```

#### Module Specific Data

##### module_version_key

All modules contain a `module_version_key` string, which must be supplied by the
client when the module is loaded.

This value should be unique across all modules and versions.  You should keep
this value up to date as you make changes to a module to prevent confusion in
the event that the wrong module version is loaded on a particular card or client
software expects different behavior from the module.

##### module_activity_color

All modules contain a `module_activity_color` int which determines the color of
LED activity indicator flashes for that module.  Any module is free to use the
value '0' which will not produce any activity indication.

*Please request the assignment of a specific value for your module.*
The valid range of values is 0-8.

#### Initialization Function

All modules contain an initialization function, `void module_init(ModuleManager
*modmgr)`.  This function is called when the module is loaded by the client for
the first time.  It can be used to do any static initialization required, and is
expected to register the RPC methods provided by the module so that they can be
called.

Methods are registered with a call to
```cpp
modmgr->register_module("module_name", "method_name", method_function);
```
and are subsequently made available for clients to execute.

## Installing and Using Modules

### Building Modules

Modules may be built using the Makefile provided in this package.  Note that you
will likely need to update the `PETA_STAGE` variable to point to an appropriate
stage directory containing the relevant headers and libraries.  Once this has
been set up, you should simply be able to run `make` and all modules present in
the module development package directory will be compiled.

### Installing Modules

To install your module on a CTP7, simply compile it and place it in
`/mnt/persistent/rpcmodules`.  It will automatically be loadable by and
available to any newly opened rpcsvc client connections.  Updates are handled in
the same manner.  When a client sends a request to load a module for the first
time, the version present on the card at that moment will be used for that
session.

## Important Notes & Caveats

1.	Do not edit or replace modules provided by the core linux build.  They will
	be reinstalled at card startup, overwriting your changes.  Your own
	non-system modules however will remain on the persistent filesystem as
	normal between reboots.

2.	You do not need to and should not ever restart the rpcsvc dameon.  Other
	services are also depending on it as well.
	
	Closing your client connection will result the termination of the subprocess
	serving your client, and you will receive a fresh subprocess upon
	reconnection.  This should handle any situation involving an rpcsvc restart.

3.	The files provided as a part of this package are a part of the rpcsvc
	project and may be updated in the future.  Please do *not* make local
	modifications to them, as having divergent sources *will* cause trouble in
	the future.  Instead, request that any needed changes be made to the main
	project source.  Please review the file `RPCSVC_MANIFEST` for details on
	this package including version information and a list of included files that
	originate from the rpcsvc core project.
