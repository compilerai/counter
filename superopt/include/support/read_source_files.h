#pragma once

//read source file (C or LLVM bitcode or ETFG itself) and convert to etfg
string convert_source_file_to_etfg(string const& filename, string const& xml_output_file, string const& xml_output_format_str, string const& function_name, bool model_llvm_semantics, int call_context_depth, string const& input_etfg_filename, string const& tmpdir_path);

//read assembly file (or TFG file) and convert to tfg
string convert_assembly_file_to_tfg(string const& filename, string const& xml_output_file, string const& xml_output_format_str, string const& function_name, bool model_llvm_semantics, string const& etfg_filename, string& asm_filename, string const& tmpdir_path, string const& eqgen_args = "");
