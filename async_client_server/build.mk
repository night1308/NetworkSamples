## CLIENT ##
TARGETS+=client

client_rule=c
client_autodepsources=client.c
client_cc=gcc -Wall -Werror -Wextra -Wshadow -Wpointer-arith -Wcast-qual -Wstrict-prototypes -Wmissing-prototypes
client_cpp=$(aclient_cc) -Wnon-virtual-dtor
client_ld=gcc
client_strip=strip
client_sourcesearchdirs=async_client_server
client_headersearchdirs=async_client_server
client_dynamiclibs=event

generated/client:

target_async_client: generated/client


## EXTRA ##
target_async_client_server: target_async_client
