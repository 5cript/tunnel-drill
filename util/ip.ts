import ipaddr, { IPv4, IPv6 } from 'ipaddr.js';
import _ from 'lodash';

const compareAddresses = (lhs: ipaddr.IPv4 | ipaddr.IPv6, rhs: ipaddr.IPv4 | ipaddr.IPv6): boolean => {
    if (lhs.kind() === 'ipv6') {
        if ((lhs as ipaddr.IPv6).isIPv4MappedAddress()) {
            return compareAddresses((lhs as ipaddr.IPv6).toIPv4Address(), rhs);
        }
    }
    if (rhs.kind() === 'ipv6') {
        if ((rhs as ipaddr.IPv6).isIPv4MappedAddress()) {
            return compareAddresses(lhs, (rhs as ipaddr.IPv6).toIPv4Address());
        }
    }
    
    if (lhs.kind() != rhs.kind())
        return false;

    if (lhs.kind() === 'ipv4')
    {
        return _.isEqual((lhs as ipaddr.IPv4).octets, (rhs as ipaddr.IPv4).octets);
    }
    else
    {
        return _.isEqual((lhs as ipaddr.IPv6).parts, (rhs as ipaddr.IPv6).parts);
    }
}

const compareAddressStrings = (lhs: string, rhs: string): boolean =>
{
    return compareAddresses(ipaddr.parse(lhs), ipaddr.parse(rhs));
}

export default compareAddressStrings