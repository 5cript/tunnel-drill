#!/bin/bash

openssl req -nodes -x509 -newkey rsa:4096 -keyout ~/.tbore/key.pem -out ~/.tbore/cert.pem -sha256 -days 365