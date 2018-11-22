#!/bin/bash

#mpiexec='mpiexec'		# Laptop test
mpiexec='/z/eduardo/CLUES/libs/bin/mpiexec'

cd ..; make; cd -
rm ../tmp/*.tmp

$mpiexec -n 4 ../bin/MetroCPP ../config/fullbox_1024.cfg
#$mpiexec -n 4 ../bin/MetroCPP ../config/fullbox_test.cfg	# Laptop setting
