from ip_addr import *

class TrieNode:
    """
    A node in the binary tree.
    """
    def __init__(self, bit=None, parent=None):
        self.children = [None, None]
        self.route_info = None
        self.alternatives = []
        self.bit = bit
        self.parent = parent


class Trie:
    """
    A trie data structure to implement the forwarding table of a BGP router.
    """
    def __init__(self):
        self.root = TrieNode()

    def insert(self, new_route):
        """
        Insert a new route into the routing table.
        :param new_route: a dictionary containing the information about the new route.
        """
        prefix_length = netmask_length(new_route["netmask"])
        network_int = ip_prefix_int(new_route["network"], prefix_length)

        node = self.root
        for i in range(prefix_length):
            bit = (network_int >> (31 - i)) & 1
            if node.children[bit] is None:
                child = TrieNode(bit, node)
                node.children[bit] = child
            node = node.children[bit]

        # end node, maintain all available routes
        if node.route_info is None:
            node.route_info = new_route
        elif self.compare_routes(new_route, node.route_info):
            node.alternatives.append(node.route_info)
            node.route_info = new_route
        else:
            node.alternatives.append(new_route)

        # attempt to aggregate for the new route
        self.aggregate(node)


    def search(self, network):
        """
        Search for a route to the given destination host.
        :param network: the ip address of the destination host.
        :return: the best available route to the address; None if no route.
        """
        network_int = ip_to_int(network)
        route = None
        node = self.root
        for i in range(32):
            if node.route_info is not None:
                route = node.route_info
            bit = (network_int >> (31 - i)) & 1
            if not node.children[bit]:
                break
            node = node.children[bit]
        return route


    def aggregate(self, node):
        """
        Aggregate forwarding table entries starting from an end node.
        """
        while node and node.parent:
            sibling_bit = 1 - node.bit
            sibling_node = node.parent.children[sibling_bit]
            if (sibling_node and sibling_node.route_info and node.route_info
                and self.can_aggregate(node.route_info, sibling_node.route_info)):
                    node.parent.route_info = self.aggregate_info(node.route_info)
                    node.parent.children = [None, None]
                    node = node.parent
            else:
                break


    @staticmethod
    def can_aggregate(route1, route2):
        """
        Two routes can be aggregated if they are forwarded to the same next-hop router,
        and have the same attributes. Requires the two routes are adjacent numerically.
        """
        return (route1["peer"] == route2["peer"] and route1["ASPath"] == route2["ASPath"]
                and route1["localpref"] == route2["localpref"] and route1["origin"] == route2["origin"]
                and route1["selfOrigin"] == route2["selfOrigin"])

    @staticmethod
    def aggregate_info(route_info):
        """
        :return: the route information for the aggregated parent node.
        """
        parent_info = route_info.copy()
        subnet_mask = netmask_length(route_info["netmask"]) - 1
        parent_info["network"] = ip_prefix(parent_info["network"], subnet_mask)
        parent_info["netmask"] = prefix_to_netmask(subnet_mask)
        return parent_info

    @staticmethod
    def compare_routes(route1, route2):
        """
        Compare two routes to the same destination host.
        :param: dictionaries of route information.
        :return: true if route 1 is preferred, false otherwise.
        """
        # prefer route with higher weight
        if route1["localpref"] != route2["localpref"]:
            return route1["localpref"] > route2["localpref"]
        # prefer self origin
        if route1["selfOrigin"] != route2["selfOrigin"]:
            return route1["selfOrigin"]
        # prefer shorter path
        if len(route1["ASPath"]) != len(route2["ASPath"]):
            return len(route1["ASPath"]) < len(route2["ASPath"])
        # origin preference
        if route1["origin"] != route2["origin"]:
            origin_preference = {'IGP': 3, 'EGP': 2, 'UNK': 1}
            return origin_preference[route1["origin"]] > origin_preference[route2["origin"]]
        return route1["peer"] < route2["peer"]


    def dump(self):
        """
        Check the current state of the forwarding table.
        :return: a list of all routing table entries.
        """
        def collect_routes(node):
            if node.route_info is not None:
                table.append(node.route_info)
                if node.alternatives:
                    table.extend(node.alternatives)
            for child in node.children:
                if child is not None:
                    collect_routes(child)

        table = []
        collect_routes(self.root)
        return table

    def __str__(self):
        return str(self.dump())
