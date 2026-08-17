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
