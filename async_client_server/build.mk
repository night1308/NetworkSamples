## CLIENT ##
TARGETS+=client

client_rule=c
client_autodepsources=client.c
client_cc=gcc -Wall -Werror -Wextra -Wshadow -Wpointer-arith -Wcast-qual -Wstrict-prototypes -Wmissing-prototypes
client_cpp=$(client_cc) -Wnon-virtual-dtor
client_ld=gcc
client_strip=strip
client_sourcesearchdirs=async_client_server
client_headersearchdirs=async_client_server
client_dynamiclibs=event

generated/client:

target_async_client: generated/client


## SERVER ##
TARGETS+=server

server_rule=c
server_autodepsources=server.c
server_cc=gcc -Wall -Werror -Wextra -Wshadow -Wpointer-arith -Wcast-qual -Wstrict-prototypes -Wmissing-prototypes
server_cpp=$(client_cc) -Wnon-virtual-dtor
server_ld=gcc
server_strip=strip
server_sourcesearchdirs=async_client_server
server_headersearchdirs=async_client_server
server_dynamiclibs=event

generated/server:

target_async_server: generated/server


## EXTRA ##
target_async_client_server: target_async_client target_async_server
