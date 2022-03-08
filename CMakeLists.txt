cmake_minimum_required( VERSION 3.8 )



project( "udraw" )



add_executable( "udraw" "main.cpp"  "wiimote.cpp" "wiimote.h" "input.cpp" "input.h" )
target_link_libraries( "udraw" "hid.dll" "setupapi.dll" )
