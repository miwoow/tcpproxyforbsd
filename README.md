# TcpProxy On FreeBsd

Using kqueue of Freebsd

# Dependment
1. pkg install json-c-0.12.1


# Compile & Install
1. cmake .
1. make

# Protocol
1. Once proxy connect to upstream server. Proxy will send client real ip in uint32\_t format to upstream server in network order.
