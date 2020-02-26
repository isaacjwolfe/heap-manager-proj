#---------------------------------------------------------------------
# Makefile
# Author: Bob Dondero
#---------------------------------------------------------------------

# This is not a proper Makefile!  It does not maintain .o files to
# allow for partial builds. Instead it simply automates the issuing
# of commands for each step of the assignment.

#---------------------------------------------------------------------
# Build rules for non-file targets
#---------------------------------------------------------------------

all: step1 step2 step3 step4 step5 step6 step7

clean:
	rm -f test1 test2 test3 test4* test5*

#---------------------------------------------------------------------
# Build rules for the steps of the assignment
#---------------------------------------------------------------------

step1:
	#------------------------------------------------------------
	# step1
	#------------------------------------------------------------ 
	gcc217 -g testheapmgr.c heapmgr4bada.o checker4.c \
		chunk4.c -o test4bada
	gcc217 -g testheapmgr.c heapmgr4badb.o checker4.c \
		chunk4.c -o test4badb
	gcc217 -g testheapmgr.c heapmgr4badc.o checker4.c \
		chunk4.c -o test4badc
	gcc217 -g testheapmgr.c heapmgr4badd.o checker4.c \
		chunk4.c -o test4badd
	gcc217 -g testheapmgr.c heapmgr4bade.o checker4.c \
		chunk4.c -o test4bade
	gcc217 -g testheapmgr.c heapmgr4badf.o checker4.c \
		chunk4.c -o test4badf
	gcc217 -g testheapmgr.c heapmgr4badg.o checker4.c \
		chunk4.c -o test4badg

step2:
	#------------------------------------------------------------
	# step2
	#------------------------------------------------------------
	gcc217 -g testheapmgr.c heapmgr4.c checker4.c \
		chunk4.c -o test4d
	gcc217 -D NDEBUG -O testheapmgr.c heapmgr4.c \
		chunk4.c -o test4
	gcc217 -D NDEBUG -O testheapmgr.c heapmgr4good.o \
		chunk4.c -o test4good

step3:
	#------------------------------------------------------------
	# step3
	#------------------------------------------------------------
	splint testheapmgr.c heapmgr4.c checker4.c chunk4.c
	critTer checker4.c
	critTer heapmgr4.c

step4:
	#------------------------------------------------------------
	# step4
	#------------------------------------------------------------
	gcc217 -g testheapmgr.c heapmgr5bada.o checker5.c \
		chunk5.c -o test5bada
	gcc217 -g testheapmgr.c heapmgr5badb.o checker5.c \
		chunk5.c -o test5badb
	gcc217 -g testheapmgr.c heapmgr5badc.o checker5.c \
		chunk5.c -o test5badc
	gcc217 -g testheapmgr.c heapmgr5badd.o checker5.c \
		chunk5.c -o test5badd
	gcc217 -g testheapmgr.c heapmgr5bade.o checker5.c \
		chunk5.c -o test5bade
	gcc217 -g testheapmgr.c heapmgr5badf.o checker5.c \
		chunk5.c -o test5badf
	gcc217 -g testheapmgr.c heapmgr5badg.o checker5.c \
		chunk5.c -o test5badg
	gcc217 -g testheapmgr.c heapmgr5badh.o checker5.c \
		chunk5.c -o test5badh

step5:
	#------------------------------------------------------------
	# step5
	#------------------------------------------------------------
	gcc217 -g testheapmgr.c heapmgr5.c checker5.c chunk5.c \
		-o test5d
	gcc217 -D NDEBUG -O testheapmgr.c heapmgr5.c chunk5.c \
		-o test5
	gcc217 -D NDEBUG -O testheapmgr.c heapmgr5good.o chunk5.c \
		-o test5good

step6:
	#------------------------------------------------------------
	# step6
	#------------------------------------------------------------
	splint testheapmgr.c heapmgr5.c checker5.c chunk5.c
	critTer checker5.c
	critTer heapmgr5.c

step7:
	#------------------------------------------------------------
	# step7
	#------------------------------------------------------------
	gcc217 -D NDEBUG -O testheapmgr.c heapmgr1.c -o test1
	gcc217 -D NDEBUG -O testheapmgr.c heapmgr2.c -o test2
	gcc217 -D NDEBUG -O testheapmgr.c heapmgr3.c chunk3.c -o test3
