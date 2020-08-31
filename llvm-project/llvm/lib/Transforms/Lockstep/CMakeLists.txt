# If we don't need RTTI or EH, there's no reason to export anything
# from the hello plugin.
if( NOT LLVM_REQUIRES_RTTI )
  if( NOT LLVM_REQUIRES_EH )
    set(LLVM_EXPORTED_SYMBOL_FILE ${CMAKE_CURRENT_SOURCE_DIR}/Lockstep.exports)
  endif()
endif()

if(WIN32 OR CYGWIN)
  set(LLVM_LINK_COMPONENTS Core Support)
endif()

#set(LogLib /home/tarun/btp/library/unseq_pred_gen/libLogLibrary.so)
add_llvm_library( LLVMLockstep MODULE BUILDTREE_ONLY
  Lockstep.cpp

  DEPENDS
  intrinsics_gen
  PLUGIN_TOOL
  opt
  )

target_link_libraries(LLVMLockstep ${SUPEROPT_LIBS})
