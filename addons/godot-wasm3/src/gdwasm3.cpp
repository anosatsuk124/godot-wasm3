#include "gdwasm3.h"
#include "godot_cpp/variant/packed_string_array.hpp"
#include "godot_cpp/variant/utility_functions.hpp"
#include <_types/_uint32_t.h>
#include <_types/_uint64_t.h>
#include <_types/_uint8_t.h>
#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <m3_api_libc.h>
#include <m3_api_wasi.h>
#include <sys/_types/_int64_t.h>
#include <wasm3.h>

#include <godot_cpp/core/class_db.hpp>

using namespace godot;

void GDWasm3::_bind_methods() {
  // ClassDB::bind_method()
  ClassDB::bind_method(
      D_METHOD("instantiate", "stack_size_in_bytes", "bytecode"),
      &GDWasm3::instantiate);
  ClassDB::bind_method(D_METHOD("func_i64", "fn_name", "args"),
                       &GDWasm3::func_i64);
  ClassDB::bind_method(D_METHOD("global_as_string", "global_name"),
                       &GDWasm3::global_as_string);
  ClassDB::bind_method(D_METHOD("get_memory"), &GDWasm3::get_memory);
  ClassDB::bind_method(D_METHOD("_init"), &GDWasm3::_init);
  ClassDB::bind_method(D_METHOD("_reset"), &GDWasm3::_reset);

  ClassDB::bind_static_method("GDWasm3", D_METHOD("addr_as_string", "addr"),
                              &GDWasm3::addr_as_string);
}

void GDWasm3::_init() { this->env = m3_NewEnvironment(); }

void GDWasm3::_reset() {
  if (this->env != nullptr) {
    m3_FreeEnvironment(this->env);
  }
  if (this->runtime != nullptr) {
    m3_FreeRuntime(this->runtime);
  }
  if (this->module != nullptr) {
    m3_FreeModule(this->module);
  }

  this->env = nullptr;
  this->runtime = nullptr;
  this->module = nullptr;
}

GDWasm3::GDWasm3() {
  this->_reset();
  this->_init();
}

GDWasm3::~GDWasm3() { this->_reset(); }

void GDWasm3::instantiate(const uint32_t stack_size_in_bytes,
                          PackedByteArray bytecode) {
  this->runtime = m3_NewRuntime(this->env, stack_size_in_bytes, nullptr);

  check_m3_result(m3_ParseModule(this->env, &this->module, bytecode.ptr(),
                                 bytecode.size()));

  check_m3_result(m3_LoadModule(this->runtime, this->module));

  check_m3_result(m3_LinkLibC(this->module));
  check_m3_result(m3_LinkWASI(this->module));
}

void GDWasm3::check_m3_result(M3Result result) {
  if (result != m3Err_none) {
    ERR_PRINT(vformat("M3Result: %s", result));
  }
}

PackedByteArray GDWasm3::get_memory() {
  uint32_t memory_size = m3_GetMemorySize(this->runtime);
  uint8_t *memory = m3_GetMemory(this->runtime, 0, 0);

  PackedByteArray packed_byte_array;
  packed_byte_array.resize(memory_size);

  memcpy((void *)packed_byte_array.ptr(), memory, memory_size);

  return packed_byte_array;
}

String GDWasm3::global_as_string(String global_name) {
  IM3Global global = m3_FindGlobal(this->module, global_name.utf8().ptr());

  IM3TaggedValue value;
  check_m3_result(m3_GetGlobal(global, value));

  return addr_as_string(value->value.i64);
}

String GDWasm3::addr_as_string(int64_t addr) {
  char *cstr = reinterpret_cast<char *>(addr);

  return String(cstr);
}

/*
              NIL,

              // atomic types
              BOOL,
              INT,
              FLOAT,
              STRING,
 */
Variant GDWasm3::get_variant_ptr(Variant variant) {
  Variant::Type type = variant.get_type();

  switch (type) {
  case Variant::Type::NIL:
    return nullptr;
  case Variant::Type::BOOL:
    return bool(variant);
  case Variant::Type::INT:
    return int64_t(variant);
  case Variant::Type::FLOAT:
    return double(variant);
  case Variant::Type::STRING:
    return String(variant).to_utf8_buffer().ptrw();
  default:
    ERR_PRINT(vformat("Unsupported variant type: %d", type));
    return nullptr;
  }
}

int64_t GDWasm3::func_i64(String fn_name, PackedStringArray args) {
  IM3Function fn;

  check_m3_result(m3_FindFunction(&fn, this->runtime, fn_name.utf8().ptr()));

  uint32_t fn_arg_count = m3_GetArgCount(fn);
  auto m3_fn_name = m3_GetFunctionName(fn);

  if (fn_arg_count != args.size()) {
    ERR_PRINT(vformat("Fn name: %s != %s", m3_fn_name, fn_name.utf8().ptr()));
    ERR_PRINT(vformat("Argument count mismatch: %d != %d", fn_arg_count,
                      args.size()));
    return 0;
  }

  if (args.size() == 0) {
    check_m3_result(m3_CallV(fn));
  } else {
    int64_t argc = args.size();

    CharString string_arr[argc];

    for (int64_t i = 0; i < argc; i++) {
      string_arr[i] = args[i].utf8();
    }

    const char *argv[argc];

    for (int64_t i = 0; i < argc; i++) {
      argv[i] = string_arr[i].ptrw();
    }

    check_m3_result(m3_CallArgv(fn, argc, argv));
  }

  auto ret_count = m3_GetRetCount(fn);

  if (ret_count == 0) {
    return 0;
  }

  int64_t ret;
  check_m3_result(m3_GetResultsV(fn, &ret));
  // const void *ret_ptrs[] = {&ret};
  // check_m3_result(m3_GetResults(fn, 1, ret_ptrs));

  return ret;
}
