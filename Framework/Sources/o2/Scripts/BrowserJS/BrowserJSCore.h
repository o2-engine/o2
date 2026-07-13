#pragma once

#if defined(SCRIPTING_BACKEND_BROWSERJS)

// Primitive API over the browser's native JS engine: values live in a JS-side handle
// table, refcounted from C++; calls cross the wasm<->JS boundary via EM_JS.

#include <cstddef>
#include <cstdint>

typedef uint32_t o2js_value_t;

typedef o2js_value_t (*o2js_external_handler_t)(o2js_value_t function_obj, o2js_value_t this_val,
                                                const o2js_value_t* args, int args_count);

typedef void (*o2js_native_free_callback_t)(void* native_p);

typedef void (*o2js_error_created_callback_t)(o2js_value_t error_value, void* user_p);

// Initializes JS-side handle table state; safe to call multiple times
void o2js_initialize();

o2js_value_t o2js_acquire(o2js_value_t value);
void o2js_release(o2js_value_t value);

o2js_value_t o2js_undefined();
o2js_value_t o2js_boolean(bool value);
o2js_value_t o2js_number(double value);
o2js_value_t o2js_string(const char* utf8);
o2js_value_t o2js_object();
o2js_value_t o2js_array(uint32_t size);
o2js_value_t o2js_external_function(o2js_external_handler_t handler);

// Returned value matches ScriptValue::ValueType numbering
int o2js_get_value_type(o2js_value_t value);
bool o2js_is_array(o2js_value_t value);
bool o2js_is_constructor(o2js_value_t value);
bool o2js_is_error(o2js_value_t value);

bool o2js_to_boolean(o2js_value_t value);
o2js_value_t o2js_to_number(o2js_value_t value);
o2js_value_t o2js_to_string(o2js_value_t value);
double o2js_as_integer(o2js_value_t value);
double o2js_get_number(o2js_value_t value);

uint32_t o2js_get_string_length(o2js_value_t value);
uint32_t o2js_string_to_buffer(o2js_value_t value, char* buffer, uint32_t bufferSize);

uint32_t o2js_get_array_length(o2js_value_t value);

o2js_value_t o2js_get_property(o2js_value_t obj, o2js_value_t name);
o2js_value_t o2js_get_property_by_index(o2js_value_t obj, uint32_t index);
o2js_value_t o2js_set_property(o2js_value_t obj, o2js_value_t name, o2js_value_t value);
o2js_value_t o2js_set_property_by_index(o2js_value_t obj, uint32_t index, o2js_value_t value);
bool o2js_delete_property(o2js_value_t obj, o2js_value_t name);
bool o2js_delete_property_by_index(o2js_value_t obj, uint32_t index);

o2js_value_t o2js_get_internal_property(o2js_value_t obj, o2js_value_t name);
bool o2js_set_internal_property(o2js_value_t obj, o2js_value_t name, o2js_value_t value);

// Own data property value; undefined when absent
o2js_value_t o2js_get_own_property(o2js_value_t obj, o2js_value_t name);

// Defines enumerable accessor property; returns true value or error
o2js_value_t o2js_define_accessor(o2js_value_t obj, o2js_value_t name, o2js_value_t getter, o2js_value_t setter);

// Own enumerable writable data property names, array indices excluded
o2js_value_t o2js_get_property_names(o2js_value_t obj);

o2js_value_t o2js_get_prototype(o2js_value_t obj);
o2js_value_t o2js_set_prototype(o2js_value_t obj, o2js_value_t proto);

void o2js_set_native_pointer(o2js_value_t obj, void* ptr, o2js_native_free_callback_t freeCb);
void* o2js_get_native_pointer(o2js_value_t obj, o2js_native_free_callback_t freeCb);

o2js_value_t o2js_call_function(o2js_value_t func, o2js_value_t thisValue, const o2js_value_t* args, int count);
o2js_value_t o2js_construct(o2js_value_t func, const o2js_value_t* args, int count);

bool o2js_equals(o2js_value_t a, o2js_value_t b);

// Unwrapped thrown value of an error handle, owned by caller
o2js_value_t o2js_get_error_value(o2js_value_t error);

o2js_value_t o2js_get_global();

o2js_value_t o2js_parse(const char* source, uint32_t sourceLength, const char* filename, uint32_t filenameLength);
o2js_value_t o2js_run(o2js_value_t parsed);

void o2js_set_error_created_callback(o2js_error_created_callback_t callback, void* userData);

// Used JS heap size when the browser exposes it (performance.memory), 0 otherwise
int o2js_get_used_memory();

#endif // SCRIPTING_BACKEND_BROWSERJS
