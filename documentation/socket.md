# MoltenGamepad Socket Protocol

The socket uses OSC messages to communicate. [OSC has a simple spec](http://opensoundcontrol.org/spec-1_0) and has numerous supporting libraries. MG will only use string, integer, and boolean values in unbundled messages.

An OSC message has a path and a list of arguments.

Every request sent by a client has an argument called the "response id","resp_id","request id". These ids are chosen by the client, and will be echoed back as the first argument of any response message that was related to that request. This allows the responses to be interleaved while still allowing the client to associate each with its request.

The response id 0 is reserved for messages that arise independently of any request.

# Wire Format
In addition to the OSC protocol, MG also needs to know the size of the packet it is about to read. The size in bytes of a OSC message should be written as a 4-byte integer immediately before the packet. Currently the byte order of this integer is host-dependent.

Thus the data stream on the socket should resemble:

    <size>-<packet> <size>-<packet> ...

This same format is used when writing response packets.

# Syntax in this Document

An OSC message will be stylized as

    /the/path  (<arg list>)

where `<arg list>` is a comma separated list of arguments. Each argument will be type followed by a name.

# Client Requests

These are the messages a client might send to MG.

## eval

    /eval (int resp_id, string command)

This runs the provided string as if it was entered directly in MG's standard input. This allows clients to act as a shell for a running instance of MG.

## listen

    /listen (int resp_id, string stream_name, bool enable)

Tells MG that this client wishes to start receiving or stop receiving messages from the specified event stream. Any such events are considered independent of this request and will have a response id of 0.

Currently there are only two streams a client can listen to:

* "slot" : events occur when an input source is added to or removed from an output slot.
* "plug" : events occur when an input source is added or removed.

# Server Responses

These are the messages a client might receive.

## done

    /done (int resp_id)

Indicates that MG is done responding to the given request. A client can stop waiting for a response to the response id.

## text

    /text (int resp_id, string text)

Indicates text for display, like standard output.

## error

    /error (int resp_id, string text, string file_name, int line_number)

Indicates an error message, possibly with a relate file name and line number where the error arose.

## Slot event

    /devslot (int resp_id, string device_name, string slot_name)

This will only be emitted if listening to slot events. It indicates the output slot an input source is moved to. If the slot name is the empty string, this means the device is currently not assigned to any slot.

## Plug event

    /devplug (int resp_id, string device_name, string action)

This will only be emitted if listening to plug events. It indicates the action has just occured for the specified device. The action may be "add" or "remove".
