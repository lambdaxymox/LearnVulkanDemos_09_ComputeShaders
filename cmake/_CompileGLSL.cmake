function(CompileGLSL_GetSupportedCompilers outSupportedCompilers)
    list(APPEND supportedCompilers "glslc" "glslangValidator")
    set(${outSupportedCompilers} ${supportedCompilers} PARENT_SCOPE)
endfunction()

function(CompileGLSL_FindCompilerCommands GLSL_COMPILER_COMMAND)
    CompileGLSL_GetSupportedCompilers(candidateGLSLCompilerCommands)
    find_program(_possibleGLSLCompilerCommand
        NAMES ${candidateGLSLCompilerCommands}
        PATHS /usr/bin
              /usr/local/bin
              $ENV{VULKAN_SDK}/Bin/
              $ENV{VULKAN_SDK}/Bin32/
    )

    if(${_possibleGLSLCompilerCommand} STREQUAL "_possibleGLSLCompilerCommand-NOTFOUND")
        message(FATAL_ERROR
            "One of the programs in `${candidateGLSLCompilerCommands}` could not be found. "
            "Make sure a GLSL compiler such as `glslc` or `glslangValidator` is installed on your system. This "
            "would typically come alongside either an OpenGL, Direct3D or Vulkan SDK. Otherwise, "
            "install it by another means. "
        )
    endif()

    set(${GLSL_COMPILER_COMMAND} ${_possibleGLSLCompilerCommand} PARENT_SCOPE)
endfunction()

function(CompileGLSL_FindSourceFiles inShaderPath outFoundShaderFiles)
    file(GLOB_RECURSE GLSL_SOURCE_FILES
        "${inShaderPath}/*.frag.glsl"
        "${inShaderPath}/*.vert.glsl"
        "${inShaderPath}/*.comp.glsl"
    )

    set(${outFoundShaderFiles} ${GLSL_SOURCE_FILES} PARENT_SCOPE)
endfunction()

function(CompileGLSL_GetShaderStage inShaderFile outShaderStage)
    get_filename_component(shaderFileName ${inShaderFile} NAME)
    get_filename_component(shaderType_extension ${shaderFileName} EXT)
    
    string(REPLACE "." ";" fileParts ${shaderType_extension})
    list(LENGTH fileParts partCount)
    
    if(NOT partCount EQUAL 3)
        message(FATAL_ERROR
            "Expected shader file name of the form `<shaderName>.<vert|frag|comp>.glsl`."
            "Got `${inShaderFileName}` from input `${inShaderFile}`."
        )
    endif()

    list(GET fileParts 1 shaderType)
    set(shaderStage)
    if (${shaderType} STREQUAL "vert")
        set(shaderStage "vertex")
    elseif (${shaderType} STREQUAL "frag")
        set(shaderStage "fragment")
    elseif (${shaderType} STREQUAL "comp")
        set(shaderStage "compute")
    else()
        message(FATAL_ERROR "Expected a shader with shader type `vert`, `frag`, or `comp`. Got `${shaderType}`.")
    endif()

    set(${outShaderStage} ${shaderStage} PARENT_SCOPE)
endfunction()

function(CompileGLSL_GetCompileOptions GLSL_COMPILER_COMMAND inShaderSourceFile inSpirvBinaryFile inShaderStage outCompileOptions)
    set(glslCompilerName)
    get_filename_component(glslCompilerName ${GLSL_COMPILER_COMMAND} NAME)

    set(compileOptions)
    if (${glslCompilerName} STREQUAL "glslc")
        list(APPEND compileOptions -fshader-stage=${inShaderStage} -o "${inSpirvBinaryFile}" "${inShaderSourceFile}")
    elseif (${glslCompilerName} STREQUAL "glslangValidator")
        list(APPEND compileOptions -V "${inShaderSourceFile}" -o "${inSpirvBinaryFile}")
    else()
        message(FATAL_ERROR
            "Unsupported compiler. The supported compilers are `glslc` and `glslangValidator`. "
            "Got `${GLSL_COMPILER_COMMAND}`."
        )
    endif()

    set(${outCompileOptions} ${compileOptions} PARENT_SCOPE)
endfunction()

function(CompileGLSL_CompileShader GLSL_COMPILER_COMMAND inShaderSourceFile inSpirvBinaryOutputPath outSpirvBinaryFile)
    get_filename_component(fileName ${inShaderSourceFile} NAME)
    set(spirvOutputFile "${inSpirvBinaryOutputPath}/${fileName}.spv")

    set(shaderStage)
    CompileGLSL_GetShaderStage(${inShaderSourceFile} shaderStage)

    set(compileOptions)
    CompileGLSL_GetCompileOptions(${GLSL_COMPILER_COMMAND} ${inShaderSourceFile} ${spirvOutputFile} ${shaderStage} compileOptions)

    list(APPEND fullCommand ${GLSL_COMPILER_COMMAND} ${compileOptions})

    add_custom_command(
        OUTPUT ${spirvOutputFile}
        COMMAND ${CMAKE_COMMAND} -E echo "COMPILING SHADER `${fileName}`"
        COMMAND ${CMAKE_COMMAND} -E echo "${fullCommand}"
        COMMAND ${fullCommand}
        COMMAND ${CMAKE_COMMAND} -E echo "DONE"
        DEPENDS ${inShaderSourceFile}
        VERBATIM
    )

    set(${outSpirvBinaryFile} ${spirvOutputFile} PARENT_SCOPE)
endfunction()

function(CompileGLSL_CompileShaders GLSL_COMPILER_COMMAND inShaderSourceFiles inSpirvBinaryOutputPath outSpirvBinaryFiles)
    list(APPEND spirvBinaryFiles)
    foreach(shaderSourceFile IN LISTS inShaderSourceFiles)
        get_filename_component(fileName ${shaderSourceFile} NAME)
        set(spirvOutputFile "${inSpirvBinaryOutputPath}/${fileName}.spv")

        message(STATUS "BUILDING SHADER")
        message(STATUS "SOURCE `${shaderSourceFile}`")
        message(STATUS "TARGET `${spirvOutputFile}`")

        set(spirvOutputFile)
        CompileGLSL_CompileShader(${GLSL_COMPILER_COMMAND} ${shaderSourceFile} ${inSpirvBinaryOutputPath} spirvOutputFile)

        list(APPEND spirvBinaryFiles ${spirvOutputFile})
    endforeach()
    
    set(${outSpirvBinaryFiles} ${spirvBinaryFiles} PARENT_SCOPE)
endfunction()

function(CompileGLSL_EmbedShader inSpirvBinaryFile inSpirvEmbeddingOutputPath outSpirvBinaryEmbeddingFile)
    get_filename_component(fileName ${inSpirvBinaryFile} NAME)
    set(spirvBinaryEmbeddingFileName "${fileName}.embedded.in")
    set(spirvBinaryEmbeddingFile "${inSpirvEmbeddingOutputPath}/${spirvBinaryEmbeddingFileName}")
    string(REPLACE "." "_" shaderEmbeddingVariableName ${fileName})

    add_custom_command(
        OUTPUT ${spirvBinaryEmbeddingFile}
        COMMAND xxd -i < ${inSpirvBinaryFile} > ${spirvBinaryEmbeddingFile}
        DEPENDS ${inSpirvBinaryFile}
        VERBATIM
    )

    set(${outSpirvBinaryEmbeddingFile} ${spirvBinaryEmbeddingFile} PARENT_SCOPE)
endfunction()

function(CompileGLSL_EmbedShaders inSpirvBinaryFiles inSpirvEmbeddingOutputPath outSpirvBinaryEmbeddingFiles)
    set(spirvBinaryEmbeddingFiles)
    foreach(spirvBinaryFile IN LISTS inSpirvBinaryFiles)
        get_filename_component(fileName ${spirvBinaryFile} NAME_WE)
        set(spirvBinaryEmbeddingFileName "${fileName}.embedded.in")
        set(spirvBinaryEmbeddingFile "${inSpirvEmbeddingOutputPath}/${spirvBinaryEmbeddingFileName}")

        message(STATUS "EMBEDDING SHADER")
        message(STATUS "SOURCE `${spirvBinaryFile}`")
        message(STATUS "TARGET `${spirvBinaryEmbeddingFile}`")
        
        set(spirvBinaryEmbeddingFile "")
        CompileGLSL_EmbedShader(${spirvBinaryFile} ${inSpirvEmbeddingOutputPath} spirvBinaryEmbeddingFile)

        list(APPEND spirvBinaryEmbeddingFiles ${spirvBinaryEmbeddingFile})
    endforeach()
    
    set(${outSpirvBinaryEmbeddingFiles} ${spirvBinaryEmbeddingFiles} PARENT_SCOPE)
endfunction()

function(CompileGLSL_CombineEmbeddedShaders inSpirvEmbeddingFiles inFinalShaderEmbeddingOutputPath inFinalShaderEmbeddingFileName outFinalShaderEmbeddingFile)
    string(APPEND combinedContent "")
    string(APPEND combinedContent "#include <unordered_map>\n")
    string(APPEND combinedContent "#include <string>\n")
    string(APPEND combinedContent "#include <vector>\n\n\n")
    string(APPEND combinedContent "const std::unordered_map<std::string, std::vector<unsigned char>> GLSL_SHADERS = std::unordered_map<std::string, std::vector<unsigned char>> {\n")

    foreach(spirvEmbeddingFile IN LISTS inSpirvEmbeddingFiles)
        get_filename_component(fileName ${spirvEmbeddingFile} NAME)
        string(REGEX REPLACE ".spv.embedded.in$" "" shaderName ${fileName})

        file(READ ${spirvEmbeddingFile} fileContent)
        # Format the contents of the embedded shader by first removing all the 
        # prepended white space on each line, and then replacing it with the desired
        # amount of white space.
        string(REGEX REPLACE "\n  " "\n" fileContent "${fileContent}")
        string(REGEX REPLACE "^  " "            " fileContent "${fileContent}")
        string(REGEX REPLACE ",\n" ",\n            " fileContent "${fileContent}")

        string(APPEND combinedContent "    {\n")
        string(APPEND combinedContent "        \"${shaderName}\",\n")
        string(APPEND combinedContent "        std::vector<unsigned char> {\n")
        string(APPEND combinedContent "${fileContent}")
        string(APPEND combinedContent "        }\n")
        string(APPEND combinedContent "    },\n")
    endforeach()

    string(APPEND combinedContent "}\;\n")

    set(finalOutputFileName "${inFinalShaderEmbeddingFileName}.in")
    set(finalOutputFile "${inFinalShaderEmbeddingOutputPath}/${finalOutputFileName}")
    file(WRITE ${finalOutputFile} ${combinedContent})

    set(${outFinalShaderEmbeddingFile} ${finalOutputFile} PARENT_SCOPE)
endfunction()
