include(FetchContent)
FetchContent_Declare(
	cxxopts
	GIT_REPOSITORY https://github.com/jarro2783/cxxopts.git
	GIT_TAG        714a105fe6e965df780c4c6f3c51affde2ca7075    
)
FetchContent_MakeAvailable(cxxopts)