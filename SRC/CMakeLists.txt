cmake_policy (SET CMP0015 NEW)

function (add_project NAME ALLSOURCES)	
	
   set (ALLSOURCES ${ALLSOURCES} ${ARGN})    
   add_definitions("-std=c++11")

   add_executable(${NAME} ${ALLSOURCES})
   #target_link_libraries(${NAME} Qt5::Core Qt5::Network)
   target_link_libraries(${NAME} ${QT_LIBRARIES})

endfunction(add_project)

add_subdirectory (Analyzer)
add_subdirectory (Loader)
