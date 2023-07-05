project (httpclient_perf)

file (GLOB_RECURSE SOURCES *.cpp)

set(Boost_INCLUDE_DIR /home/artem_bogdanov/third_party/boost/include)
set(Boost_LIBRARY_DIR /home/artem_bogdanov/third_party/boost/lib)
find_package(Boost REQUIRED COMPONENTS program_options)

add_executable (${PROJECT_NAME} ${SOURCES})
target_link_libraries (${PROJECT_NAME}
    userver-core
    Boost::program_options
)
