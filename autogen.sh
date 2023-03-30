#!/bin/sh

export AUTOMAKE="/opt/nec/ve/bin/ve-automake"
export ACLOCAL="/opt/nec/ve/bin/ve-aclocal"
export LIBTOOLIZE="/opt/nec/ve/bin/ve-libtoolize"
/opt/nec/ve/bin/ve-autoreconf -isf
