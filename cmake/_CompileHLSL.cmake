function(CompileHLSL_GetSupportedCompilers outSupportedCompilers)
    list(APPEND supportedCompilers "dxc")
    set(${outSupportedCompilers} ${supportedCompilers} PARENT_SCOPE)
endfunction()

function(CompileHLSL_FindCompilerCommands HLSL_COMPILER_COMMAND)
    CompileHLSL_GetSupportedCompilers(candidateHLSLCompilerCommands)
    find_program(_possibleHLSLCompilerCommand
        NAMES ${candidateHLSLCompilerCommands}
        PATHS /usr/bin
              /usr/local/bin
              $ENV{VULKAN_SDK}/Bin/
              $ENV{VULKAN_SDK}/Bin32/
    )

    if(${_possibleHLSLCompilerCommand} STREQUAL "_possbileHLSLCompilerCommand-NOTFOUND")
        message(FATAL_ERROR
            "One of the programs in `${candidateHLSLCompilerCommands}` could not be found. "
            "Make sure a HLSL compiler such as `dxc` is installed on your system. This "
            "would typically come alongside either Direct3D or Vulkan SDK. Otherwise, "
            "install it by another means. "
        )
    endif()

    set(${HLSL_COMPILER_COMMAND} ${_possibleHLSLCompilerCommand} PARENT_SCOPE)
endfunction()

function(CompileHLSL_FindSourceFiles inShaderPath outFoundShaderFiles)
    file(GLOB_RECURSE HLSL_SOURCE_FILES
        "${inShaderPath}/*.frag.hlsl"
        "${inShaderPath}/*.vert.hlsl"
        "${inShaderPath}/*.comp.hlsl"
    )

    set(${outFoundShaderFiles} ${HLSL_SOURCE_FILES} PARENT_SCOPE)
endfunction()

function(CompileHLSL_GetShaderType inShaderFile outShaderType)
    get_filename_component(shaderFileName ${inShaderFile} NAME)
    get_filename_component(shaderType_extension ${shaderFileName} EXT)
    
    string(REPLACE "." ";" fileParts ${shaderType_extension})
    list(LENGTH fileParts partCount)
    
    if(NOT partCount EQUAL 3)
        set(${outShaderType} "" PARENT_SCOPE)
        message(FATAL_ERROR
            "Expected shader file name of the form `<shaderName>.<vert|frag|comp>.hlsl`. "
            "Got `${inShaderFileName}` from input `${inShaderFile}`."
        )
    endif()

    list(GET fileParts 1 shaderType)
    set(${outShaderType} ${shaderType} PARENT_SCOPE)
endfunction()

function(CompileHLSL_GetCompileOptions inShaderType outCompileOptions)
    set(shaderStage)
    if(inShaderType STREQUAL "vert")
        set(shaderStage "vs_6_1")
    elseif(inShaderType STREQUAL "frag")
        set(shaderStage "ps_6_1")
    elseif(inShaderType STREQUAL "comp")
        set(shaderStage "cs_6_1")
    else()
        message(FATAL_ERROR "Expected `inShaderType` to be one of `vert`, `frag`, or `comp`. Got `${inShaderType}`.")
    endif()

    list(APPEND compileOptions -spirv -T ${shaderStage} -E main -Fo ${spirvOutputFile} ${inShaderSourceFile})

    set(${outCompileOptions} ${compileOptions} PARENT_SCOPE)
endfunction()

function(CompileHLSL_CompileShader HLSL_COMPILER_COMMAND inShaderSourceFile inSpirvBinaryOutputPath outSpirvBinaryFile)
    get_filename_component(fileName ${inShaderSourceFile} NAME)
    set(spirvOutputFile "${inSpirvBinaryOutputPath}/${fileName}.spv")
    
    set(shaderType)
    CompileHLSL_GetShaderType(${inShaderSourceFile} shaderType)
    
    set(compileOptions)
    CompileHLSL_GetCompileOptions(${shaderType} compileOptions)

    list(APPEND fullCommand ${HLSL_COMPILER_COMMAND} ${compileOptions})

    add_custom_command(
        OUTPUT ${spirvOutputFile}
        COMMAND ${CMAKE_COMMAND} -E echo "COMPILING SHADER `${fileName}`"
        COMMAND ${CMAKE_COMMAND} -E echo "${fullCommand}"
        COMMAND ${fullCommand}
        COMMAND ${CMAKE_COMMAND} -E echo "DONE"
        DEPENDS ${InShaderSourceFile}
        VERBATIM
    )

    set(${outSpirvBinaryFile} ${spirvOutputFile} PARENT_SCOPE)
endfunction()

function(CompileHLSL_CompileShaders HLSL_COMPILER_COMMAND inShaderSourceFiles inSpirvBinaryOutputPath outSpirvBinaryFiles)
    list(APPEND spirvBinaryFiles)
    foreach(shaderSourceFile IN LISTS inShaderSourceFiles)
        get_filename_component(fileName ${shaderSourceFile} NAME)
        set(spirvOutputFile "${inSpirvBinaryOutputPath}/${fileName}.spv")

        message(STATUS "BUILDING SHADER")
        message(STATUS "SOURCE `${shaderSourceFile}`")
        message(STATUS "TARGET `${spirvOutputFile}`")

        set(spirvBinaryFile)
        CompileHLSL_CompileShader(${HLSL_COMPILER_COMMAND} ${shaderSourceFile} ${inSpirvBinaryOutputPath} spirvBinaryFile)

        list(APPEND spirvBinaryFiles ${spirvBinaryFile})
    endforeach()

    set(${outSpirvBinaryFiles} ${spirvBinaryFiles} PARENT_SCOPE)
endfunction()

function(CompileHLSL_EmbedShader inSpirvBinaryFile inSpirvEmbeddingOutputPath outSpirvBinaryEmbeddingFile)
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

function(CompileHLSL_EmbedShaders inSpirvBinaryFiles inSpirvEmbeddingOutputPath outSpirvBinaryEmbeddingFiles)
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

function(CompileHLSL_CombineEmbeddedShaders inSpirvEmbeddingFiles inFinalShaderEmbeddingOutputPath inFinalShaderEmbeddingFileName outFinalShaderEmbeddingFile)
    string(APPEND combinedContent "")
    string(APPEND combinedContent "#include <unordered_map>\n")
    string(APPEND combinedContent "#include <string>\n")
    string(APPEND combinedContent "#include <vector>\n\n\n")
    string(APPEND combinedContent "const std::unordered_map<std::string, std::vector<unsigned char>> HLSL_SHADERS = std::unordered_map<std::string, std::vector<unsigned char>> {\n")

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
