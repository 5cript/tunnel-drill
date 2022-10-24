include(FetchContent)
FetchContent_Declare(
	spdlog
	GIT_REPOSITORY https://github.com/gabime/spdlog.git
	GIT_TAG        3c0e036cc9e12664e2b757fd17fdba42f6f19f90    
)
FetchContent_MakeAvailable(spdlog)