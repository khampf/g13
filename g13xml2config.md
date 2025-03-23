% g13xml2config(1) | Logitech G13 command

NAME
====

**g13xml2config** -- Converts Logitech G13 XML profiles into a G13 daemon
configuration file.

SYNOPSYS
========

**g13xml2config** \[**-h**|**--help**] \[**-o** <_FILE_>] \[<_FILE_> ...]


DESCRIPTION
===========

Converts all profiles in every given input <_FILE_> to a single configuration
file for the *g13d* daemon. If no input file is given, profiles are read from
the standard input.

If no output <_FILE_> is specified, the configuration is written to the
standard output.

The generated configuration is "self-contained"; upon loading, it:

- clears all loaded profiles, key bindings and stick zone,
- binds the first four profiles to G13 keys L1 to L4,
- binds each profile states to G13 keys M1 to M3,
- activates the first profile in state 1.

The configuration file can directly be used as the g13d daemon command
line **-c** option or written to the daemon input pipe.

CAVEATS
=======

Profile-specific stick zone bindings are not supported: the last bindings
encountered are globally kept.  
Delays between generated key events are ignored.  
No LCD feedback/menu/color supported.  
No handling of M1-M3 lights.

LICENSE
=======
Public domain.
