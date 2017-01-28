# coding: utf8
import os
import subprocess

print "\nLancement des tests positifs :\n"
nb_success = nb_failures = 0

for file in os.listdir("tests/positive/"):
    command = "./texcc < tests/positive/%r > /dev/null" % file
    result = subprocess.check_output(command, shell=True)

    command = "spim -file output.s | tail -n 2 | head -n 1"
    result = subprocess.check_output(command, shell=True)

    if "Hello" in result or int(result) == 1:
        print "OK: %r" % file
        nb_success+=1
    else:
        print "FAILED: %r" % file
        nb_failures+=1

print "\nLancement des tests négatifs :\n"

for file in os.listdir("tests/negative/"):
    command = "./texcc < tests/negative/%r 2> /dev/null" % file

    try:
        result = subprocess.check_output(command, shell=True)
        print "FAILED: %r" % file
        nb_failures+=1
    except:
        print "OK: %r" % file
        nb_success+=1

print "\nTotal :"
print " - Nombre de tests réussis : %d" % nb_success
print " - Nombre de tests échoués : %d" % nb_failures
