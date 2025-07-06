// ch01-networks-and-protocols/unix_list.c

#include <ifaddrs.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>

/**
 * @brief List network adapters and their IP addresses
 *
 * The main function retrieves and prints a list of network interfaces
 * available on the system along with their respective IP addresses.
 *
 * It uses the getifaddrs function to obtain a linked list of network
 * interfaces, iterates through the list, and for each interface with
 * an IP address, it prints the interface name, address family (IPv4 or
 * IPv6), and the corresponding IP address in a human-readable form.
 *
 * Returns EXIT_SUCCESS on successful execution, or -1 if getifaddrs
 * fails.
 */
int main(void) {
  struct ifaddrs *addresses; // pointer to a link list of network interfaces
  // Retrieve the current interfaces and store them in "addresses"
  if (getifaddrs(&addresses) == -1) { // Check for errors
    perror("getifaddrs call failed");
    return EXIT_FAILURE;
  }

  struct ifaddrs *address = addresses; // ptr to traverse the addresses list
  // Loop through each address in the linked list
  for (struct ifaddrs *address = addresses; address;
       address = address->ifa_next) {
    int family = address->ifa_addr->sa_family; // Get the address family
    // Check whether the address is IPv4 or IPv6
    if (family == AF_INET || family == AF_INET6) {
      printf("%s\t", address->ifa_name); // Print the interface name
      printf("%s\t", family == AF_INET ? "IPv4" : "IPv6"); // Print IPv4/IPv6
      char ipaddr_str[100]; // Buffer to hold stringified numeric address
      // Determine the size of the address structure
      const int family_size = family == AF_INET ? sizeof(struct sockaddr_in)
                                                : sizeof(struct sockaddr_in6);
      // Convert the address to a human-readable form
      getnameinfo(address->ifa_addr, family_size, ipaddr_str,
                  sizeof(ipaddr_str), 0, 0, NI_NUMERICHOST);
      printf("\t%s\n", ipaddr_str); // Print the readable IP address
    }
  }

  freeifaddrs(addresses); // Free the memory allocated by getifaddrs
  return EXIT_SUCCESS;
}
