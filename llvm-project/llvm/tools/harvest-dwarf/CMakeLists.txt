set(LLVM_LINK_COMPONENTS
  DebugInfoDWARF
  AllTargetsDescs
  AllTargetsInfos
  MC
  Object
  Support
  )

add_llvm_tool(harvest-dwarf
  main.cpp
  )

add_superopt_libs(slibs)
target_link_libraries(harvest-dwarf PRIVATE ${slibs} gsupport) # XXX HACK because consts_structs_t's init_addr_refs require esp_version_at_entry
