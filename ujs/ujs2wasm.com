#!/bin/sh
exec sh "${0%/*}/ujs.com" "$@"
