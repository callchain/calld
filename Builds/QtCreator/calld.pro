
# Call protocol buffers

PROTOS = ../../src/call_data/protocol/call.proto
PROTOS_DIR = ../../build/proto

# Google Protocol Buffers support

protobuf_h.name = protobuf header
protobuf_h.input = PROTOS
protobuf_h.output = $${PROTOS_DIR}/${QMAKE_FILE_BASE}.pb.h
protobuf_h.depends = ${QMAKE_FILE_NAME}
protobuf_h.commands = protoc --cpp_out=$${PROTOS_DIR} --proto_path=${QMAKE_FILE_PATH} ${QMAKE_FILE_NAME}
protobuf_h.variable_out = HEADERS
QMAKE_EXTRA_COMPILERS += protobuf_h

protobuf_cc.name = protobuf implementation
protobuf_cc.input = PROTOS
protobuf_cc.output = $${PROTOS_DIR}/${QMAKE_FILE_BASE}.pb.cc
protobuf_cc.depends = $${PROTOS_DIR}/${QMAKE_FILE_BASE}.pb.h
protobuf_cc.commands = $$escape_expand(\\n)
#protobuf_cc.variable_out = SOURCES
QMAKE_EXTRA_COMPILERS += protobuf_cc

# Call compilation

DESTDIR = ../../build/QtCreator
OBJECTS_DIR = ../../build/QtCreator/obj

TEMPLATE = app
CONFIG += console thread warn_off
CONFIG -= qt gui

DEFINES += _DEBUG

linux-g++:QMAKE_CXXFLAGS += \
    -Wall \
    -Wno-sign-compare \
    -Wno-char-subscripts \
    -Wno-invalid-offsetof \
    -Wno-unused-parameter \
    -Wformat \
    -O0 \
    -std=c++11 \
    -pthread

INCLUDEPATH += \
    "../../src/leveldb/" \
    "../../src/leveldb/port" \
    "../../src/leveldb/include" \
    $${PROTOS_DIR}

OTHER_FILES += \
#   $$files(../../src/*, true) \
#   $$files(../../src/beast/*) \
#   $$files(../../src/beast/modules/beast_basics/diagnostic/*)
#   $$files(../../src/beast/modules/beast_core/, true)

UI_HEADERS_DIR += ../../src/call_basics

# ---------
# New style
#
SOURCES += \
    ../../src/call/beast/call_beast.unity.cpp \
    ../../src/call/beast/call_beastc.c \
    ../../src/call/common/call_common.unity.cpp \
    ../../src/call/http/call_http.unity.cpp \
    ../../src/call/json/call_json.unity.cpp \
    ../../src/call/peerfinder/call_peerfinder.unity.cpp \
    ../../src/call/radmap/call_radmap.unity.cpp \
    ../../src/call/resource/call_resource.unity.cpp \
    ../../src/call/sitefiles/call_sitefiles.unity.cpp \
    ../../src/call/sslutil/call_sslutil.unity.cpp \
    ../../src/call/testoverlay/call_testoverlay.unity.cpp \
    ../../src/call/types/call_types.unity.cpp \
    ../../src/call/validators/call_validators.unity.cpp

# ---------
# Old style
#
SOURCES += \
    ../../src/call_app/call_app.unity.cpp \
    ../../src/call_app/call_app_pt1.unity.cpp \
    ../../src/call_app/call_app_pt2.unity.cpp \
    ../../src/call_app/call_app_pt3.unity.cpp \
    ../../src/call_app/call_app_pt4.unity.cpp \
    ../../src/call_app/call_app_pt5.unity.cpp \
    ../../src/call_app/call_app_pt6.unity.cpp \
    ../../src/call_app/call_app_pt7.unity.cpp \
    ../../src/call_app/call_app_pt8.unity.cpp \
    ../../src/call_basics/call_basics.unity.cpp \
    ../../src/call_core/call_core.unity.cpp \
    ../../src/call_data/call_data.unity.cpp \
    ../../src/call_hyperleveldb/call_hyperleveldb.unity.cpp \
    ../../src/call_leveldb/call_leveldb.unity.cpp \
    ../../src/call_net/call_net.unity.cpp \
    ../../src/call_overlay/call_overlay.unity.cpp \
    ../../src/call_rpc/call_rpc.unity.cpp \
    ../../src/call_websocket/call_websocket.unity.cpp

LIBS += \
    -lboost_date_time-mt\
    -lboost_filesystem-mt \
    -lboost_program_options-mt \
    -lboost_regex-mt \
    -lboost_system-mt \
    -lboost_thread-mt \
    -lboost_random-mt \
    -lprotobuf \
    -lssl \
    -lrt
