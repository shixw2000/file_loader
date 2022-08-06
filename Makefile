release=

RM=rm -f

COMILE_OPT=
MACROS=-D__TEST_LOST_PKG__N

ifndef release
CC=g++ -g -W -Wall  $(MACROS) $(COMILE_OPT)
else
#-s :ɾ����ִ�г����е����з��ű�������ض�λ��Ϣ���������������� strip ���ﵽ��Ч����ͬ
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


