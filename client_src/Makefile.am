INCLUDES = \
		   -I../common_src \
		   -I../proto_src

AM_CXXFLAGS =

client_LDADD = -lpthread -levent -lprotobuf

bin_PROGRAMS = client

client_SOURCES = \
				 ../common_src/net_thread.cpp \
				 ../common_src/msg_protobuf.cpp \
				 ../common_src/connector.cpp \
				 ../common_src/read_cb.cpp \
				 ../common_src/write_cb.cpp \
				 ../common_src/event_cb.cpp \
				 ../common_src/log.cpp \
				 ../common_src/logic_thread.cpp \
				 ../proto_src/cmd_util.cpp \
				 ../proto_src/login.pb.cc \
				 server_cb.cpp \
				 main.cpp

