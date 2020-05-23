// The Switch uses a little endian ARMv8 processor

#include <machine/endian.h>
#include <pwd.h>
#include <sys/types.h>

uint32_t htonl(uint32_t hostlong)
{
    return __builtin_bswap32(hostlong);
}

uint16_t htons(uint16_t hostshort)
{
    return __builtin_bswap16(hostshort);
}

uint32_t ntohl(uint32_t netlong)
{
    return __builtin_bswap32(netlong);
}

uint16_t ntohs(uint16_t netshort)
{
    return __builtin_bswap16(netshort);
}

uid_t getuid(void)
{
    return 0;
}

struct passwd *getpwuid(uid_t uid)
{
    struct passwd pass;

    pass.pw_dir = "/citra";

    return &pass;
}