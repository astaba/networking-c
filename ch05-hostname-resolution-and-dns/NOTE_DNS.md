# DNS Message

## Some free public DNS Servers

| DNS Provider | IPv4 Addresses | IPv6 Addresses|
|--------------|----------------|---------------|
| **Cloudflare 1.1.1.1** | `1.1.1.1` | `2606:4700:4700::1111` |
| | `1.0.0.1` | `2606:4700:4700::1001` |
| **FreeDNS** | `37.235.1.174` | |
| | `37.235.1.177` | |
| **Google Public DNS** | `8.8.8.8` | `2001:4860:4860::8888` |
| | `8.8.4.4` | `2001:4860:4860::8844` |
| **OpenDNS** | `208.67.222.222` | `2620:0:ccc::2` |
| | `208.67.220.220` | `2620:0:ccd::2` |

## DNS Protocol

**WARNING:** OS little-endian vs DNS Protocol big-endian.

**DNS Protocol specifies high-order byte is sent first.**

As an example: When written to memory the 2-byte size decimal integer `999` convert to `0x03E7` and is subject to OS little-endianness rule specifying least significant bytes occupy lowest memory addresses. As a result `999` is commit to memory as `"memory_cells" = { [0]="E7", [1]="03" }`. And send in the same order `[E7, 03]` accros the network by function like `send()` and `recv()`.

However you should think of the DNS Protocol as a human asking for a number he needs to write down. When you ask the year 2025 you do not expect to hear 5, 2, 0 and 2 as little-endianness demands. DNS Protocol expects to be told 2025 before it writes 2, 0, 5 and then 2. Therefore if you were to send `999` through DNS query you are required to send `[03, E7]`.

That is the purpose of bytes swapping functions like `htons()` and `htonl()` (host to network short/long). They read `999` fro memory as `[E7, 03]` swap it to big-endian and deliver it as `[03, E7]`.

## DNS Message Header format

---

### DNS Message Header Section

```code
  ┎───────────────────────────────────────────────────────────────────────────────┒
  ┃                       12-byte Header Format (16 bits * 6)                     ┃
  ┠────┰────┰────┰────┰────┰────┰────┰────┰────┰────┰────┰────┰────┰────┰────┰────┨
  ┃ 00 ┃ 01 ┃ 02 ┃ 03 ┃ 04 ┃ 05 ┃ 06 ┃ 07 ┃ 08 ┃ 09 ┃ 10 ┃ 11 ┃ 12 ┃ 13 ┃ 14 ┃ 15 ┃
  ┖────┸────┸────┸────┸────┸────┸────┸────┸────┸────┸────┸────┸────┸────┸────┸────┚
                            Header Section of a DNS Message
  ┎───────────────────────────────────────────────────────────────────────────────┒
  ┃                                       ID                                      ┃
  ┠────┰───────────────────┰────┰────┰────┰────┰──────────────┰───────────────────┨
  ┃ QR ┃       OpCode      ┃ AA ┃ TC ┃ RD ┃ RA ┃       Z      ┃       RCODE       ┃
  ┠────┸───────────────────┸────┸────┸────┸────┸──────────────┸───────────────────┨
  ┃                                    QDCOUNT                                    ┃
  ┠───────────────────────────────────────────────────────────────────────────────┨
  ┃                                    ANCOUNT                                    ┃
  ┠───────────────────────────────────────────────────────────────────────────────┨
  ┃                                    NSCOUNT                                    ┃
  ┠───────────────────────────────────────────────────────────────────────────────┨
  ┃                                    ARCOUNT                                    ┃
  ┖───────────────────────────────────────────────────────────────────────────────┚
```

This table illustrates the standard **DNS Message Header format** as defined in RFC 1035. This 12-byte header is the first part of every DNS message (query or response) and is transmitted in **network byte order (big-endian)**.

Here's a brief explanation of each field:

1.  **ID (00-15 bits / First 2 bytes):**
    * A 16-bit identifier assigned by the program that generates any kind of query. This ID is copied into the corresponding response, allowing the client to match replies to outstanding queries.

2.  **Flags (16-31 bits / Second 2 bytes):**
    * A critical 16-bit field containing various control bits for the DNS operation:
        * **QR (0):** Query (0) or Response (1).
        * **OpCode (1-4):** Kind of query (0 for standard query, 1 for inverse query, 2 for server status request).
        * **AA (5):** Authoritative Answer. Set to 1 if the responding name server is an authority for the domain name in question.
        * **TC (6):** TrunCation. Set to 1 if the message was truncated due to length exceeding the transport's limit.
        * **RD (7):** Recursion Desired. Set to 1 if the querier wants the name server to recursively pursue the query.
        * **RA (8):** Recursion Available. Set to 1 if the name server supports recursive queries.
        * **Z (9-11):** Reserved for future use; must be zero.
        * **RCODE (12-15):** Response Code. Indicates the status of the query (e.g., 0 for No Error, 3 for Name Error).

3.  **QDCOUNT (32-47 bits / Third 2 bytes):**
    * A 16-bit unsigned integer specifying the number of entries in the Question section.

4.  **ANCOUNT (48-63 bits / Fourth 2 bytes):**
    * A 16-bit unsigned integer specifying the number of resource records in the Answer section.

5.  **NSCOUNT (64-79 bits / Fifth 2 bytes):**
    * A 16-bit unsigned integer specifying the number of name server resource records in the Authority section.

6.  **ARCOUNT (80-95 bits / Sixth 2 bytes):**
    * A 16-bit unsigned integer specifying the number of resource records in the Additional records section.

---

### Complete DNS Query Message

```code
  ┎───────────────────────────────────────────────────────────────────────────────┒
  ┃                                   bit count                                   ┃
  ┠────┰────┰────┰────┰────┰────┰────┰────┰────┰────┰────┰────┰────┰────┰────┰────┨
  ┃ 00 ┃ 01 ┃ 02 ┃ 03 ┃ 04 ┃ 05 ┃ 06 ┃ 07 ┃ 08 ┃ 09 ┃ 10 ┃ 11 ┃ 12 ┃ 13 ┃ 14 ┃ 15 ┃
  ┖────┸────┸────┸────┸────┸────┸────┸────┸────┸────┸────┸────┸────┸────┸────┸────┚


                                DNS QUERY MESSAGE
┎───────────────────────────────────────────────────────────────────────────────────┒
┃ ┎───────────────────────────────────────────────────────────────────────────────┒ ┃
┃ ┇                                HEADER                                         ┇ ┃
┃ ┇             12 bytes with its 13 sub-fields and bit flags                     ┇ ┃
┃ ┇                                                                               ┇ ┃
┃ ┖───────────────────────────────────────────────────────────────────────────────┚ ┃
┃  Question                                                                         ┃
┃ ┎───────────────────────────────────────────────────────────────────────────────┒ ┃
┃ ┇                                                                               ┇ ┃
┃ ┇                                    QNAME                                      ┇ ┃
┃ ┇                               (variable size)                                 ┇ ┃
┃ ┠───────────────────────────────────────────────────────────────────────────────┨ ┃
┃ ┃                                QTYPE (2 bytes)                                ┃ ┃
┃ ┠───────────────────────────────────────────────────────────────────────────────┨ ┃
┃ ┃                                QCLASS (2 bytes)                               ┃ ┃
┃ ┖───────────────────────────────────────────────────────────────────────────────┚ ┃
┃                                                                                   ┃
┃  ┇  Other whole question sections which each one QNAME, QTYPE and QCLASS are    ┇ ┃
┃  ┇  theorytically repeatable header.QDCOUNT times but rarely done               ┇ ┃
┃  ┇                                                                              ┇ ┃
┖───────────────────────────────────────────────────────────────────────────────────┚

```

---

### Complete DNS Response Message

```code
  ┎───────────────────────────────────────────────────────────────────────────────┒
  ┃                                   bit count                                   ┃
  ┠────┰────┰────┰────┰────┰────┰────┰────┰────┰────┰────┰────┰────┰────┰────┰────┨
  ┃ 00 ┃ 01 ┃ 02 ┃ 03 ┃ 04 ┃ 05 ┃ 06 ┃ 07 ┃ 08 ┃ 09 ┃ 10 ┃ 11 ┃ 12 ┃ 13 ┃ 14 ┃ 15 ┃
  ┖────┸────┸────┸────┸────┸────┸────┸────┸────┸────┸────┸────┸────┸────┸────┸────┚


                                DNS RESPONSE MESSAGE
┎───────────────────────────────────────────────────────────────────────────────────┒
┃ ┎───────────────────────────────────────────────────────────────────────────────┒ ┃
┃ ┇                                HEADER                                         ┇ ┃
┃ ┇             12 bytes with its 13 sub-fields and bit flags                     ┇ ┃
┃ ┇                                                                               ┇ ┃
┃ ┖───────────────────────────────────────────────────────────────────────────────┚ ┃
┃  Question                                                                         ┃
┃ ┎───────────────────────────────────────────────────────────────────────────────┒ ┃
┃ ┇                                                                               ┇ ┃
┃ ┇                                    QNAME                                      ┇ ┃
┃ ┇                               (variable size)                                 ┇ ┃
┃ ┠───────────────────────────────────────────────────────────────────────────────┨ ┃
┃ ┃                               QTYPE (2 bytes)                                 ┃ ┃
┃ ┠───────────────────────────────────────────────────────────────────────────────┨ ┃
┃ ┃                               QCLASS (2 bytes)                                ┃ ┃
┃ ┖───────────────────────────────────────────────────────────────────────────────┚ ┃
┃                                                                                   ┃
┃  ┇           As many question sections as in the incoming DNS Query             ┇ ┃
┃                                                                                   ┃
┃  A Resource Record either: Answer, Authority or Additional                        ┃
┃ ┎───────────────────────────────────────────────────────────────────────────────┒ ┃
┃ ┇                                                                               ┇ ┃
┃ ┇                                    NAME                                       ┇ ┃
┃ ┇                               (variable size)                                 ┇ ┃
┃ ┠───────────────────────────────────────────────────────────────────────────────┨ ┃
┃ ┃                                TYPE (2 bytes)                                 ┃ ┃
┃ ┠───────────────────────────────────────────────────────────────────────────────┨ ┃
┃ ┃                                CLASS (2 bytes)                                ┃ ┃
┃ ┠───────────────────────────────────────────────────────────────────────────────┨ ┃
┃ ┃                                 TTL (4 bytes)                                 ┃ ┃
┃ ┃                                                                               ┃ ┃
┃ ┠───────────────────────────────────────────────────────────────────────────────┨ ┃
┃ ┃                               RDLENGTH (2 bytes)                              ┃ ┃
┃ ┠───────────────────────────────────────────────────────────────────────────────┨ ┃
┃ ┇                                                                               ┇ ┃
┃ ┇                                   RDATA                                       ┇ ┃
┃ ┇                  (as many bytes as indicated by RDLENGTH)                     ┇ ┃
┃ ┇                                                                               ┇ ┃
┃ ┖───────────────────────────────────────────────────────────────────────────────┚ ┃
┃                                                                                   ┃
┃  ┇              RR sections are repeated as many times as:                      ┇ ┃
┃  ┇              header.ANCOUNT + header.NSCOUNT + header.ARCOUNT                ┇ ┃
┃                                                                                   ┃
┖───────────────────────────────────────────────────────────────────────────────────┚

```
-----

## DNS Message Format Summaries (HTML Tables)

Here are the HTML tables, formatted to be embedded directly into a Markdown file.

### DNS Header Format

<table style="width:100%; border-collapse: collapse; text-align: left;">
<thead>
<tr style="background-color:#f2f2f2;">
<th style="padding: 8px; border: 1px solid #ddd;">Field</th>
<th style="padding: 8px; border: 1px solid #ddd;">Bits / Bytes</th>
<th style="padding: 8px; border: 1px solid #ddd;">Description</th>
</tr>
</thead>
<tbody>
<tr>
<td style="padding: 8px; border: 1px solid #ddd;"><strong>ID</strong></td>
<td style="padding: 8px; border: 1px solid #ddd;">16 bits (2 bytes)</td>
<td style="padding: 8px; border: 1px solid #ddd;">A unique identifier for the query. Copied into the response.</td>
</tr>
<tr>
<td style="padding: 8px; border: 1px solid #ddd;"><strong>Flags</strong><br>(see detail below)</td>
<td style="padding: 8px; border: 1px solid #ddd;">16 bits (2 bytes)</td>
<td style="padding: 8px; border: 1px solid #ddd;">Contains various control bits for the DNS operation.</td>
</tr>
<tr style="background-color:#e6e6e6;">
<td colspan="3" style="padding: 8px; border: 1px solid #ddd; font-weight: bold;">Flags Breakdown:</td>
</tr>
<tr>
<td style="padding: 8px; border: 1px solid #ddd;">&nbsp;&nbsp;&nbsp;&nbsp;QR</td>
<td style="padding: 8px; border: 1px solid #ddd;">1 bit</td>
<td style="padding: 8px; border: 1px solid #ddd;">Query (0) or Response (1).</td>
</tr>
<tr>
<td style="padding: 8px; border: 1px solid #ddd;">&nbsp;&nbsp;&nbsp;&nbsp;OpCode</td>
<td style="padding: 8px; border: 1px solid #ddd;">4 bits</td>
<td style="padding: 8px; border: 1px solid #ddd;">Type of query (0 for standard query).</td>
</tr>
<tr>
<td style="padding: 8px; border: 1px solid #ddd;">&nbsp;&nbsp;&nbsp;&nbsp;AA</td>
<td style="padding: 8px; border: 1px solid #ddd;">1 bit</td>
<td style="padding: 8px; border: 1px solid #ddd;">Authoritative Answer (1 if server is authority).</td>
</tr>
<tr>
<td style="padding: 8px; border: 1px solid #ddd;">&nbsp;&nbsp;&nbsp;&nbsp;TC</td>
<td style="padding: 8px; border: 1px solid #ddd;">1 bit</td>
<td style="padding: 8px; border: 1px solid #ddd;">TrunCation (1 if message was truncated).</td>
</tr>
<tr>
<td style="padding: 8px; border: 1px solid #ddd;">&nbsp;&nbsp;&nbsp;&nbsp;RD</td>
<td style="padding: 8px; border: 1px solid #ddd;">1 bit</td>
<td style="padding: 8px; border: 1px solid #ddd;">Recursion Desired (1 if recursive query is requested).</td>
</tr>
<tr>
<td style="padding: 8px; border: 1px solid #ddd;">&nbsp;&nbsp;&nbsp;&nbsp;RA</td>
<td style="padding: 8px; border: 1px solid #ddd;">1 bit</td>
<td style="padding: 8px; border: 1px solid #ddd;">Recursion Available (1 if server supports recursion).</td>
</tr>
<tr>
<td style="padding: 8px; border: 1px solid #ddd;">&nbsp;&nbsp;&nbsp;&nbsp;Z</td>
<td style="padding: 8px; border: 1px solid #ddd;">3 bits</td>
<td style="padding: 8px; border: 1px solid #ddd;">Reserved for future use; must be zero.</td>
</tr>
<tr>
<td style="padding: 8px; border: 1px solid #ddd;">&nbsp;&nbsp;&nbsp;&nbsp;RCODE</td>
<td style="padding: 8px; border: 1px solid #ddd;">4 bits</td>
<td style="padding: 8px; border: 1px solid #ddd;">Response Code (e.g., 0=No Error, 3=Name Error).</td>
</tr>
<tr>
<td style="padding: 8px; border: 1px solid #ddd;"><strong>QDCOUNT</strong></td>
<td style="padding: 8px; border: 1px solid #ddd;">16 bits (2 bytes)</td>
<td style="padding: 8px; border: 1px solid #ddd;">Number of entries in the Question section.</td>
</tr>
<tr>
<td style="padding: 8px; border: 1px solid #ddd;"><strong>ANCOUNT</strong></td>
<td style="padding: 8px; border: 1px solid #ddd;">16 bits (2 bytes)</td>
<td style="padding: 8px; border: 1px solid #ddd;">Number of resource records in the Answer section.</td>
</tr>
<tr>
<td style="padding: 8px; border: 1px solid #ddd;"><strong>NSCOUNT</strong></td>
<td style="padding: 8px; border: 1px solid #ddd;">16 bits (2 bytes)</td>
<td style="padding: 8px; border: 1px solid #ddd;">Number of name server resource records in the Authority section.</td>
</tr>
<tr>
<td style="padding: 8px; border: 1px solid #ddd;"><strong>ARCOUNT</strong></td>
<td style="padding: 8px; border: 1px solid #ddd;">16 bits (2 bytes)</td>
<td style="padding: 8px; border: 1px solid #ddd;">Number of resource records in the Additional records section.</td>
</tr>
</tbody>
</table>

### DNS Query Message Format

<table style="width:100%; border-collapse: collapse; text-align: left;">
<thead>
<tr style="background-color:#f2f2f2;">
<th style="padding: 8px; border: 1px solid #ddd;">Section</th>
<th style="padding: 8px; border: 1px solid #ddd;">Field</th>
<th style="padding: 8px; border: 1px solid #ddd;">Bytes</th>
<th style="padding: 8px; border: 1px solid #ddd;">Description</th>
</tr>
</thead>
<tbody>
<tr>
<td style="padding: 8px; border: 1px solid #ddd;" rowspan="1"><strong>Header</strong></td>
<td style="padding: 8px; border: 1px solid #ddd;" colspan="3">A 12-byte fixed header (as described above). QDCOUNT will typically be 1, others 0. QR flag is 0.</td>
</tr>
<tr>
<td style="padding: 8px; border: 1px solid #ddd;" rowspan="3"><strong>Question</strong><br>(1 or more entries)</td>
<td style="padding: 8px; border: 1px solid #ddd;"><strong>QNAME</strong></td>
<td style="padding: 8px; border: 1px solid #ddd;">Variable</td>
<td style="padding: 8px; border: 1px solid #ddd;">The domain name being queried, represented as a sequence of length-prefixed labels ending with a null byte (e.g., `3www6google3com0`). No compression pointers in queries.</td>
</tr>
<tr>
<td style="padding: 8px; border: 1px solid #ddd;"><strong>QTYPE</strong></td>
<td style="padding: 8px; border: 1px solid #ddd;">2</td>
<td style="padding: 8px; border: 1px solid #ddd;">The type of resource record requested (e.g., 1 for A record, 28 for AAAA).</td>
</tr>
<tr>
<td style="padding: 8px; border: 1px solid #ddd;"><strong>QCLASS</strong></td>
<td style="padding: 8px; border: 1px solid #ddd;">2</td>
<td style="padding: 8px; border: 1px solid #ddd;">The class of the query (e.g., 1 for IN - Internet).</td>
</tr>
</tbody>
</table>

### DNS Response Message Format

<table style="width:100%; border-collapse: collapse; text-align: left;">
<thead>
<tr style="background-color:#f2f2f2;">
<th style="padding: 8px; border: 1px solid #ddd;">Section</th>
<th style="padding: 8px; border: 1px solid #ddd;">Field</th>
<th style="padding: 8px; border: 1px solid #ddd;">Bytes</th>
<th style="padding: 8px; border: 1px solid #ddd;">Description</th>
</tr>
</thead>
<tbody>
<tr>
<td style="padding: 8px; border: 1px solid #ddd;" rowspan="1"><strong>Header</strong></td>
<td style="padding: 8px; border: 1px solid #ddd;" colspan="3">A 12-byte fixed header (as described above). QR flag is 1. QDCOUNT, ANCOUNT, NSCOUNT, ARCOUNT indicate the number of records in subsequent sections.</td>
</tr>
<tr>
<td style="padding: 8px; border: 1px solid #ddd;" rowspan="3"><strong>Question</strong><br>(0 or more entries)</td>
<td style="padding: 8px; border: 1px solid #ddd;"><strong>QNAME</strong></td>
<td style="padding: 8px; border: 1px solid #ddd;">Variable</td>
<td style="padding: 8px; border: 1px solid #ddd;">Typically an exact copy of the QNAME from the original query. Can use compression pointers.</td>
</tr>
<tr>
<td style="padding: 8px; border: 1px solid #ddd;"><strong>QTYPE</strong></td>
<td style="padding: 8px; border: 1px solid #ddd;">2</td>
<td style="padding: 8px; border: 1px solid #ddd;">Type of the original query.</td>
</tr>
<tr>
<td style="padding: 8px; border: 1px solid #ddd;"><strong>QCLASS</strong></td>
<td style="padding: 8px; border: 1px solid #ddd;">2</td>
<td style="padding: 8px; border: 1px solid #ddd;">Class of the original query.</td>
</tr>
<tr>
<td style="padding: 8px; border: 1px solid #ddd;" rowspan="6"><strong>Answer / Authority / Additional</strong><br>(0 or more Resource Records (RRs))</td>
<td style="padding: 8px; border: 1px solid #ddd;"><strong>NAME</strong></td>
<td style="padding: 8px; border: 1px solid #ddd;">Variable</td>
<td style="padding: 8px; border: 1px solid #ddd;">The domain name to which this resource record pertains. Can be a full name or, commonly, a <strong style="color: blue;">compression pointer</strong> (e.g., `0xC0XX`) to an earlier occurrence of the name in the message.</td>
</tr>
<tr>
<td style="padding: 8px; border: 1px solid #ddd;"><strong>TYPE</strong></td>
<td style="padding: 8px; border: 1px solid #ddd;">2</td>
<td style="padding: 8px; border: 1px solid #ddd;">The type of resource data (e.g., 1 for A, 28 for AAAA, 5 for CNAME, 15 for MX).</td>
</tr>
<tr>
<td style="padding: 8px; border: 1px solid #ddd;"><strong>CLASS</strong></td>
<td style="padding: 8px; border: 1px solid #ddd;">2</td>
<td style="padding: 8px; border: 1px solid #ddd;">The class of the data (e.g., 1 for IN - Internet).</td>
</tr>
<tr>
<td style="padding: 8px; border: 1px solid #ddd;"><strong>TTL</strong></td>
<td style="padding: 8px; border: 1px solid #ddd;">4</td>
<td style="padding: 8px; border: 1px solid #ddd;">Time To Live in seconds. How long the record can be cached.</td>
</tr>
<tr>
<td style="padding: 8px; border: 1px solid #ddd;"><strong>RDLENGTH</strong></td>
<td style="padding: 8px; border: 1px solid #ddd;">2</td>
<td style="padding: 8px; border: 1px solid #ddd;">The length of the RDATA field in bytes.</td>
</tr>
<tr>
<td style="padding: 8px; border: 1px solid #ddd;"><strong>RDATA</strong></td>
<td style="padding: 8px; border: 1px solid #ddd;">Variable</td>
<td style="padding: 8px; border: 1px solid #ddd;">The actual resource data. Its format depends on the TYPE field (e.g., 4 bytes for an IPv4 address if TYPE is A, another name if TYPE is CNAME or MX).</td>
</tr>
</tbody>
</table>


-----

### DNS Message Header Format

The DNS message always starts with a fixed 12-byte header. This header contains essential control information for the entire message.

```lua
-- Lua pseudocode to foster a mental model of the DNS Message Header Format

local header = { -- (6 x 16-bit == 12 bytes)
    -- Transaction ID: identifier assigned by the query originator.
    -- It is copied into the response to match queries with responses.
    id = 0x0000, -- (16-bit) e.g., 0x1234 for an example

    -- Flags: A 16-bit field containing various control bits.
    -- Each flag is typically represented by a single bit or a few bits.
    flags = {       -- ( bit flags adding up to 2 bytes)
        qr = 0,     -- (1-bit) Query (0) or Response (1)
        opcode = 0, -- (4-bit) Standard Query (0), Inverse Query (1), Status (2), etc.
        aa = 0,     -- (1-bit) Authoritative Answer (1 if server is authority for the domain)
        tc = 0,     -- (1-bit) TrunCation (1 if message was truncated due to length limits)
        rd = 0,     -- (1-bit) Recursion Desired (1 if recursive query is requested)
        ra = 0,     -- (1-bit) Recursion Available (1 if server supports recursion)
        z = 0,      -- (3-bit) Reserved for future use (must be zero)
        rcode = 0   -- (4-bit) Response Code (0=No Error, 1=Format Error, 2=Server Failure, 3=Name Error, etc.)
    },

    -- Question Count: Number of entries in the Question section.
    qdcount = 0, -- (16-bit)

    -- Answer Count: Number of resource records in the Answer section.
    ancount = 0, -- (16-bit)

    -- Name Server Count: Number of name server resource records in the Authority section.
    nscount = 0, -- (16-bit)

    -- Additional Records Count: Number of resource records in the Additional records section.
    arcount = 0  -- (16-bit)
}
```

  * **`id`**: A 16-bit transaction identifier. This ID is set by the client in a query and copied unchanged into the server's response. It helps the client match a response to its corresponding query.
  * **`flags`**: A 16-bit field containing various flags that control the behavior and indicate the status of the DNS message.
      * **`qr` (Query/Response)**: 0 for a query, 1 for a response.
      * **`opcode`**: A 4-bit field specifying the type of query. Standard queries are `0`.
      * **`aa` (Authoritative Answer)**: Set to 1 in a response if the responding name server is an authority for the domain name in question.
      * **`tc` (TrunCation)**: Set to 1 if the message was truncated due to length limits (e.g., if a UDP response exceeds 512 bytes).
      * **`rd` (Recursion Desired)**: Set to 1 in a query if the client wants the name server to perform a recursive query (i.e., resolve the name completely, even if it needs to contact other servers).
      * **`ra` (Recursion Available)**: Set to 1 in a response if the name server supports recursive queries.
      * **`z`**: A 3-bit field reserved for future use; always set to 0.
      * **`rcode` (Response Code)**: A 4-bit field indicating the status of the query. `0` means No Error. Other values indicate various error conditions.
  * **`qdcount` (Question Count)**: A 16-bit unsigned integer specifying the number of entries in the Question section.
  * **`ancount` (Answer Count)**: A 16-bit unsigned integer specifying the number of resource records in the Answer section.
  * **`nscount` (Name Server Count)**: A 16-bit unsigned integer specifying the number of name server resource records in the Authority section.
  * **`arcount` (Additional Records Count)**: A 16-bit unsigned integer specifying the number of resource records in the Additional records section.

-----

### For the Query:

```lua
-- Lua pseudocode to foster a mental model of the DNS Query Format

local query = {
    header = {...}, -- (12-byte)

    -- The 'question' field is theoretically repeatable header.qdcount times,
    -- but in practice, almost all DNS clients send queries with only 1 question.
    question = {    -- (variable bytes)
        -- The domain name being queried, represented as a sequence of
        -- length-prefixed labels ending with a null byte (e.g., `3www6google3com0`).
        -- No compression pointers in queries.
        "qname",    -- (variable bytes)

        -- e.g. type:
        -- 1 for A (IPv4 address record),
        -- 28 for AAAA (IPv6 address record),
        -- 15 for MX (mail exchange record),
        -- 16 for TXT (Text record),
        -- 5 for CNAME (canonical name),
        -- 255 for * (any).
        "qtype",    -- (2-byte) type of the original query

        -- The class of the data (e.g., 1 for IN - Internet).
        "qclass"    -- (2-byte) class of the original query
    }
}
```

  * **`header = {...}`**: Correct. This refers to the fixed 12-byte header structure described above.
  * **`question = { "qname", "qtype", "qclass" }`**: Correct. This represents a single question entry.
  * **"The 'question' field is theoretically repeatable... but in practice, almost all DNS clients send queries with only 1 question."**: This is a very insightful observation. While the DNS specification *allows* for `QDCOUNT` (Question Count in the header) to be greater than 1 (meaning multiple questions in a single query), in practice, almost all DNS clients send queries with only **one** question. It's theoretically possible, but rarely implemented.

-----

### For the Response:

```lua
-- Lua pseudocode to foster a mental model of the DNS Response Format

local response = {
    header = {...}, -- (12-byte)

    -- follows the same specification as in the query
    question = { "qname", "qtype", "qclass"}, -- (variable bytes)

    -- The "Resource Record" field acts as a template for entries in the
    -- Answer, Authority, and Additional sections repeatable n times:
    -- n = header.ancount + header.nscount + header.arcount
    ["RR"] = {      -- (variable bytes)
        -- The domain name to which this resource record pertains.
        -- Can be a full name or, commonly, a compression pointer (e.g., `0xC0XX`)
        -- to an earlier occurrence of the name in the message.
        "name",     -- (variable bytes)

        -- The type of resource data
        "type",     -- (2-byte)

        -- The class of the data.
        "class",    -- (2-byte)

        -- Time To Live in seconds. How long the record can be cached.
        "ttl",      -- (4-byte)

        -- The length of the RDATA field in bytes.
        "rdlength", -- (2-byte)

        -- The actual resource data. Its format depends on the TYPE field
        -- (e.g., 4 bytes for an IPv4 address if TYPE is A, another name if TYPE is CNAME or MX).
        "rdata"     -- (as many bytes as the integer value of rdlength)
    }
}
```

  * **`header = {...}`**: Correct. The response header will match the query ID, set the QR flag to 1, and contain the counts for each section.
  * **`question = { "qname", "qtype", "qclass" }`**: Correct. A response typically echoes the original query's question section(s).
  * **`["RR"] = { "name", "type", "class", "ttl", "rdlength", "rdata" }`**: This is an excellent way to abstract the Resource Record.
      * **"The 'Resource Record' field acts as a template for entries in the Answer, Authority, and Additional sections."**: **Absolutely correct\!** All three of these sections (Answer, Authority, Additional) are composed of Resource Records (RRs), and each RR follows the `name, type, class, ttl, rdlength, rdata` format. The `print_dns_message` function correctly iterates through `ancount + nscount + arcount` because they all share this common structure.
      * **"These sections are repeatable... Answer: header.ancount times, Authority: header.nscount times, Additional: header.arcount times"**: **Yes, precisely\!** The `ANCOUNT`, `NSCOUNT`, and `ARCOUNT` fields in the header explicitly tell you how many such `RR` structures to expect in each respective section. Each of these counts can be zero or more.

-----

## recvfrom() and UDP

In the `main` function, the `recvfrom()` call looks like this:

```c
int bytes_received = recvfrom(socket_peer, read, 1024, 0, 0, 0);
```

Let's break down the purpose of the last three arguments and why they are `0` (which is equivalent to `NULL` for pointers) in this specific scenario:

The `recvfrom()` function prototype is generally:

```c
ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
                 struct sockaddr *src_addr, socklen_t *addrlen);
```

And here's how the `0`s map to them:

1.  **`flags` (the `0` before the `src_addr`):**

      * This argument is for various control flags, such as `MSG_PEEK` (to look at data without removing it from the receive queue) or `MSG_WAITALL` (to wait until all data is available).
      * Setting it to `0` simply means no special flags are being used, which is common for a basic receive operation.

2.  **`src_addr` (the `0` that should be a `struct sockaddr *`):**

      * This is a pointer to a `struct sockaddr` (or a `struct sockaddr_in`/`sockaddr_in6` cast to `sockaddr*`). Its purpose is for `recvfrom()` to **fill in the address of the sender** of the datagram you just received.
      * If you provide a valid pointer here, `recvfrom()` will tell you *who sent the data*.

3.  **`addrlen` (the `0` that should be a `socklen_t *`):**

      * This is a pointer to a `socklen_t` variable. You would **initialize** the `socklen_t` with the *size of the buffer* pointed to by `src_addr` before the call.
      * Upon return, `recvfrom()` will **update** this `socklen_t` to contain the *actual size of the sender's address* that it wrote into `src_addr`.

### Why are they `0` in the code?

In the context of the DNS client, there's a strong assumption being made:

1.  **You're making a request to a specific, known server:** You explicitly configured the socket to send a query to `8.8.8.8` on port `53` using `getaddrinfo()` and `sendto()`.
2.  **You *expect* the response to come *only* from that server:** For a simple client that sends a query to a single, hardcoded DNS server, you generally don't need to verify the source address of the incoming packet. You assume that any valid DNS response you receive on that socket must be from `8.8.8.8`. If it were from somewhere else, it would likely be unexpected or malicious anyway.

**Therefore, by passing `0` for `src_addr` and `addrlen`, you are telling `recvfrom()`: "I don't care who sent this packet; just give me the data."**

This is a common simplification in client-side UDP applications where the communication is strictly between two known endpoints and the client doesn't need to dynamically determine the sender's address for subsequent actions.

If you were building a more robust or complex server that accepts connections from multiple clients, or a client that might receive unsolicited datagrams from unknown sources, you would absolutely need to provide valid `src_addr` and `addrlen` buffers to identify the sender of each incoming packet.
