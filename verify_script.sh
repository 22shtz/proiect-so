#!/bin/bash
pat1="corrupted"
pat2="dangerous"
pat3="risk"
pat4="attack"
pat5="malware"
pat6="malicious"
pat7="virus"
file=$1

result=0
if grep -q "$pat1" "$file"; then
result=1
fi

if grep -q "$pat2" "$file"; then
result=1
fi

if grep -q "$pat3" "$file"; then
result=1
fi

if grep -q "$pat4" "$file"; then
result=1
fi

if grep -q "$pat5" "$file"; then
result=1
fi

if grep -q "$pat6" "$file"; then
result=1
fi

if grep -q "$pat7" "$file"; then
result=1
fi

if grep -q "[0x80-0xFF]" "$file"
then
result=1
fi

lines=$(wc -l < "$file")
words=$(wc -w < "$file")
chars=$(wc -m < "$file")

if [ "$lines" -lt  3 ] && [ "$words" -gt 1000 ] && [ "$chars" -gt 2000 ]; then 
    result=1
fi

if [ "$result" -eq 1 ];
then
	echo "$file"
else
	echo "SAFE"
fi

exit 1
