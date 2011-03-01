#!/bin/sh

# This is not automake. But it makes dchen cringe to think I am
# reimplementing automake in sed. So I'm calling it that.

echo "Writing dep file"
gcc -MM *.c > .dep
echo "Rewriting makefile"
sed -i -n -e '/^\#\#\#\# AUTO \#\#\#\#$/{p ; b end} ; p ; d'  -e ':end { /\#\#\#\# END AUTO \#\#\#\#/b afterend  n ; b end}' -e ':afterend { r .dep
a #### END AUTO ####
}' Makefile
echo "Done"
rm .dep
