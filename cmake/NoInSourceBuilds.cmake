function(CheckNoInSourceBuilds)
    if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
        message(
            FATAL_ERROR
            "\n"
            "In-source builds are not allowed.\n"
            "Instead, provide a path to the build tree like so:\n"
            "`cmake -B <destination>`\n"
            "\n"
            "To remove files you accidentally created, run:\n"
            "`rm -rf CMakeFiles CMakeCache.txt`\n"
        )
    endif()
endfunction()
