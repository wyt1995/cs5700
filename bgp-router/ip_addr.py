# Util functions for manipulating IPv4 addresses.

def binary_addr(ipv4):
    ip_quads = [int(quad) for quad in ipv4.split('.')]
    binary_quads = [bin(num)[2:].zfill(8) for num in ip_quads]
    return ".".join(binary_quads)

def netmask_length(netmask):
    return binary_addr(netmask).count('1')

def cidr_repr(network, netmask):
    mask = netmask_length(netmask)
    return f"{network}/{mask}"

def ip_prefix_int(network, prefix_length):
    ip_num = ip_to_int(network)
    mask = 0xFFFFFFFF << (32 - prefix_length)
    return ip_num & mask

def ip_prefix(network, prefix_length):
    return int_to_ip(ip_prefix_int(network, prefix_length))

def ip_to_int(ipv4):
    ip_quads = [int(quad) for quad in ipv4.split('.')]
    ip_int = 0
    for quad in ip_quads:
        ip_int = (ip_int << 8) + quad
    return ip_int

def int_to_ip(num):
    addr = []
    for i in range(4):
        addr.append(str(num & 0xFF))
        num >>= 8
    return ".".join(addr[::-1])

def prefix_to_netmask(length):
    netmask_int = (0xFFFFFFFF << (32 - length)) & 0xFFFFFFFF
    return int_to_ip(netmask_int)
