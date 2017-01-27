cmake_minimum_required(VERSION 2.8)
PROJECT(TProxy)

INCLUDE_DIRECTORIES(
/usr/local/include/
)

AUX_SOURCE_DIRECTORY(
./
DIRA
)


set(DIR_SRCS
${DIRA}
)

link_directories(
/usr/local/lib/
)

SET(LIBRARIES 
    libjson-c.so
)

ADD_EXECUTABLE(TProxy ${DIR_SRCS})

TARGET_LINK_LIBRARIES(TProxy ${LIBRARIES})
