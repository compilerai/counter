include_directories(
  ${LLVM_MAIN_SRC_DIR}/lib/Target/X86
  ${LLVM_BINARY_DIR}/lib/Target/X86
  )

set(LLVM_LINK_COMPONENTS
  AllTargetsAsmParsers
  AllTargetsCodeGens
  AllTargetsDescs
  AllTargetsInfos
  Analysis
  AsmPrinter
  CodeGen
  Core
  IRReader
  MC
  MIRParser
  Remarks
  ScalarOpts
  SelectionDAG
  Support
  Target
  TransformUtils
  Vectorize
  )

add_llvm_tool(llc
  llc.cpp

  DEPENDS
  intrinsics_gen
  SUPPORT_PLUGINS
  )

target_link_libraries(llc PRIVATE ${SUPEROPT_LIBS})

export_executable_symbols_for_plugins(llc)
