# RecoveryManager

System services recovery manager for embedded Linux systems

## What it does?
The RecoveryManager runs as a system daemon and hooks to systemd service add events to monitor all services available in (or added to) the system. For each service in the system a configuration unit can be defined. When the service terminates with failure and a recovery unit is available, the recovery manager will take the actions in order defined in the configuration file trying to recover the service. If the recovery action is successful  it starts a timer to monitor this unit (relax timer) and considers the service recovered if there is no other failure.

## What's with the running mode?
The recovery manager can run in two modes: primary and client (replica). 
  * Whatever the mode is, the recovery manager instance is responsible to recover the service units in its scope by monitor the systemd instance in the context it is running. Only the system level actions are executed different between modes.
  * The application can also autodetect the running mode, (RunMode = auto) or enforced (RunMode = primary, RunMode = replica)
  * The primary mode is to be used in host system where system reset, container reset and factory reset is possible.
  * The replica mode is to be used in containers to monitor the services running with systemd instance in container. 
  * In replica mode, the recovery manager cannot take system actions required by services in container so it dispatch the actions to the primary instance to execute this actions. For actions affecting only the service state (restart service) the recovery manager will take directly the action in client mode (in container context)
  * A service recovery unit has also the notion of "friend" which can be used to take actions in case a service is crashing in a different context
## How it works?
Here is a brief description of how things works internally:
  * At start the daemon is reading all defined recovery unit from the configuration directory and ingest the unit in SQlite3 persistence database. The units are hashed, so the service recovery unit is added or replace if not existent or different than previous ingested unit for this service.   
  * After reading the units the recovery manager creates a DBus proxy to systemd Manager and listen for new service units. It also reads the existing units and creates DBus proxies for all service units to monitor the service Active state and SubState transitions.   
  * For each recovery unit a recovery vector (rvector) is defined and set initially to zero. The rvector points always to the current recovery action and is incremented on each service failure (direction of the most destructive action). Each recovery action defines an interval where the rvector should be in order to take the action. The rvector can only be reset by the relaxation timer when the service is considered recovered.   
  * When a service is failing and has a recovery unit defined the rvector is incremented by 1 and the service entry in database is queried for the next fitting action. If an action is found is sent to the dispatcher module which will execute the action directly or it will dispatch the action to the primary instance if incapable to take the action directly.
  * After the action is taken, a relaxation timer is started with the current value of the rvector multiplied with the timeout value defined in the recovery unit. When the timer expires if the rvector has the same value as when the timer was started (no other crash during this time) the service is considered recovered and rvector reset to zero.

