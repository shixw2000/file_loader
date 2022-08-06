release=

RM=rm -f

COMILE_OPT=
MACROS=-D__TEST_LOST_PKG__N

ifndef release
CC=g++ -g -W -Wall  $(MACROS) $(COMILE_OPT)
else
#-s :删除可执行程序中的所有符号表和所有重定位信息。其结果与运行命令 strip 所达到的效果相同
CC=g++ -s -O3 -W -Wall $(MACROS) $(COMILE_OPT)
endif


prog:=bin_test
inc_dir=
lib_dir=
libs=-lpthread -lrt

srcs=$(wildcard *.cpp)
	
objs=$(srcs:.cpp=.o)

default:$(prog)
.PHONY:default

$(prog):$(objs)
	$(CC) -o $@ $^ $(lib_dir) $(libs)

$(objs):%.o:%.cpp
	$(CC) -c -o $@ $< $(inc_dir)	

clean:
	-@$(RM)  $(objs) $(prog)
.PHONY:clean


