/*unix_list.c*/

#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>

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
int main(void)
{
  // Declare a pointer to a linked list of network interfaces
  struct ifaddrs *addresses;
  // Retrieve the current interfaces and store them in 'addresses'
  if (getifaddrs(&addresses) == -1) { // Check for errors
    printf("getifaddrs call failed\n");
    return -1;
  }

  // Declare a pointer to traverse the list of addresses
  struct ifaddrs *address = addresses;
  // Loop through each address in the linked list
  while(address) {
    // Get the address family (IPv4 or IPv6)
    int family = address->ifa_addr->sa_family;
    // Check if the address is IPv4 or IPv6
    if (family == AF_INET || family == AF_INET6) {
      // Print the interface name
      printf("%s\t", address->ifa_name);
      // Print whether it is IPv4 or IPv6
      printf("%s\t", family == AF_INET ? "IPv4" : "IPv6");
      // Buffer to hold the numeric address as a string
      char ap[100];
      // Determine the size of the address structure
      const int family_size = family == AF_INET ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6);
      // Convert the address to a human-readable form
      getnameinfo(address->ifa_addr, family_size, ap, sizeof(ap), 0, 0, NI_NUMERICHOST);
      // Print the readable IP address
      printf("\t%s\n", ap);
    }
    // Move to the next interface in the list
    address = address->ifa_next;
  }

  // Free the memory allocated for the linked list
  freeifaddrs(addresses);
  // Return success
  return EXIT_SUCCESS;
}
