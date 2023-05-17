include(FetchContent)
FetchContent_Declare(
	jwt-cpp
	GIT_REPOSITORY https://github.com/Thalhammer/jwt-cpp.git
	GIT_TAG        4e3dd40cfef9f06fa48c8a8d804405ec196885cf    
)
FetchContent_MakeAvailable(jwt-cpp)