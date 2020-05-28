# RPCsvc Module Development Package

This package contains example modules and a makefile for the APx Gen 1
RPC service. This document covers the structure of RPC modules as well as how
to build them and install them on a card. It also includes some [important
points](#important-notes--caveats) to be aware of when using this package.

You can find the development headers on your card in `/usr/include/rpcsvc`
after installing the `rpcsvc-devel` package.

## Considerations for Module Design

1.	Each client connection runs in its own process.

	The implication of this is that it is not possible to use the pthread_mutex
	functions for locking or mutual exclusion.  Use the provided tools in
	`LockTools.h` instead.

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

RPC methods are defined as functions which take a `RPCMsg` request and
`RPCMsg` response as parameters. These provide access to input and output
data for the RPC request that the function services. Methods such as
`get_word` and `set_word` can be used to access the data in these objects.
Please review `wiscRPCMsg.h` () for a list of available data access methods,
or reference `rpctest.cpp` for a full test case.

Errors should be reported as a response body field which is checked for on by
the RPC client.

```cpp
void mread(const RPCMsg *request, RPCMsg *response) {
	uint32_t count = request->get_word("count");
	uint32_t addr = request->get_word("address");
	uint32_t data[count];

	if (easymem_saferead32(memrange, addr, count, data, 1) == 0) {
		response->set_word_array("data", data, count);
	}
	else {
		response->set_string("error", stdsprintf("Easymem error: errno %d", errno));
		LOGGER->log_message(LogManager::INFO, stdsprintf("read easymem error: %d", errno));
	}
}
```

### Logging

The RPC server provides a logging facility to be used in RPC modules, which is
configured to send their output to the standard unix syslog facility.

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

`indicate_activity` requires no parameters and will trigger a flash on the front
panel activity indicator LED.  The color of this flash is determined by the
module parameter `module_activity_color`, and the LED used is determined by the
module parameter `module_led_id`.  See below for more details.

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
	const char *module_version_key = "memory v1.0.2";
	int module_activity_color = 0xff0066;
	int module_led_id = 0;
	void module_init(ModuleManager *modmgr) {
		if (easymem_map_uio(&memrange, "/dev/test_bram", 0, 0x2000, 0) != 0) {
			LOGGER->log_message(LogManager::ERROR, stdsprintf("Unable to map UIO /dev/test_bram: errno %d", errno));
			LOGGER->log_message(LogManager::ERROR, "Unable to load module");
			return; // Do not register our functions, we depend on that mapping.
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

##### module_activity_color & module_led_id

All modules contain a `module_activity_color` int which specifies the color of
LED activity indicator flashes for that module as a standard hex color code.
Any module is free to use the value '0' which will not produce any activity
indication.

*Please coordinate the color value chosen for your module to avoid conflicts.*

All modules contain a `module_led_id` int which specifies which LED activity indicator flashes for that module are sent to.

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

Modules may be built using the Makefile provided in this package.

You will need to perform this operation on a card with `g++`, etc. installed,
as well as the `rpcsvc-devel` package, which contains the module API headers.

Once you have an appropriate environment, simply type `make` and all modules
present in the module development package directory will be compiled.

```sh
sudo yum groupinstall 'Development Tools'
sudo yum install rpcsvc-devel
make
```

### Installing Modules

To install your module on a card, simply compile it and place it in
`/usr/lib/rpcsvc/`. It will automatically be loadable by and available to any
newly opened rpcsvc client connections. Updates are handled in the same
manner. When a client sends a request to load a module for the first time,
the version present on the card at that moment will be used for that session.

## Important Notes & Caveats

1.	You do not need to and should not ever restart the rpcsvc dameon.  Other
	services are also depending on it as well.

	Closing your client connection will result the termination of the subprocess
	serving your client, and you will receive a fresh subprocess upon
	reconnection.  This should handle any situation involving an rpcsvc restart.

2.	The files provided as a part of this package are a part of the rpcsvc
	project and may be updated in the future. Please do *not* make local
	modifications to them, as having divergent sources *will* cause trouble
	in the future. Instead, use a different name for your own modules, or
	submit a pull request at https://github.com/uwcms/APx-rpcsvc to have any
	necessary changes included in the main project. Please review the file
	`RPCSVC_MANIFEST` for details on this package including version
	information and a list of included files that originate from the rpcsvc
	core project.
