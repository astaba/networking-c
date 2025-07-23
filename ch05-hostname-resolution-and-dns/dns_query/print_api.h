// ch05-hostname-resolution-and-dns/dns_query/print_api.h

void print_dns_message(const char *message, int msg_length);

const unsigned char *print_name(const unsigned char *msg,
                                const unsigned char *p,
                                const unsigned char *end);
