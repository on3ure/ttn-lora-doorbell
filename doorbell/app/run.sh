#!/bin/bash

# production mojo
PERL_ANYEVENT_RESOLV_CONF=resolv.conf carton exec hypnotoad -f mojo.pl
