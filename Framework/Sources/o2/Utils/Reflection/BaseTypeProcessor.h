//@CODETOOLIGNORE

#pragma once

#include "o2/Utils/Types/CommonTypes.h"

namespace o2
{
    class Type;

    // ---------------------------------------------------------------------
    // Base type processor class. You can use it as base class or as example
    // ---------------------------------------------------------------------
    class BaseTypeProcessor
    {
    public:
        // ----------------------------------------------------------
        // Field processor. Use it for meta-program field definitions
        // ----------------------------------------------------------
        struct FieldProcessor
        {
            template<typename _attribute_type, typename ... _args>
            FieldProcessor& AddAttribute(_args ... args) { return *this; }

            template<typename _type>
            FieldProcessor& SetDefaultValue(const _type& value) { return *this; }

            FieldProcessor& SetProtectSection(ProtectSection section) { return *this; }

            template<typename _object_type, typename _field_type>
            void FieldBasics(_object_type* object, Type* type, const char* name, void* (*pointerGetter)(void*),
                             _field_type& field) {}
        };

        // ----------------------------------------------------------------
        // Function processor. Use it for meta-program function definitions
        // ----------------------------------------------------------------
        struct FunctionProcessor
        {
            template<typename _attribute_type, typename ... _args>
            FunctionProcessor& AddAttribute(_args ... args) { return *this; }

            FunctionProcessor& SetProtectSection(ProtectSection section) { return *this; }

            template<typename _object_type, typename ... _args>
            void Constructor(_object_type* object, Type* type) {}

            template<typename _object_type, typename _res_type, typename ... _args>
            void Signature(_object_type* object, Type* type, const char* name,
                           _res_type(_object_type::* pointer)(_args ...)) {}

            template<typename _object_type, typename _res_type, typename ... _args>
            void Signature(_object_type* object, Type* type, const char* name,
                           _res_type(_object_type::* pointer)(_args ...) const) {}

            template<auto pointer, typename _object_type>
            void Signature(_object_type* object, Type* type, const char* name) {}

            template<typename _object_type, typename _res_type, typename ... _args>
            void SignatureStatic(_object_type* object, Type* type, const char* name,
                                 _res_type(*pointer)(_args ...)) {}
        };

        template<typename _object_type>
        void Start(_object_type* object, Type* type) {}

        template<typename _object_type>
        void StartBases(_object_type* object, Type* type) {}

        template<typename _object_type, typename _base_type>
        void BaseType(_object_type* object, Type* type, const char* name) {}

        template<typename _object_type>
        void StartFields(_object_type* object, Type* type) {}

        FieldProcessor StartField() { return FieldProcessor(); }

        template<typename _object_type>
        void StartMethods(_object_type* object, Type* type) {}

        FunctionProcessor StartFunction() { return FunctionProcessor(); }
    };
}
