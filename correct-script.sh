ge=/home/mrjantz/cs360/labs/Lab-4-Fakemake/Gradescript-Examples
cp $ge/twofile-0.c twofile.c
cp $ge/first_three-0.c first_three.c
rm -f twofile.o first_three.o twoexec
gcc -c twofile.c
gcc -o twoexec twofile.c first_three.c
touch -t 01011110.10 twofile.c
touch -t 01011110.15 first_three.c
touch -t 01011110.05 twofile.o
touch -t 01011110.08 twoexec
cp $ge/008.fm fmakefile
if /home/mrjantz/cs360/labs/Lab-4-Fakemake/fakemake; then
  ./twoexec
fi
