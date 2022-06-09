add_llvm_library( LLVMSuperopt MODULE
  SplitModule.cpp
  InstrumentMarkerCall.cpp
  ConvertLocalLinkageToGlobal.cpp

  DEPENDS
  intrinsics_gen
  PLUGIN_TOOL
  opt
  )

#add_superopt_libs(slibs)
#target_link_libraries(LLVMSuperopt PRIVATE ${slibs})
