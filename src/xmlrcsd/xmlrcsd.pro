SOURCES += \
    client.cpp \
    main.cpp \
    server.cpp \
    hiredis/async.c \
    hiredis/dict.c \
    hiredis/hiredis.c \
    hiredis/net.c \
    hiredis/sds.c \
    configuration.cpp \
    generic.cpp \
    streamitem.cpp

HEADERS += \
    client.hpp \
    configuration.hpp \
    server.hpp \
    hiredis/async.h \
    hiredis/dict.h \
    hiredis/fmacros.h \
    hiredis/hiredis.h \
    hiredis/net.h \
    hiredis/sds.h \
    generic.hpp \
    streamitem.hpp
