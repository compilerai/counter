add_llvm_library( LLVMSuperopt MODULE
  SplitModule.cpp
  InstrumentMarkerCall.cpp

  DEPENDS
  intrinsics_gen
  PLUGIN_TOOL
  opt
  )

#target_link_libraries(LLVMSuperopt PRIVATE ${SUPEROPT_LIBS})
