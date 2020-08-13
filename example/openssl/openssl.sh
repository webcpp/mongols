#!/bin/sh

openssl req -x509 -out localhost.crt -keyout localhost.key \
  -newkey rsa:2048 -nodes -sha256 -days 3650\
  -subj '/CN=localhost' -extensions EXT -config openssl.config
