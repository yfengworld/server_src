INCLUDES = \
	   -I/usr/local/include/zdb \
	   -I../common_src \
	   -I../proto_src

AM_CXXFLAGS =

login_LDADD = -lzdb -lpthread -levent_pthreads -lprotobuf

bin_PROGRAMS = login

login_SOURCES = \
			   ../common_src/atomic_counter.cpp \
			   ../common_src/log.cpp \
			   ../common_src/net_thread.cpp \
			   ../common_src/msg_protobuf.cpp \
			   ../common_src/connection.cpp \
			   ../common_src/listener.cpp \
			   ../common_src/connector.cpp \
			   ../common_src/accept_cb.cpp \
			   ../common_src/read_cb.cpp \
			   ../common_src/write_cb.cpp \
			   ../common_src/conn_event_cb.cpp \
			   ../common_src/connector_event_cb.cpp \
			   ../proto_src/cmd_util.cpp \
			   ../proto_src/login.pb.cc \
			   client_cb.cpp \
			   center_cb.cpp \
			   center_info.cpp \
			   user.cpp \
			   main.cpp

