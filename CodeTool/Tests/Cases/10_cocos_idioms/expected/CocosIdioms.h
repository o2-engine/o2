//@CODETOOL_NON_EXCLUDE
#pragma once
#include "o2/Utils/Basic/IObject.h"

// CC_DLL, CC_CONSTRUCTOR_ACCESS and CC_DISALLOW_COPY_AND_ASSIGN come from cocos
// headers; the tool knows them through its replaces table

namespace cocos2d
{
    namespace ui
    {
        // Export macro of the gui library, another spelling of the same idiom
        class CC_GUI_DLL Widget : public o2::IObject
        {
        public:
            IOBJECT(Widget);

            enum class FocusDirection { Left, Right };

            virtual void setTouchEnabled(bool enabled);
            bool isTouchEnabled() const;

        protected:
            bool _touchEnabled = false; // @EDITOR_PROPERTY
        };
    }

    namespace backend { class ProgramState; }

    class Texture2D;

    /**
     * Doc comment in the cocos style, right before the class.
     */
    class CC_DLL CocosNode : public o2::IObject
    {
    public:
        IOBJECT(CocosNode);

        /** Pointer glued to the function name, cocos style. */
        virtual backend::ProgramState *getProgramState() const;

        /** Pointer glued to the argument name. */
        virtual void setTexture(Texture2D *texture);

        /** Reference glued to the argument name. */
        virtual void setName(const std::string &name);

        /** Multi word fundamental types. */
        virtual void setCameraMask(unsigned short mask, bool applyChildren = true);
        virtual unsigned short getCameraMask() const;

        virtual std::string getDescription() const;

    CC_CONSTRUCTOR_ACCESS:
        CocosNode();
        virtual ~CocosNode();

    protected:
        /// Brace initialized default value
        Rect  _centerRectNormalized = {0, 0, 1, 1};

        bool  _flippedX = false; // @EDITOR_PROPERTY
        float _rotationX;
        unsigned short _cameraMask;

    private:
        CC_DISALLOW_COPY_AND_ASSIGN(CocosNode);
    };
}

class     DeprecatedApi : public o2::IObject
{
public:
    CC_DEPRECATED_ATTRIBUTE static DeprecatedApi* createLegacy(int value);
    CC_DEPRECATED_ATTRIBUTE virtual bool setLegacyPath(const std::string& path, float fontSize = 0);

    int legacyValue = 0;

    IOBJECT(DeprecatedApi);
};
// --- META ---

PRE_ENUM_META(cocos2d::ui::Widget::FocusDirection);

CLASS_BASES_META(DeprecatedApi)
{
    BASE_CLASS(o2::IObject);
}
END_META;
CLASS_FIELDS_META(DeprecatedApi)
{
    FIELD().PUBLIC().DEFAULT_VALUE(0).NAME(legacyValue);
}
END_META;
CLASS_METHODS_META(DeprecatedApi)
{

    FUNCTION().PUBLIC().SIGNATURE_STATIC(DeprecatedApi*, createLegacy, int);
    FUNCTION().PUBLIC().SIGNATURE(bool, setLegacyPath, const std::string&, float);
}
END_META;

CLASS_BASES_META(cocos2d::CocosNode)
{
    BASE_CLASS(o2::IObject);
}
END_META;
CLASS_FIELDS_META(cocos2d::CocosNode)
{
    FIELD().PROTECTED().NAME(_centerRectNormalized);
    FIELD().PROTECTED().EDITOR_PROPERTY_ATTRIBUTE().DEFAULT_VALUE(false).NAME(_flippedX);
    FIELD().PROTECTED().NAME(_rotationX);
    FIELD().PROTECTED().NAME(_cameraMask);
}
END_META;
CLASS_METHODS_META(cocos2d::CocosNode)
{

    FUNCTION().PUBLIC().SIGNATURE(backend::ProgramState*, getProgramState);
    FUNCTION().PUBLIC().SIGNATURE(void, setTexture, Texture2D*);
    FUNCTION().PUBLIC().SIGNATURE(void, setName, const std::string&);
    FUNCTION().PUBLIC().SIGNATURE(void, setCameraMask, unsigned short, bool);
    FUNCTION().PUBLIC().SIGNATURE(unsigned short, getCameraMask);
    FUNCTION().PUBLIC().SIGNATURE(std::string, getDescription);
    FUNCTION().PUBLIC().CONSTRUCTOR();
}
END_META;

CLASS_BASES_META(cocos2d::ui::Widget)
{
    BASE_CLASS(o2::IObject);
}
END_META;
CLASS_FIELDS_META(cocos2d::ui::Widget)
{
    FIELD().PROTECTED().EDITOR_PROPERTY_ATTRIBUTE().DEFAULT_VALUE(false).NAME(_touchEnabled);
}
END_META;
CLASS_METHODS_META(cocos2d::ui::Widget)
{

    FUNCTION().PUBLIC().SIGNATURE(void, setTouchEnabled, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, isTouchEnabled);
}
END_META;
// --- END META ---
