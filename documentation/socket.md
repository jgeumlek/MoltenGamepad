# OSC Socket Protocol

The `--make-socket` command line argument creates a socket named `moltengamepad.sock` at the location specified by `--socket-path` (default: `$XDG_RUNTIME_DIR`)

The protocol follows the OSC spec, where each OSC packet is prefixed by its length when written to the socket.

The socket allows bidirectional communication with the running instance of moltengamepad.

At the moment, this API amounts to little more than just enabling a CLI session. In the future, it should allow better programmatic control, and the ability to subscribe to events.

## Initializing the protocol version

Every connection must start with the client sending an OSC method `/protocol/version' with a 32 bit integer argument. If the running instance agrees, then it will send back a matching OSC packet. Otherwise, it sends an error packet and closes the connection.

Only the value `1` is valid for the protocol version at this time.

## Running a command

A command can be sent, following the same syntax as STDIN, if sent as a `/eval` OSC method with the command as a string argument.

## Receiving replies

Any plain text output intended for the client is sent back on the socket as a `/text` OSC method with two string arguments. The first argument is a name identifying the source of the message within MoltenGamepad, and the second argument is the text to display.

## Errors

An error is sent as `/error` with a 32 bit integer error code, and string message. 

## Bad Packet Sizes

If the server receives a packet size that does not appear to match the following data, or the size is too large, the connection will be closed.
