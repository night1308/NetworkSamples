include bs/firstline.mk

include async_client_server/build.mk
include async_client_server_ssl/build.mk

default: target_most
most: target_most

target_most: target_async_client_server

include bs/lastline.mk
