#ifndef GDWASM3_H
#define GDWASM3_H

#include <_types/_uint32_t.h>
#include <_types/_uint64_t.h>
#include <_types/_uint8_t.h>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <m3_api_wasi.h>
#include <wasm3.h>

namespace godot {

class GDWasm3 : public RefCounted {
  GDCLASS(GDWasm3, RefCounted)

protected:
  static void _bind_methods();

public:
  GDWasm3();
  ~GDWasm3();

  void instantiate(const uint32_t stack_size_in_bytes,
                   PackedByteArray bytecode);

  // Variant func(String fn_name, Array args);
  int64_t func_i64(String fn_name, PackedStringArray args);

  String global_as_string(String global_name);

  PackedByteArray get_memory();

  static String addr_as_string(int64_t ptr);

  void check_m3_result(M3Result result);

  static Variant get_variant_ptr(Variant variant);

  void _init();

  void _reset();

  IM3Environment env;
  IM3Runtime runtime;
  IM3Module module;
};

} // namespace godot

#endif
